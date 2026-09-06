#include "wmsp_private.h"

static int WmspMakePortPacket(u32 pollBitmap, u32 childBitmap, u32 mpReadyBitmap, u16 *wmHeader, u32 *destBitmap, u32 *dataLength);
static u16 WmspGetTmptt(u32 dataLength, u32 txop, u32 pollBitmap, u32 targetVCount);
static void WmspDataToString(char *buf, u32 buflen, const u16 *data, u32 size);

void WMSP_InitSendQueue(void)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    u16 i;

    OS_LockMutex(&status->sendQueueMutex);

    MI_CpuClear16(status->sendQueueData, sizeof(status->sendQueueData));
    for (i = 0; i < WM_SEND_QUEUE_NUM - 1; i++) {
        status->sendQueueData[i].next = (u16)(i + 1);
    }
    status->sendQueueData[i].next = WM_SEND_QUEUE_END;

    status->sendQueueFreeList.head = 0;
    status->sendQueueFreeList.tail = i;

    for (i = 0; i < WM_PRIORITY_LEVEL; i++) {
        status->readyQueue[i].head = WM_SEND_QUEUE_END;
        status->readyQueue[i].tail = WM_SEND_QUEUE_END;

        status->sendQueue[i].head = WM_SEND_QUEUE_END;
        status->sendQueue[i].tail = WM_SEND_QUEUE_END;
    }

    OS_UnlockMutex(&status->sendQueueMutex);
}

void WMSP_SendMaKeyData(void)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    u32 wlBuf[128];
    WlMaKeyDataCfm *wlResult;

    u32 destBitmap;
    u32 dataLength;
    u16 wmHeader;

    if (status->child_bitmap == 0) {
        WMSP_DLOG("child_bitmap == 0 @WMSP_SendMaKeyData");
        return;
    }

    status->mp_setDataFlag = TRUE;

    wmHeader = 0;

    if (WmspMakePortPacket(0x0001U, 0x0001U, 0x0001U, &wmHeader, &destBitmap, &dataLength) == WM_ERRCODE_FAILED) {
        WMSP_DLOG("Failed to make a packet @WMSP_SendMaKeyData");
        status->mp_setDataFlag = FALSE;
        return;
    }

    if (status->VSyncFlag == TRUE) {
        wmHeader |= WM_HEADER_VSYNC;
    }

    WMSP_DLOGF_PORT_SENDRECV("KeyData H: %04x, L: %d", wmHeader, dataLength);

    wlResult = WMSP_WL_MaKeyData((u16 *)wlBuf, (u16)dataLength, wmHeader, (u16 *)status->mp_sendBuf);

#ifdef WM_DEBUG_PORT_SENDRECV
    {
        u16 in, out;

        in = *(vu16 *)MREG_KEYIN_ADRS;
        out = *(vu16 *)MREG_KEYOUT_ADRS;
        WMSP_DLOGF_PORT_SENDRECV("KeyData Done I:%d O:%d", (in & 0x8000U) != 0, (out & 0x8000U) != 0);
    }
#endif

    if (wlResult->resultCode != WL_CMDRES_SUCCESS) {
        WMSP_DLOGF("MaKeyData failed. Result:%d", wlResult->resultCode);
        if (wlResult->resultCode != WL_CMDRES_NOT_ENOUGH_MEM) {
            status->mp_setDataFlag = FALSE;
        }
    }
}

void WMSP_SendMaMP(u16 pollBitmap)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    u32 wlBuf[128];
    WlMaMpCfm *wlResult;

    const u16 resume = 0;
    const u16 retryLimit = 0;
    u32 txop;
    u16 tmptt;
    u16 currTsf;
    u32 destBitmap;
    u32 dataLength;
    u16 wmHeader;
    u32 currPollBitmap;
    u32 childBitmap;
    u32 mpReadyBitmap;

    {
        OSIntrMode e = OS_DisableInterrupts();
        childBitmap = status->child_bitmap;
        mpReadyBitmap = status->mp_readyBitmap;
        (void)OS_RestoreInterrupts(e);
    }

    wmHeader = 0;

    if (status->mp_newFrameFlag == TRUE) {
        pollBitmap = 0xffffU;
        status->mp_newFrameFlag = FALSE;
    }

    if (status->mp_pingFlag == FALSE) {
        u16 recvSize = status->mp_recvSize;
        int nextRecvBufSize;

        if (WmspMakePortPacket(pollBitmap, childBitmap, mpReadyBitmap, &wmHeader, &destBitmap, &dataLength) == WM_ERRCODE_FAILED) {
            WMSP_DLOG("Failed to make a packet @WMSP_SendMaMP");
            status->mp_count = 0;
            status->mp_limitCount = 0;
            return;
        }

        txop = (u32)(recvSize + WM_HEADER_SIZE);

        if (status->mp_current_minPollBmpMode == TRUE) {
            currPollBitmap = destBitmap;
        } else {
            currPollBitmap = pollBitmap;
        }

        currPollBitmap &= childBitmap;

        nextRecvBufSize = WMSP_GetMPOneReceiveBufferSizeParent(recvSize, MATH_CountPopulation(currPollBitmap));
        if (status->mp_recvBufSize < nextRecvBufSize) {
            WMSP_DLOGF("not enough recvBufSize. %d < %d (recvSize = %d, currPollBitmap = %04X)",
                status->mp_recvBufSize,
                nextRecvBufSize,
                recvSize,
                currPollBitmap);

            (void)WMSP_FlushSendQueue(FALSE, (u16)currPollBitmap);

            status->mp_count = 0;
            status->mp_limitCount = 0;
            return;
        }

        if (status->mp_count == 1 || status->mp_limitCount == 1) {
            tmptt = WmspGetTmptt(dataLength, txop, currPollBitmap, status->mp_parentVCount);

            wmHeader |= WM_HEADER_VSYNC;
        } else {
            tmptt = 0;
        }
    } else {
        WM_DPRINTF("PING!\n");

        status->mp_pingFlag = FALSE;

        currPollBitmap = childBitmap;
        txop = 0x80d6;
        tmptt = 0;
        dataLength = 0;
        wmHeader &= ~WM_HEADER_VSYNC;

        {
            OSIntrMode e = OS_DisableInterrupts();
            status->mp_count++;
            status->mp_limitCount++;
            (void)OS_RestoreInterrupts(e);
        }
    }

    WMSP_DLOGF_PORT_SENDRECV("MP P: %04x, C: %04x, H: %04x", currPollBitmap, childBitmap, wmHeader);

    currTsf = *(u16 *)MREG_TSF0;

    wlResult = WMSP_WL_MaMp((u16 *)wlBuf, resume, retryLimit, (u16)txop, (u16)currPollBitmap, tmptt, currTsf, (u16)dataLength, wmHeader, (u16 *)status->mp_sendBuf);

    status->mp_prevPollBitmap = (u16)currPollBitmap;
    status->mp_prevWmHeader = wmHeader;
    status->mp_prevTxop = (u16)txop;
    status->mp_prevDataLength = (u16)dataLength;

    if (wlResult->resultCode != WL_CMDRES_SUCCESS) {
        WMSP_DLOGF("MaMp failed. Result:%d", wlResult->resultCode);
    }
}

void WMSP_ResumeMaMP(u16 pollBitmap)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    u32 wlBuf[128];
    WlMaMpCfm *wlResult;

    u16 tmptt;
    u32 currPollBitmap;
    u16 currTsf;
    u32 wmHeader;
    u32 childBitmap;
    u32 mpReadyBitmap;
    int nextRecvBufSize;
    u16 recvSize;

    {
        OSIntrMode e = OS_DisableInterrupts();
        childBitmap = status->child_bitmap;
        mpReadyBitmap = status->mp_readyBitmap;
        (void)OS_RestoreInterrupts(e);
    }

    currTsf = *(u16 *)MREG_TSF0;

    wmHeader = status->mp_prevWmHeader;
    currPollBitmap = (u32)(pollBitmap & status->child_bitmap);

    recvSize = status->mp_recvSize;
    nextRecvBufSize = WMSP_GetMPOneReceiveBufferSizeParent(recvSize, MATH_CountPopulation(currPollBitmap));
    if (status->mp_recvBufSize < nextRecvBufSize) {
        WMSP_DLOGF("not enough recvBufSize at ResumeMaMP. %d < %d (recvSize = %d, currPollBitmap = %04X)",
            status->mp_recvBufSize,
            nextRecvBufSize,
            recvSize,
            currPollBitmap);

        OS_Sleep(2);
        WMSP_RequestResumeMP();
        return;
    }

    if (status->mp_count == 1 || status->mp_limitCount == 1) {
        tmptt = WmspGetTmptt(status->mp_prevDataLength, status->mp_prevTxop, currPollBitmap, status->mp_parentVCount);

        wmHeader |= WM_HEADER_VSYNC;
    } else {
        tmptt = 0;
        wmHeader &= ~WM_HEADER_VSYNC;
    }

    WMSP_DLOGF_PORT_SENDRECV("MP (Resume) P: %04x, C: %04x, H: %04x", currPollBitmap, status->child_bitmap, wmHeader);

    wlResult = WMSP_WL_MaMp((u16 *)wlBuf, 0x800c, 0, 0, (u16)currPollBitmap, tmptt, currTsf, 0, (u16)wmHeader, NULL);

#ifdef WM_DEBUG
    if (wlResult->resultCode != WL_CMDRES_SUCCESS) {
        OS_Warning("MaMp(Resume) failed. Result:%d", wlResult->resultCode);
    }
#endif
}

static int
WmspMakePortPacket(u32 pollBitmap, u32 childBitmap, u32 mpReadyBitmap, u16 *wmHeader, u32 *destBitmap, u32 *dataLength)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    u32 maxSize = status->mp_sendSize;
    u16 *sendBuf = (u16 *)status->mp_sendBuf;

    int restSize;
    u32 portBitmap;
    u16 *write_p;
    BOOL firstPacket;
    BOOL isParent;
    u32 packetCount;
    u32 tmpDestBitmap;
    u32 tmpDataLength;
    BOOL destCheckFlag;
    u32 destBitmapMask;
    u32 reachableBitmap;

    WMSP_DLOGF_PORT_SEND_QUEUE("M %d", status->sendQueueInUse);

#ifdef WM_DEBUG
    WMSP_CheckSendQueue("beginning of WmspMakePortPacket()");
#endif

    if (status->mp_sendBufSize < WMSP_GetMPSendBufferSize((int)maxSize)) {
        WMSP_DLOGF("not enough sendBufSize. %d < %d (sendSize = %d)",
            status->mp_sendBufSize,
            WMSP_GetMPSendBufferSize((int)maxSize),
            maxSize);
        return WM_ERRCODE_FAILED;
    }

    if (status->aid >= WM_NUM_MAX_CHILD + 1) {
        OS_TWarning("invaild aid: %d", status->aid);
        return WM_ERRCODE_FAILED;
    }

    if (status->state == WM_STATE_MP_PARENT) {
        isParent = TRUE;
    } else if (status->state == WM_STATE_MP_CHILD) {
        isParent = FALSE;
    } else {
        OS_TWarning("invalid state: %d", status->state);
        return WM_ERRCODE_FAILED;
    }

    *wmHeader = 0;
    *destBitmap = 0;
    *dataLength = 0;
    tmpDestBitmap = 0;
    tmpDataLength = 0;
    restSize = (s32)maxSize;
    packetCount = 0;

    if (restSize < 0) {
        return WM_ERRCODE_FAILED;
    }

    destBitmapMask = mpReadyBitmap;
    destCheckFlag = TRUE;
    if (isParent) {
        if (status->mp_current_minPollBmpMode) {
            destBitmapMask = childBitmap;
            destCheckFlag = FALSE;
        }
    } else {
        destCheckFlag = FALSE;
    }

    reachableBitmap = mpReadyBitmap & pollBitmap;

    portBitmap = 0;
    write_p = sendBuf;
    firstPacket = TRUE;

    OS_LockMutex(&status->sendQueueMutex);

    if (status->sendQueueInUse == TRUE) {
#ifdef WM_DEBUG
        OS_Warning("too many calls of WmspMakePortPacket()");
#endif
        OS_UnlockMutex(&status->sendQueueMutex);
        return WM_ERRCODE_FAILED;
    }
    status->sendQueueInUse = TRUE;

    {
        WMPortSendQueueData *queueData = status->sendQueueData;
        u32 iPrio;

#ifdef WM_DEBUG
        for (iPrio = 0; iPrio < WM_PRIORITY_LEVEL; iPrio++) {
            WMPortSendQueue *sendQueue;
            sendQueue = &(status->sendQueue[iPrio]);
            if (sendQueue->head != WM_SEND_QUEUE_END || sendQueue->tail != WM_SEND_QUEUE_END) {
                OS_TWarning("[ERROR] sendQueue[%d] is not empty: head: %d, tail: %d\n", iPrio, sendQueue->head, sendQueue->tail);
            }
        }
#endif
        
        for (iPrio = 0; iPrio < WM_PRIORITY_LEVEL && restSize > WM_HEADER_SIZE; iPrio++) {
            WMPortSendQueue *queue;
            WMPortSendQueue *sendQueue;
            u32 index;
            u16 *prevPointer;
            u32 prevIndex;

            queue = &(status->readyQueue[iPrio]);
            sendQueue = &(status->sendQueue[iPrio]);
            prevPointer = &queue->head;
            prevIndex = WM_SEND_QUEUE_END;
            for (index = queue->head; index != WM_SEND_QUEUE_END && restSize > WM_HEADER_SIZE;
                index = (u16)((index != WM_SEND_QUEUE_END) ? queueData[index].next : queue->head)) {
                WMPortSendQueueData *data = &(queueData[index]);
                u32 portBit;
                u32 currDestBitmap;
                u16 *contents;
                BOOL addSeqNoFlag, addDestBmpFlag;

                portBit = (1U << data->port);
                if ((portBitmap & portBit) != 0) {
                    goto next_packet;
                }
                portBitmap |= portBit;

                currDestBitmap = data->restBitmap & destBitmapMask;

                if (destCheckFlag && (currDestBitmap & ~reachableBitmap)) {
                    WM_DPRINTF("cannot send to dest %04x ( C:%04x R:%04x P:%04x ). skipped. @ WmspMakePortPacket\n",
                        data->destBitmap,
                        childBitmap,
                        mpReadyBitmap,
                        pollBitmap);
                    goto next_packet;
                }

                addSeqNoFlag = (data->port & WM_SEQ_PORT_FLAG) ? TRUE : FALSE;
                addDestBmpFlag = (isParent && ((data->restBitmap | 1) != 0xffffU)) ? TRUE : FALSE;

                if ((data->size & 1) == 1) {
                    OS_Warning("send an odd length packet. data->size: %d", data->size);
                    data->size++;
                }

                if (data->size + (firstPacket ? 0 : WM_HEADER_SIZE) + (addSeqNoFlag ? 2 : 0) + (addDestBmpFlag ? 2 : 0) <= restSize) {
                    if (firstPacket == FALSE) {
                        wmHeader = write_p;
                        *wmHeader = 0;
                        write_p++;
                        tmpDataLength += WM_HEADER_SIZE;
                        restSize -= WM_HEADER_SIZE;
                    }
                    *wmHeader |= ((data->port << WM_HEADER_PORT_SHIFT) & WM_HEADER_PORT_MASK)
                        | ((data->size / WM_HEADER_LENGTH_SCALE) & WM_HEADER_LENGTH_MASK);
                    contents = write_p;
                    MI_CpuCopy16(data->data, write_p, data->size);
                    write_p += data->size / sizeof(u16);
                    tmpDataLength += data->size;
                    restSize -= data->size;

                    if (addSeqNoFlag == TRUE) {
                        u32 seqNo;
                        if (data->seqNo & 1)
                        {
                            seqNo = status->portSeqNo[status->aid][data->port & WM_SEQ_PORT_MASK]++;
                            data->seqNo = (u16)(seqNo << 1);
                        } else {
                            seqNo = (u32)(data->seqNo >> 1U);
                        }
                        seqNo &= 0x7fffU;
                        *write_p = (u16)seqNo;
                        write_p++;
                        tmpDataLength += 2;
                        restSize -= 2;
                    }

                    if (addDestBmpFlag == TRUE) {
                        *wmHeader |= WM_HEADER_DEST_BITMAP;
                        *write_p = data->restBitmap;
                        write_p++;
                        tmpDataLength += 2;
                        restSize -= 2;
                    }

                    if (firstPacket == TRUE) {
                        firstPacket = FALSE;
                    }

                    if (queue->tail == index) {
                        WM_ASSERT(data->next == WM_SEND_QUEUE_END);
                        queue->tail = (u16)prevIndex;
                    }
                    *prevPointer = data->next;

                    data->next = WM_SEND_QUEUE_END;
                    if (sendQueue->tail == WM_SEND_QUEUE_END) {
                        WM_ASSERT(sendQueue->head == WM_SEND_QUEUE_END);
                        sendQueue->head = (u16)index;
                    } else {
                        WM_ASSERT(queueData[sendQueue->tail].next == WM_SEND_QUEUE_END);
                        queueData[sendQueue->tail].next = (u16)index;
                    }
                    sendQueue->tail = (u16)index;

                    WM_ASSERT(data->sendingBitmap == 0);
                    data->sendingBitmap = (u16)currDestBitmap;

                    packetCount++;

#ifdef WM_DEBUG_PORT_SENDRECV
                    {
                        char str_data[40], str_seq[16], str_dst[36];
                        WmspDataToString(str_data, sizeof(str_data), contents, data->size);
                        if (addSeqNoFlag == TRUE) {
                            (void)OS_SNPrintf(str_seq, sizeof(str_seq), ", seq=%d", data->seqNo >> 1);
                        } else {
                            str_seq[0] = '\0';
                        }
                        if (addDestBmpFlag == TRUE) {
                            (void)OS_SNPrintf(str_dst, sizeof(str_dst), ", dst=%04x(%04x,%04x,%04x)", currDestBitmap, data->destBitmap, data->restBitmap, data->sentBitmap);
                        } else {
                            str_dst[0] = '\0';
                        }
                        WMSP_DLOGF_PORT_SENDRECV("-> packet %04x: %slen=%d%s%s", *wmHeader, str_data, data->size, str_seq, str_dst);
                    }
#endif

                    portBitmap &= ~portBit;

                    tmpDestBitmap |= data->destBitmap;

                    index = prevIndex;

                    if (status->mp_current_singlePacketMode == TRUE) {
                        goto packet_found;
                    }
                } else {
                    WM_DPRINTF("not enough room. restSize: %d, packetSize: %d, destBitmap: %04x, pollBitmap: %04x\n",
                        restSize,
                        data->size + (addSeqNoFlag ? 2 : 0) + (addDestBmpFlag ? 2 : 0),
                        data->destBitmap,
                        pollBitmap);
                }

next_packet:
                if (index != WM_SEND_QUEUE_END) {
                    prevPointer = &(queueData[index].next);
                } else {
                    prevPointer = &queue->head;
                }
                prevIndex = index;
            }
        }
    }

packet_found:

    *destBitmap = tmpDestBitmap;
    *dataLength = tmpDataLength;

    OS_UnlockMutex(&status->sendQueueMutex);

    return WM_ERRCODE_SUCCESS;
}

int WMSP_PutSendQueue(u32 childBitmap, u16 priority, u16 port, u32 destBitmap, const u16 *sendData, u16 sendDataSize, WMCallbackFunc callback, void *arg)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    WMPortSendQueueData *queueData = status->sendQueueData;
    WMPortSendQueue *queue = &status->readyQueue[priority];
    WMPortSendQueueData *newData;
    u16 newIndex;

    if (sendDataSize == 0) {
        OS_Warning("put null data");
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (sendDataSize + ((port & WM_SEQ_PORT_FLAG) ? 2 : 0) > 0x204) {
        OS_Warning("too large send-data: size=%d+%d, maxSize=%d", sendDataSize, ((port & WM_SEQ_PORT_FLAG) ? 2 : 0), status->mp_maxSendSize);
        return WM_ERRCODE_INVALID_PARAM;
    }

    OS_LockMutex(&status->sendQueueMutex);

    newIndex = status->sendQueueFreeList.head;

    if (newIndex == WM_SEND_QUEUE_END) {
        OS_UnlockMutex(&status->sendQueueMutex);
        return WM_ERRCODE_SEND_QUEUE_FULL;
    }

    newData = &queueData[newIndex];

    status->sendQueueFreeList.head = newData->next;
    if (status->sendQueueFreeList.tail == newIndex) {
        WM_ASSERT(newData->next == WM_SEND_QUEUE_END);
        status->sendQueueFreeList.tail = WM_SEND_QUEUE_END;
    }

    newData->port = port;
    newData->destBitmap = (u16)destBitmap;
    newData->restBitmap = (u16)(destBitmap & childBitmap);
    newData->sentBitmap = 0;
    newData->sendingBitmap = 0;
    newData->data = sendData;
    newData->size = sendDataSize;
    newData->callback = callback;
    newData->arg = arg;
    newData->next = WM_SEND_QUEUE_END;
    newData->seqNo = 0xffffU;
    newData->retryCount = status->mp_current_defaultRetryCount;

    if (queue->tail == WM_SEND_QUEUE_END) {
        WM_ASSERT(queue->head == WM_SEND_QUEUE_END);
        queue->head = newIndex;
    } else {
        WM_ASSERT(queueData[queue->tail].next == WM_SEND_QUEUE_END);
        queueData[queue->tail].next = newIndex;
    }
    queue->tail = newIndex;

    OS_UnlockMutex(&status->sendQueueMutex);

#ifdef WM_DEBUG_PORT_SEND_QUEUE
    {
        char str_data[40];
        WmspDataToString(str_data, sizeof(str_data), sendData, sendDataSize);
        WMSP_DLOGF_PORT_SEND_QUEUE("Q port %x: %slen=%d", port, str_data, sendDataSize);
    }
#endif

#ifdef WM_DEBUG
    WMSP_CheckSendQueue("end of PutSendQueue()");
#endif

    return WM_ERRCODE_OPERATING;
}

BOOL WMSP_FlushSendQueue(BOOL timeout, u16 pollBitmap)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    WMPortSendQueueData *queueData = status->sendQueueData;
    int iPrio;
    BOOL isParent;
    u32 childBitmap;
    u32 mpReadyBitmap;
    BOOL retryFlag = FALSE;

    WMSP_DLOGF_PORT_SEND_QUEUE("F %d", status->sendQueueInUse);

    if (status->state == WM_STATE_MP_PARENT) {
        isParent = TRUE;
    } else if (status->state == WM_STATE_MP_CHILD) {
        isParent = FALSE;
    } else {
        OS_Warning("invalid state: %d", status->state);
        return FALSE;
    }

    OS_LockMutex(&status->sendQueueMutex);

    if (status->sendQueueInUse == FALSE) {
        OS_UnlockMutex(&status->sendQueueMutex);
        return FALSE;
    }

    if (isParent) {
        OSIntrMode e = OS_DisableInterrupts();
        childBitmap = status->child_bitmap;
        mpReadyBitmap = status->mp_readyBitmap;
        (void)OS_RestoreInterrupts(e);
    } else {
        childBitmap = 0x0001U;
        mpReadyBitmap = 0x0001U;
    }

    for (iPrio = 0; iPrio < WM_PRIORITY_LEVEL; iPrio++) {
        WMPortSendQueue *queue;
        WMPortSendQueue tmpQueue;
        u32 index;
        u16 *prevPointer;
        u32 prevIndex;

        queue = &status->sendQueue[iPrio];
        prevPointer = &queue->head;
        prevIndex = WM_SEND_QUEUE_END;
        tmpQueue.head = tmpQueue.tail = WM_SEND_QUEUE_END;
        for (index = queue->head; index != WM_SEND_QUEUE_END;
            index = ((index != WM_SEND_QUEUE_END) ? queueData[index].next : queue->head)) {
            WMPortSendQueueData *data = &queueData[index];

            if ((data->restBitmap & ~childBitmap) != 0) {
                WMSP_DLOGF("rest(%04x) & ~child(%04x) != 0: dest(%04x), sent(%04x)",
                    data->restBitmap,
                    childBitmap,
                    data->destBitmap,
                    data->sentBitmap);
            }

            if (!timeout) {
                data->sentBitmap |= data->sendingBitmap & ~pollBitmap;
                data->restBitmap &= ~data->sentBitmap;
            }
            data->restBitmap &= childBitmap;
            data->sendingBitmap = 0;

            if (data->restBitmap != 0 && ((data->port & WM_SEQ_PORT_FLAG) || (data->retryCount > 0))) {
                retryFlag = TRUE;

                if (data->retryCount > 0) {
                    data->retryCount--;
                }

                if (data->next == WM_SEND_QUEUE_END) {
                    WM_ASSERT(queue->tail == index);
                    queue->tail = (u16)prevIndex;
                }
                *prevPointer = data->next;

                data->next = WM_SEND_QUEUE_END;
                if (tmpQueue.tail == WM_SEND_QUEUE_END) {
                    WM_ASSERT(tmpQueue.head == WM_SEND_QUEUE_END);
                    tmpQueue.head = (u16)index;
                } else {
                    WM_ASSERT(queueData[tmpQueue.tail].next == WM_SEND_QUEUE_END);
                    queueData[tmpQueue.tail].next = (u16)index;
                }
                tmpQueue.tail = (u16)index;
            } else {

                WMPortSendCallback *cb_PortSend;

                cb_PortSend = (WMPortSendCallback *)WMSP_GetBuffer4Callback2Wm9();

                cb_PortSend->apiid = WM_APIID_PORT_SEND;
                if (data->restBitmap == 0) {
                    cb_PortSend->errcode = WM_ERRCODE_SUCCESS;
                } else {
                    cb_PortSend->errcode = WM_ERRCODE_SEND_FAILED;
                }
                cb_PortSend->state = WM_STATECODE_PORT_SEND;
                cb_PortSend->port = data->port;
                cb_PortSend->destBitmap = data->destBitmap;
                cb_PortSend->restBitmap = data->restBitmap;
                cb_PortSend->sentBitmap = data->sentBitmap;
                cb_PortSend->size = data->size;
                cb_PortSend->data = data->data;
                cb_PortSend->callback = data->callback;
                cb_PortSend->arg = data->arg;
                cb_PortSend->seqNo = data->seqNo;

                {
                    u16 parentSize = status->mp_parentSize;
                    u16 childSize = status->mp_childSize;
                    cb_PortSend->maxSendDataSize = (status->aid == 0) ? parentSize : childSize;
                    cb_PortSend->maxRecvDataSize = (status->aid == 0) ? childSize : parentSize;
                }

#ifdef WM_DEBUG_PORT_SEND_QUEUE
                {
                    char str_data[40], str_seq[16];
                    WmspDataToString(str_data, sizeof(str_data), data->data, data->size);
                    if (data->seqNo != 0xffffU) {
                        (void)OS_SNPrintf(str_seq, sizeof(str_seq), ", seq=%d", data->seqNo >> 1);
                    } else {
                        str_seq[0] = '\0';
                    }
                    WMSP_DLOGF_PORT_SEND_QUEUE("D port %x: %slen=%d%s", data->port, str_data, data->size, str_seq);
                }
#endif

                WMSP_ReturnResult2Wm9(cb_PortSend);

                if (data->next == WM_SEND_QUEUE_END) {
                    WM_ASSERT(queue->tail == index);
                    queue->tail = (u16)prevIndex;
                }
                *prevPointer = data->next;

                data->next = WM_SEND_QUEUE_END;
                if (status->sendQueueFreeList.tail == WM_SEND_QUEUE_END) {
                    WM_ASSERT(status->sendQueueFreeList.head == WM_SEND_QUEUE_END);
                    status->sendQueueFreeList.head = (u16)index;
                } else {
                    WM_ASSERT(queueData[status->sendQueueFreeList.tail].next == WM_SEND_QUEUE_END);
                    queueData[status->sendQueueFreeList.tail].next = (u16)index;
                }
                status->sendQueueFreeList.tail = (u16)index;
            }

            index = prevIndex;
            if (index != WM_SEND_QUEUE_END) {
                prevPointer = &(queueData[index].next);
            } else {
                prevPointer = &queue->head;
            }
            prevIndex = index;
        }
        if (tmpQueue.tail != WM_SEND_QUEUE_END) {
            WM_ASSERT(tmpQueue.head != WM_SEND_QUEUE_END);
            WM_ASSERT(queueData[tmpQueue.tail].next == WM_SEND_QUEUE_END);
            queueData[tmpQueue.tail].next = status->readyQueue[iPrio].head;
            if (status->readyQueue[iPrio].tail == WM_SEND_QUEUE_END) {
                WM_ASSERT(status->readyQueue[iPrio].head == WM_SEND_QUEUE_END);
                status->readyQueue[iPrio].tail = tmpQueue.tail;
            }
            status->readyQueue[iPrio].head = tmpQueue.head;
        }
    }

#ifdef WM_DEBUG_PORT_SENDRECV
    if (retryFlag == TRUE) {
        WMSP_DLOG_PORT_SENDRECV("-- requeued");
    }
#endif

    status->sendQueueInUse = FALSE;

    OS_UnlockMutex(&status->sendQueueMutex);

#ifdef WM_DEBUG
    WMSP_CheckSendQueue("end of FlushSendQueue()");
#endif

    return retryFlag;
}

void WMSP_CleanSendQueue(u16 aidBitmap)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    WMPortSendQueueData *queueData = status->sendQueueData;
    int iQueue;
    int iPrio;
    u32 aidBitmapMask;
    u32 childBitmap;

#ifdef WM_DEBUG
    WMSP_CheckSendQueue("beginning of CleanSendQueue()");
#endif

    WMSP_DLOGF_PORT_SEND_QUEUE("C %d", status->sendQueueInUse);

    childBitmap = status->child_bitmap;
    aidBitmapMask = (~aidBitmap) & childBitmap;

    OS_LockMutex(&status->sendQueueMutex);

    for (iQueue = 0; iQueue < 2; iQueue++) {
        WMPortSendQueue *baseQueue;

        baseQueue = (iQueue == 0) ? status->sendQueue : status->readyQueue;
        baseQueue = status->readyQueue;

        for (iPrio = 0; iPrio < WM_PRIORITY_LEVEL; iPrio++) {
            WMPortSendQueue *queue;
            u32 index;
            u16 *prevPointer;
            u32 prevIndex;

            queue = &baseQueue[iPrio];
            prevPointer = &queue->head;
            prevIndex = WM_SEND_QUEUE_END;
            for (index = queue->head; index != WM_SEND_QUEUE_END;
                index = (u32)((index != WM_SEND_QUEUE_END) ? queueData[index].next : queue->head)) {
                WMPortSendQueueData *data = &queueData[index];

                if ((data->restBitmap & ~childBitmap) != 0) {
                    WMSP_DLOGF("rest(%04x) & ~child(%04x) != 0: dest(%04x), sent(%04x)",
                        data->restBitmap,
                        childBitmap,
                        data->destBitmap,
                        data->sentBitmap);
                }

                data->restBitmap &= aidBitmapMask;
                data->sendingBitmap &= aidBitmapMask;

                if (data->restBitmap == 0) {

                    WMPortSendCallback *cb_PortSend;

                    cb_PortSend = (WMPortSendCallback *)WMSP_GetBuffer4Callback2Wm9();

                    cb_PortSend->apiid = WM_APIID_PORT_SEND;
                    cb_PortSend->errcode = WM_ERRCODE_SUCCESS;
                    cb_PortSend->state = WM_STATECODE_PORT_SEND;
                    cb_PortSend->port = data->port;
                    cb_PortSend->destBitmap = data->destBitmap;
                    cb_PortSend->restBitmap = data->restBitmap;
                    cb_PortSend->sentBitmap = data->sentBitmap;
                    cb_PortSend->size = data->size;
                    cb_PortSend->data = data->data;
                    cb_PortSend->callback = data->callback;
                    cb_PortSend->arg = data->arg;
                    cb_PortSend->seqNo = data->seqNo;

                    {
                        u16 parentSize = status->mp_parentSize;
                        u16 childSize = status->mp_childSize;
                        cb_PortSend->maxSendDataSize = (status->aid == 0) ? parentSize : childSize;
                        cb_PortSend->maxRecvDataSize = (status->aid == 0) ? childSize : parentSize;
                    }

#ifdef WM_DEBUG_PORT_SEND_QUEUE
                    {
                        char str_data[40], str_seq[16];
                        WmspDataToString(str_data, sizeof(str_data), data->data, data->size);
                        if (data->seqNo != 0xffffU) {
                            (void)OS_SNPrintf(str_seq, sizeof(str_seq), ", seq=%d", data->seqNo >> 1);
                        } else {
                            str_seq[0] = '\0';
                        }
                        WMSP_DLOGF_PORT_SEND_QUEUE("C port %x: %slen=%d%s", data->port, str_data, data->size, str_seq);
                    }
#endif

                    WMSP_ReturnResult2Wm9(cb_PortSend);

                    if (data->next == WM_SEND_QUEUE_END) {
                        queue->tail = (u16)prevIndex;
                    }
                    *prevPointer = data->next;

                    data->next = WM_SEND_QUEUE_END;
                    if (status->sendQueueFreeList.tail == WM_SEND_QUEUE_END) {
                        status->sendQueueFreeList.head = (u16)index;
                    } else {
                        queueData[status->sendQueueFreeList.tail].next = (u16)index;
                    }
                    status->sendQueueFreeList.tail = (u16)index;

                    index = prevIndex;
                }

                if (index != WM_SEND_QUEUE_END) {
                    prevPointer = &(queueData[index].next);
                } else {
                    prevPointer = &queue->head;
                }
                prevIndex = index;
            }
        }
    }

    OS_UnlockMutex(&status->sendQueueMutex);

#ifdef WM_DEBUG
    WMSP_CheckSendQueue("end of CleanSendQueue()");
#endif

    return;
}

void WMSP_ParsePortPacket(u16 aid, u16 wmHeader, u16 *data, u16 length, WMMpRecvBuf *recvBuf)
{
    WMStatus *status = WMSP_GetStatusStructure();
    int restSize = length;
    BOOL firstPacketFlag;

    WMSP_DLOGF_PORT_SENDRECV("<- from %x, len=%d", aid, length);

    if (length == 0) {
        WMSP_DLOGF_PORT_SENDRECV("<- packet %04x: len=%d", wmHeader, length);
        return;
    }

    if (length > 0x204) {
        OS_Warning("too huge packet from aid %d, len=%d, childMaxSize+header=%d+%d", aid, length, status->pparam.childMaxSize, WM_HEADER_CHILD_MAX_SIZE);
        return;
    }

    if (aid == status->aid || aid >= (WM_NUM_MAX_CHILD + 1)) {
        OS_Warning("invalid aid %d ! @ WMSP_ParsePortPacket", aid);
        return;
    }

    if (length & 1 == 1) {
        OS_Warning("odd length %d @ WMSP_ParsePortPacket", length);
        return;
    }

    firstPacketFlag = TRUE;

    while (restSize > 0) {
        int len, packetLength;
        BOOL destBmpFlag, seqNoFlag;
        u16 *header;
        u16 *footer;
        u16 *contents;
        u32 port;
        u32 seqNo = 0xffffU;
#ifdef WM_DEBUG_PORT_SENDRECV
        char str_data[40], str_seq[16], str_dst[20];
#endif

        if (firstPacketFlag == TRUE) {
            firstPacketFlag = FALSE;
            header = &wmHeader;
        } else {
            header = data;
            data++;
            restSize -= WM_HEADER_SIZE;
        }

        len = (*header & WM_HEADER_LENGTH_MASK) * WM_HEADER_LENGTH_SCALE;
        if (len == 0) {
            len = (WM_HEADER_LENGTH_MASK + 1) * WM_HEADER_LENGTH_SCALE;
        }
        destBmpFlag = (*header & WM_HEADER_DEST_BITMAP) ? TRUE : FALSE;
        seqNoFlag = (*header & WM_HEADER_SEQ_FLAG) ? TRUE : FALSE;
        packetLength = len + (seqNoFlag ? 2 : 0) + (destBmpFlag ? 2 : 0);
        restSize -= packetLength;

        if (restSize < 0) {
            OS_Warning("corrupted packet. rest: %d, packet: %d + %d(destBmp) + %d(seqNo)", restSize, len, (destBmpFlag ? 2 : 0), (seqNoFlag ? 2 : 0));
            break;
        }

        port = ((u16)(((*header) & WM_HEADER_PORT_MASK) >> WM_HEADER_PORT_SHIFT));
        contents = data;
        footer = (u16 *)((u32)data + len);

        data = (u16 *)((u32)data + packetLength);

#ifdef WM_DEBUG_PORT_SENDRECV
        WmspDataToString(str_data, sizeof(str_data), contents, (u32)len);
        str_seq[0] = '\0';
        str_dst[0] = '\0';
#endif

        if (seqNoFlag == TRUE) {
            u16 *pCurrSeqNo = &(status->portSeqNo[aid][port & WM_SEQ_PORT_MASK]);
            u32 currSeqNo = *pCurrSeqNo;

            seqNo = *footer;
            footer++;

#ifdef WM_DEBUG_PORT_SENDRECV
            (void)OS_SNPrintf(str_seq, sizeof(str_seq), ", seq=%d", seqNo);
#endif

            if (currSeqNo & 1) {
                *pCurrSeqNo = (u16)(seqNo << 1);
            } else {
                seqNo <<= 1;
                if ((u16)(currSeqNo - seqNo) < (WM_PACKED_PACKET_MAX << 1)) {
                    WM_DPRINTF("received redundunt packet @ WMSP_ParsePortPacket\n");
                    continue;
                } else {
#ifdef WM_DEBUG
                    if ((u16)(currSeqNo + 2) != (u16)seqNo) {
                        WM_DPRINTF("!!! received invalid seqno %d+2 != %d @ WMSP_ParsePortPacket\n",
                            currSeqNo,
                            seqNo);
                    }
#endif
                    *pCurrSeqNo = (u16)seqNo;
                }
            }
        }

        if (destBmpFlag == TRUE) {
            u32 destBitmap;

            destBitmap = *footer;
            footer++;

#ifdef WM_DEBUG_PORT_SENDRECV
            (void)OS_SNPrintf(str_dst, sizeof(str_dst), ", dst=%04x", destBitmap);
#endif

            if (!(destBitmap & (1 << status->aid))) {
                WMSP_DLOGF_PORT_SENDRECV("<- packet %04x: %slen=%d%s%s", *header, str_data, len, str_seq, str_dst);
                continue;
            }
        }

        WMSP_DLOGF_PORT_SENDRECV("<- packet %04x: %slen=%d%s%s", *header, str_data, len, str_seq, str_dst);

        if (len > 0) {
            WMPortRecvCallback *cb_Port;
            cb_Port = (WMPortRecvCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb_Port->apiid = WM_APIID_PORT_RECV;
            cb_Port->errcode = WM_ERRCODE_SUCCESS;
            cb_Port->state = WM_STATECODE_PORT_RECV;
            cb_Port->port = (u16)port;
            cb_Port->recvBuf = recvBuf;
            cb_Port->data = contents;
            cb_Port->length = (u16)len;
            cb_Port->aid = aid;
            cb_Port->myAid = status->aid;
            cb_Port->seqNo = (u16)seqNo;

            {
                u16 parentSize = status->mp_parentSize;
                u16 childSize = status->mp_childSize;
                cb_Port->maxSendDataSize = (status->aid == 0) ? parentSize : childSize;
                cb_Port->maxRecvDataSize = (status->aid == 0) ? childSize : parentSize;
            }

            WMSP_ReturnResult2Wm9(cb_Port);
        } else {
            OS_Warning("received zero length packet. discarded.");
        }
    }
}

void WMSP_CheckSendQueue(char *name)
{
#pragma unused(name)
    WMStatus *status = WMSP_GetStatusStructure();
    WMPortSendQueueData *queueData = status->sendQueueData;
    u16 iQueue, iPrio;
    u16 linkedFlag[WM_SEND_QUEUE_NUM];
    BOOL err = FALSE;

    return;

    OS_LockMutex(&status->sendQueueMutex);

    for (iQueue = 0; iQueue < WM_SEND_QUEUE_NUM; iQueue++) {
        linkedFlag[iQueue] = WM_SEND_QUEUE_END;
    }
    for (iQueue = 0; iQueue < WM_SEND_QUEUE_NUM; iQueue++) {
        if (queueData[iQueue].next != WM_SEND_QUEUE_END) {
            if (queueData[iQueue].next < 0 || WM_SEND_QUEUE_NUM <= queueData[iQueue].next) {
                OS_TPrintf("!!! invalid next value: sendQueueData[%d] = %d at %s\n", iQueue, queueData[iQueue].next, name);
                err = TRUE;
            } else {
                if (linkedFlag[queueData[iQueue].next] != WM_SEND_QUEUE_END) {
                    OS_TPrintf("!!! double link: %d -> %d and %d -> %d\n",
                        linkedFlag[queueData[iQueue].next],
                        queueData[iQueue].next,
                        iQueue,
                        queueData[iQueue].next);
                    err = TRUE;
                }
                linkedFlag[queueData[iQueue].next] = iQueue;
            }
        }
    }

    for (iPrio = 0; iPrio < WM_PRIORITY_LEVEL; iPrio++) {
        if (status->readyQueue[iPrio].head != WM_SEND_QUEUE_END
            && (status->readyQueue[iPrio].head < 0
                || WM_SEND_QUEUE_NUM <= status->readyQueue[iPrio].head)) {
            OS_TPrintf("!!! invalid value: readyQueue[%d].head = %d at %s\n", iPrio, status->readyQueue[iPrio].head, name);
            err = TRUE;
        } else {
            if (status->readyQueue[iPrio].head == WM_SEND_QUEUE_END || linkedFlag[status->readyQueue[iPrio].head] == WM_SEND_QUEUE_END) {
            } else {
                OS_TPrintf("!!! invalid queue head: readyQueue[%d] at %s\n", iPrio, name);
                err = TRUE;
            }
        }

        if (status->readyQueue[iPrio].tail != WM_SEND_QUEUE_END
            && (status->readyQueue[iPrio].tail < 0
                || WM_SEND_QUEUE_NUM <= status->readyQueue[iPrio].tail)) {
            OS_TPrintf("!!! invalid value: readyQueue[%d].tail = %d at %s\n", iPrio, status->readyQueue[iPrio].tail, name);
            err = TRUE;
        } else {
            if (status->readyQueue[iPrio].tail == WM_SEND_QUEUE_END || status->sendQueueData[status->readyQueue[iPrio].tail].next == WM_SEND_QUEUE_END) {
            } else {
                OS_TPrintf("!!! invalid queue tail: readyQueue[%d] at %s\n", iPrio, name);
                err = TRUE;
            }
        }

        if (status->sendQueue[iPrio].head != WM_SEND_QUEUE_END
            && (status->sendQueue[iPrio].head < 0
                || WM_SEND_QUEUE_NUM <= status->sendQueue[iPrio].head)) {
            OS_TPrintf("!!! invalid value: sendQueue[%d].head = %d at %s\n", iPrio, status->sendQueue[iPrio].head, name);
            err = TRUE;
        } else {
            if (status->sendQueue[iPrio].head == WM_SEND_QUEUE_END || linkedFlag[status->sendQueue[iPrio].head] == WM_SEND_QUEUE_END) {
            } else {
                OS_TPrintf("!!! invalid queue head: sendQueue[%d] at %s\n", iPrio, name);
                err = TRUE;
            }
        }

        if (status->sendQueue[iPrio].tail != WM_SEND_QUEUE_END
            && (status->sendQueue[iPrio].tail < 0
                || WM_SEND_QUEUE_NUM <= status->sendQueue[iPrio].tail)) {
            OS_TPrintf("!!! invalid value: sendQueue[%d].tail = %d at %s\n", iPrio, status->sendQueue[iPrio].tail, name);
            err = TRUE;
        } else {
            if (status->sendQueue[iPrio].tail == WM_SEND_QUEUE_END || status->sendQueueData[status->sendQueue[iPrio].tail].next == WM_SEND_QUEUE_END) {
            } else {
                OS_TPrintf("!!! invalid queue tail: sendQueue[%d] at %s\n", iPrio, name);
                err = TRUE;
            }
        }
    }
    if (status->sendQueueFreeList.head != WM_SEND_QUEUE_END
        && (status->sendQueueFreeList.head < 0
            || WM_SEND_QUEUE_NUM <= status->sendQueueFreeList.head)) {
        OS_TPrintf("!!! invalid value: sendQueueFreeList.head = %d at %s\n",
            status->sendQueueFreeList.head,
            name);
        err = TRUE;
    } else {
        if (status->sendQueueFreeList.head == WM_SEND_QUEUE_END || linkedFlag[status->sendQueueFreeList.head] == WM_SEND_QUEUE_END) {
        } else {
            OS_TPrintf("!!! invalid queue head: sendQueueFreeList at %s\n", name);
            err = TRUE;
        }
    }

    if (status->sendQueueFreeList.tail != WM_SEND_QUEUE_END
        && (status->sendQueueFreeList.tail < 0
            || WM_SEND_QUEUE_NUM <= status->sendQueueFreeList.tail)) {
        OS_TPrintf("!!! invalid value: sendQueueFreeList.tail = %d at %s\n",
            status->sendQueueFreeList.tail,
            name);
        err = TRUE;
    } else {
        if (status->sendQueueFreeList.tail == WM_SEND_QUEUE_END || status->sendQueueData[status->sendQueueFreeList.tail].next == WM_SEND_QUEUE_END) {
        } else {
            OS_TPrintf("!!! invalid queue tail: sendQueueFreeList at %s\n", name);
            err = TRUE;
        }
    }

    if (err == TRUE) {
        OS_TWarning("[ERROR] invalid sendQueue");
    }

    OS_UnlockMutex(&status->sendQueueMutex);
}

void WmspDataToString(char *buf, u32 buflen, const u16 *data, u32 size)
{
    if (size >= 16) {
        (void)OS_SNPrintf(buf, buflen, "%04x%04x %04x%04x %04x%04x %04x%04x... ", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    } else if (size >= 8) {
        (void)OS_SNPrintf(buf, buflen, "%04x %04x %04x %04x... ", data[0], data[1], data[2], data[3]);
    } else if (size >= 4) {
        (void)OS_SNPrintf(buf, buflen, "%04x %04x... ", data[0], data[1]);
    } else if (size >= 2) {
        (void)OS_SNPrintf(buf, buflen, "%04x... ", data[0]);
    } else if (buflen > 0) {
        buf[0] = '\0';
    }
}

static u16 WmspGetTmptt(u32 dataLength, u32 txop, u32 pollBitmap, u32 targetVCount)
{
#define WM_MAMP_VCOUNT_MARGIN 2

#define TIME_BYTE_2M        4
#define TIME_PREAMBLE_SHORT 96
#define TIME_SLOT           20
#define TIME_SIFS           10
#define TIME_PIFS           (TIME_SIFS + TIME_SLOT)
#define TIME_DIFS           (TIME_PIFS + TIME_SLOT)
#define TIME_MAX_MP_BACKOFF (TIME_DIFS + 620)
#define TIME_PREAMBLE_SHORT 96
#define MPACK_2M_S          (TIME_SIFS + TIME_PREAMBLE_SHORT + 32 * TIME_BYTE_2M)

    register s32 tmptt;
    u16 vcount;
    u32 pollCnt;
    u32 mp_time;

    if (txop & 0x8000) {
        txop &= 0x7fff;
    } else {
        txop = 96 + ((txop + 24 + 4) * TIME_BYTE_2M) + 6;
    }
    pollCnt = MATH_CountPopulation(pollBitmap);
    mp_time = (u32)(TIME_PREAMBLE_SHORT + (28 + 4 + dataLength + 2) * TIME_BYTE_2M);
    mp_time += TIME_MAX_MP_BACKOFF + txop * pollCnt + MPACK_2M_S;

    vcount = (u16)GX_GetVCount();
    tmptt = (int)(targetVCount - WM_MAMP_VCOUNT_MARGIN - vcount);
    while (tmptt < 0) {
        tmptt += 263;
    }
    tmptt *= 127;

#if defined(SDK_CW) || defined(__MWERKS__)
#pragma optimize_for_size off
#endif
    tmptt /= 20;


    if (tmptt * 10 < mp_time) {
        tmptt = 0;
    }

    return (u16)tmptt;
}

