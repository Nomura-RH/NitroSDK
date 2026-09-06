#include "wmsp_private.h"

#ifdef WM_ALARM_VARIABLE_ON_WRAM
#include <nitro/wram_begin.h>

static OSAlarm wmspMPIntervalAlarm;
static OSAlarm wmspMPAckAlarm;

#include <nitro/wram_end.h>
#endif

static void WmspIndicate(WlCmdReq *req);
static void WmspFreeBufOfWL(WlCmdReq *buf);
static void WmspIndicateFuncDummy(WlCmdReq *req);

static void WmspIndicateMlmeBeaconSend(WlCmdReq *req);
static void WmspIndicateMlmeBeaconRecv(WlCmdReq *req);
static void WmspIndicateMlmeBeaconLost(WlCmdReq *req);
static void WmspIndicateMaMultiPollEnd(WlCmdReq *req);
static void WmspIndicateMaMultiPoll(WlCmdReq *req);
static void WmspIndicateMaMultiPollAck(WlCmdReq *req);
static void WmspIndicateMaData(WlCmdReq *req);
#if SDK_VERSION_WL >= 20500
static void WmspIndicateMaFatalErr(WlCmdReq *req);
#endif
static void WmspIndicateMlmeDisAssociate(WlCmdReq *req);
static void WmspIndicateMlmeReAssociate(WlCmdReq *req);
static void WmspIndicateMlmeAssociate(WlCmdReq *req);
static void WmspIndicateMlmeDeAuthenticate(WlCmdReq *req);
static void WmspIndicateMlmeAuthenticate(WlCmdReq *req);
static void WmspMaMultiPollAckAlarmCallback(void *arg);
static void WmspSetRssi(WlMaMpEndInd *pInd, u16 pollBitmap, BOOL minPollBmpMode);

static void WmspMPParentIntervalAlarmCallback(void *arg);
static void WmspKickMPParent(u16 pollbmp);
static void WmspMPChildIntervalAlarmCallback(void *arg);
static void WmspKickMPChild(void);

static void WmspCancelMPIntervalAlarm(void);
static void WmspCancelMPAckAlarm(void);

static inline BOOL WMSPi_IsChild(u32 stat)
{
    return (stat == WM_STATE_MP_CHILD) || (stat == WM_STATE_CHILD);
}

static inline BOOL WMSPi_IsParent(u32 stat)
{
    return (stat == WM_STATE_MP_PARENT) || (stat == WM_STATE_PARENT);
}

static inline void WmspCancelMPAckAlarm(void)
{
#ifdef WM_ALARM_VARIABLE_ON_WRAM
    OS_CancelAlarm(&wmspMPAckAlarm);
#else
    OS_CancelAlarm(&WMSP_GetSystemWork()->mp_ackAlarm);
#endif
}

static inline void WmspCancelMPIntervalAlarm(void)
{
#ifdef WM_ALARM_VARIABLE_ON_WRAM
    OS_CancelAlarm(&wmspMPIntervalAlarm);
#else
    OS_CancelAlarm(&WMSP_GetSystemWork()->mp_intervalAlarm);
#endif
}

void WMSP_IndicateThread(void *arg)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    OSMessage msg;
    u16 cmd_group;
    u16 cmd_id;

    while (TRUE) {
        (void)OS_ReceiveMessage(&(sys->fromWLmsgQ), &msg, OS_MESSAGE_BLOCK);

        if (msg == 0) {
            OS_ExitThread();
            return;
        }

        cmd_group = (u16)(((WlCmdReq *)msg)->header.code & WL_CMDGCODE_MASK);
        cmd_id = (u16)(((WlCmdReq *)msg)->header.code & WL_CMDSCODE_MASK);

        WMSP_DLOGF_INDICATE_QUEUE("[I]%d,%d", cmd_group, cmd_id);

        switch (cmd_group) {
        case WL_CMDGCODE_MA:
        case WL_CMDGCODE_MLME:
            if (cmd_id & 0x80) {
                WmspIndicate((WlCmdReq *)msg);
                continue;
            }
            break;
        }

        (void)OS_SendMessage(&(sys->confirmQ), msg, OS_MESSAGE_BLOCK);
    }
}

static void WmspIndicate(WlCmdReq *req)
{
    if (NULL == WMSP_GetSystemWork()->wm7buf) {
        return;
    }
    if (WM_STATE_STOP == WMSP_GetStatusStructure()->state) {
        return;
    }

    switch (req->header.code) {
    case WL_CMDCODE_MLME_AUTH_IND:
        WmspIndicateMlmeAuthenticate(req);
        break;
    case WL_CMDCODE_MLME_DEAUTH_IND:
        WmspIndicateMlmeDeAuthenticate(req);
        break;
    case WL_CMDCODE_MLME_ASS_IND:
        WmspIndicateMlmeAssociate(req);
        break;
    case WL_CMDCODE_MLME_REASS_IND:
        WmspIndicateMlmeReAssociate(req);
        break;
    case WL_CMDCODE_MLME_DISASS_IND:
        WmspIndicateMlmeDisAssociate(req);
        break;
    case WL_CMDCODE_MLME_BCLOST_IND:
        WmspIndicateMlmeBeaconLost(req);
        break;
    case WL_CMDCODE_MLME_BCSEND_IND:
        WmspIndicateMlmeBeaconSend(req);
        break;
    case WL_CMDCODE_MLME_BCRECV_IND:
        WmspIndicateMlmeBeaconRecv(req);
        break;
    case WL_CMDCODE_MA_DATA_IND:
        WmspIndicateMaData(req);
        break;
    case WL_CMDCODE_MA_MP_IND:
        WmspIndicateMaMultiPoll(req);
        break;
    case WL_CMDCODE_MA_MPEND_IND:
        WmspIndicateMaMultiPollEnd(req);
        break;
    case WL_CMDCODE_MA_MPACK_IND:
        WmspIndicateMaMultiPollAck(req);
        break;
    case WL_CMDCODE_MA_FATAL_ERR_IND:
        WmspIndicateMaFatalErr(req);
        break;
    default:
        WmspIndicateFuncDummy(req);
    }

    WmspFreeBufOfWL(req);
}

static void WmspFreeBufOfWL(WlCmdReq *buf)
{
    OSIntrMode enabled;
    WMSPWork *sys = WMSP_GetSystemWork();

    enabled = OS_DisableInterrupts();
    OS_FreeToHeap(sys->arenaId, sys->heapHandle, buf);
    WM_ASSERT(-1 != OS_CheckHeap(sys->arenaId, sys->heapHandle));
    (void)OS_RestoreInterrupts(enabled);
}

static void WmspIndicateFuncDummy(WlCmdReq *req)
{
    OS_TPrintf("ARM7: Dummy indication function is called.\n");
    return;
}

static void WmspIndicateMlmeBeaconSend(WlCmdReq *req)
{
    if (WMSP_GetStatusStructure()->beaconIndicateFlag > 0) {
        WMStartParentCallback *callback = (WMStartParentCallback *)WMSP_GetBuffer4Callback2Wm9();

        callback->apiid = WM_APIID_START_PARENT;
        callback->errcode = WM_ERRCODE_SUCCESS;
        callback->state = WM_STATECODE_BEACON_SENT;
        WMSP_ReturnResult2Wm9(callback);
    }
}

static void WmspIndicateMlmeBeaconRecv(WlCmdReq *req)
{
    WMSPWork *const work = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    WlMlmeBeaconRecvInd *pInd = (WlMlmeBeaconRecvInd *)req;
    WMGameInfo *pGameInfo = (WMGameInfo *)&(pInd->gameInfo);
    u32 *buf;

    WMSP_AddRssiToRandomPool(WMSP_GetRssi8(pInd->rssi));

    if ((WM_STATE_CHILD != status->state) && (WM_STATE_MP_CHILD != status->state)) {
        return;
    }

    if (status->curr_tgid != pGameInfo->tgid) {
        BOOL result;

        buf = WMSP_GetInternalRequestBuf();
        if (buf == NULL) {
            result = FALSE;
        } else {
            buf[0] = WM_APIID_AUTO_DISCONNECT;
            buf[1] = 0x0001;
            buf[2] = WM_DISCONNECT_REASON_TGID_CHANGED;

            WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

            result = OS_SendMessage(&(WMSP_GetSystemWork()->requestQ),
                (OSMessage)buf,
                OS_MESSAGE_NOBLOCK);
        }

        if (!result) {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->state = WM_STATECODE_FIFO_ERROR;
            cb->reason = WM_APIID_AUTO_DISCONNECT;
            WMSP_ReturnResult2Wm9(cb);
        }
        return;
    } else {
        if (WMSP_GetStatusStructure()->beaconIndicateFlag > 0) {
            WMBeaconRecvIndCallback *callback = (WMBeaconRecvIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            callback->apiid = WM_APIID_INDICATION;
            callback->errcode = WM_ERRCODE_SUCCESS;
            callback->state = WM_STATECODE_BEACON_RECV;
            callback->tgid = pGameInfo->tgid;
            callback->wmstate = status->state;
            callback->gameInfoLength = pInd->gameInfoLength;

            if (callback->gameInfoLength <= 128) {
                MI_CpuCopy16(pGameInfo,
                    &(callback->gameInfo),
                    ((callback->gameInfoLength + 1U) & ~1U));
            }
            WMSP_ReturnResult2Wm9(callback);
        }
    }
}

static void WmspIndicateMlmeBeaconLost(WlCmdReq *req)
{
#pragma unused(req)

    if (WMSP_GetStatusStructure()->beaconIndicateFlag > 0) {
        WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();

        callback->apiid = WM_APIID_START_CONNECT;
        callback->errcode = WM_ERRCODE_SUCCESS;
        callback->state = WM_STATECODE_BEACON_LOST;
        WMSP_ReturnResult2Wm9(callback);
    }
}

static void WmspIndicateMaMultiPollEnd(WlCmdReq *req)
{

    u16 i;
    WMStatus *status = WMSP_GetStatusStructure();
    WMStartMPCallback *callback;

    WlMaMpEndInd *pInd = (WlMaMpEndInd *)req;

    WMMpRecvHeader *bufp;
    WMMpRecvData *datap;
    OSIntrMode enabled;
    BOOL kick;
    BOOL retryFlag = FALSE;
    u32 size;
    OSTick now;
    u16 errBitmap;
    u32 recvErrorBitmap;

    if (status->mp_flag == FALSE) {
        return;
    }

    WMSP_DLOGF_INDICATES("MPEND.ind P:%04x C:%d", pInd->mpKey.bitmap, pInd->mpKey.count);

#if SDK_VERSION_WL >= 20500
    errBitmap = pInd->mpKey.errBitmap;
#if 0
    {
#if 0
        static MATHRandContext32 rand = {
            12345,
            0x5d588b656c078965ULL,
            2531011
        };
        if (MATH_Rand32(&rand, 100) < 1)
        {
            WM_DPRINTF("Generate Fatal Error!\n");
            errBitmap = status->child_bitmap;
        }
#else
        static u16 prev_pad = 0;
        u16     pad = PAD_Read();
        if ((pad & ~prev_pad) & PAD_BUTTON_L)
        {
            WM_DPRINTF("Generate Fatal Error!\n");
            errBitmap = status->child_bitmap;
        }
#endif
        prev_pad = pad;
    }
#endif
    if (errBitmap != 0 || (status->mp_resumeFlag == TRUE && pInd->mpKey.bitmap != 0)) {
        if (errBitmap != 0) {
            WM_DPRINTF("[ARM7]MPEnd.Ind ErrorBitmap = 0x%04x\n", errBitmap);
#ifdef WM_DEBUG_HEAP
            WMSP_CheckWLHeap("MPEnd.Ind");
#endif
        }

        WMSP_RequestResumeMP();

        return;
    }

    if (status->mp_resumeFlag != FALSE) {
        status->mp_resumeFlag = FALSE;
    }
#endif

    bufp = (WMMpRecvHeader *)status->mp_recvBuf[status->mp_recvBufSel];

    size = pInd->mpKey.length * pInd->mpKey.count + sizeof(WMMpRecvHeader) - sizeof(WMMpRecvData);
    if (status->mp_recvBufSize < size) {
        OS_Warning("receive buffer size is less than received data. %d < %d * %d + %d",
            status->mp_recvBufSize,
            pInd->mpKey.length,
            pInd->mpKey.count,
            sizeof(WMMpRecvHeader) - sizeof(WMMpRecvData));
        size = status->mp_recvBufSize;
    }
#if 0
    WM_DPRINTF("%d: %d >= %d * %d + %d\n", size, status->mp_recvBufSize, pInd->mpKey.length,
               pInd->mpKey.count, sizeof(WMMpRecvHeader) - sizeof(WMMpRecvData));
#endif
    MI_CpuCopy8(&(pInd->mpKey),
        bufp,
        size
    );

    WmspSetRssi(pInd, bufp->bitmap, status->mp_minPollBmpMode);
#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
#else
    status->linkLevel = WMSP_GetAverageLinkLevel();
#endif

    now = OS_GetTick() | 1;
    datap = bufp->data;
    recvErrorBitmap = bufp->bitmap;
    for (i = 0; i < bufp->count; ++i, datap = (WMMpRecvData *)(((u32)datap) + bufp->length)) {
        u16 aid = datap->aid;
        u16 length = datap->length;

        if (aid < 1 || 15 < aid) {
#ifdef WM_DEBUG
            OS_Warning("Internal Error: illegal aid %d, len=%d", aid, length);
#endif
            continue;
        }
        if (length >= 2 && length != 0xffff) {
            datap->length -= 2;
            length -= 2;

            status->mp_readyBitmap |= (1 << aid);
            status->mp_lastRecvTick[aid] = now;

            if (length != 0) {
                WMSP_ParsePortPacket(aid, datap->wmHeader, datap->cdata, length, (WMMpRecvBuf *)bufp);
            }
        } else if (length == 0) {
            OSTick last = status->mp_lastRecvTick[aid];
            recvErrorBitmap |= (1 << aid);
            if (status->mp_lifeTimeTick != 0ULL && last != 0ULL
                && (now - last) > status->mp_lifeTimeTick) {
                u32 *buf = WMSP_GetInternalRequestBuf();
                BOOL result;

                WM_DPRINTF("NullAck TimeOut: aid=%d\n", aid);
                status->mp_lastRecvTick[aid] = 0ULL;

                if (buf == NULL) {
                    result = FALSE;
                } else {
                    buf[0] = WM_APIID_AUTO_DISCONNECT;
                    buf[1] = (u32)(1 << aid);
                    buf[2] = WM_DISCONNECT_REASON_MP_LIFETIME;

                    WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

                    result = OS_SendMessage(&(WMSP_GetSystemWork()->requestQ),
                        (OSMessage)buf,
                        OS_MESSAGE_NOBLOCK);
                }

                if (!result) {
                    WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

                    cb->apiid = WM_APIID_INDICATION;
                    cb->errcode = WM_ERRCODE_FIFO_ERROR;
                    cb->state = WM_STATECODE_FIFO_ERROR;
                    cb->reason = WM_APIID_AUTO_DISCONNECT;
                    WMSP_ReturnResult2Wm9(cb);
                }
            }
        }
    }

#ifdef WMSP_MP_DETECT_FATAL_ERROR_STRICTLY
    (void)WMSP_FlushSendQueue(FALSE, (u16)recvErrorBitmap);
#else
    (void)WMSP_FlushSendQueue(FALSE, bufp->bitmap);
#endif
    if (bufp->bitmap != 0) {
        retryFlag = TRUE;
    }

    callback = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_START_MP;
    callback->errcode = WM_ERRCODE_SUCCESS;
    callback->state = WM_STATECODE_MPEND_IND;
    callback->recvBuf = (WMMpRecvBuf *)bufp;

    WMSP_ReturnResult2Wm9(callback);

#ifdef WMSP_MP_WAIT_FOR_ARM9
    WMSP_Wait4Wm9();
#endif

    status->mp_recvBufSel ^= 1;

    enabled = OS_DisableInterrupts();
    if (retryFlag == FALSE) {
        status->mp_count--;
    }
    if (status->mp_limitCount > 0) {
        status->mp_limitCount--;
    }
    kick = (status->mp_count > 0 && status->mp_limitCount > 0);
    (void)OS_RestoreInterrupts(enabled);

    if (kick) {
        u16 pollbmp;

        if (retryFlag == TRUE) {
            pollbmp = pInd->mpKey.bitmap;
        } else {
            pollbmp = 0xffff;
        }

        if (status->mp_parentInterval != 0) {
            WmspCancelMPIntervalAlarm();
            OS_SetAlarm(
#ifdef WM_ALARM_VARIABLE_ON_WRAM
                &wmspMPIntervalAlarm,
#else
                &sys->mp_intervalAlarm,
#endif
                status->mp_parentIntervalTick,
                WmspMPParentIntervalAlarmCallback,
                (void *)pollbmp);
        } else {
            WmspKickMPParent(pollbmp);
        }
    }

    WMSP_DLOG_INDICATES("MPEND.ind done");
}

void WMSP_RequestResumeMP(void)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u32 *buf = WMSP_GetInternalRequestBuf();
    BOOL result;

    if (buf == NULL) {
        result = FALSE;
    } else {
        buf[0] = WM_APIID_KICK_MP_RESUME;
        buf[1] = status->mp_prevPollBitmap;

        WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

        result = OS_SendMessage(&(WMSP_GetSystemWork()->requestQ),
            (OSMessage)buf,
            OS_MESSAGE_NOBLOCK);
    }

    if (!result) {
        WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_INDICATION;
        cb->errcode = WM_ERRCODE_FIFO_ERROR;
        cb->state = WM_STATECODE_FIFO_ERROR;
        cb->reason = WM_APIID_KICK_MP_RESUME;
        WMSP_ReturnResult2Wm9(cb);
    } else {
        status->mp_resumeFlag = TRUE;
    }

    return;
}

static void WmspMPParentIntervalAlarmCallback(void *arg)
{
    u32 pollbmp = (u32)arg;
    WmspKickMPParent((u16)pollbmp);
}

static void WmspKickMPParent(u16 pollbmp)
{
    u32 *buf = WMSP_GetInternalRequestBuf();
    WMSPWork *sys = WMSP_GetSystemWork();
    int result;

    if (buf == NULL) {
        result = FALSE;
    } else {
        buf[0] = WM_APIID_KICK_MP_PARENT;
        buf[1] = (u32)pollbmp;

        WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

        result = OS_SendMessage(&(sys->requestQ), (OSMessage)buf, OS_MESSAGE_NOBLOCK);
    }

    if (!result) {
        if (sys->wm7buf != NULL) {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->state = WM_STATECODE_FIFO_ERROR;
            cb->reason = WM_APIID_KICK_MP_PARENT;
            WMSP_ReturnResult2Wm9(cb);
        }
    }
}

static void WmspSetRssi(WlMaMpEndInd *pInd, u16 pollBitmap, BOOL minPollBmpMode)
{
#pragma unused(minPollBmpMode)
    WlMpKeyData *kd;
    s32 i;
    u16 temp;
    u16 min;
    WMStatus *status = WMSP_GetStatusStructure();

#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
    min = status->minRssi;
#else
    min = 0xff;
#endif

    if (pollBitmap != 0) {
#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
#ifdef WMSP_LINK_LEVEL_TREAT_ABSENCE_AS_ERROR
        return;
#else
        if (WMSP_RSSI_ERROR < min) {
            min = WMSP_RSSI_ERROR;
        }
#endif
#else
        min = WMSP_RSSI_ERROR;
#endif
    } else if (pInd->mpKey.count < 1) {
        return;
    } else {
        for (i = 0; i < pInd->mpKey.count; i++) {
            kd = (WlMpKeyData *)((u32)pInd->mpKey.data + (pInd->mpKey.length * i));
            temp = WMSP_GetRssi8(kd->rssi);
            if (temp < min) {
                min = temp;
            }
        }
    }

#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
    status->minRssi = min;
#else
    WMSP_AddRssiToList((u8)min);
#endif
}

static void WmspIndicateMaMultiPoll(WlCmdReq *req)
{
    WMStartMPCallback *callback;
    WlMaMpInd *pInd = (WlMaMpInd *)req;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    WMMpRecvBuf *bufp;
    OSIntrMode enabled;
    BOOL timeout;
    u16 oldSentFlag, oldEmptyFlag;
    BOOL polled;
    u32 size;
    u16 txKeySts;

#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
    {
        u16 rssi8;
        rssi8 = WMSP_GetRssi8(pInd->frame.rssi);

        if (status->minRssi > rssi8) {
            status->minRssi = rssi8;
        }
    }
#else
    WMSP_AddRssiToList(WMSP_GetRssi8(pInd->frame.rssi));
    status->linkLevel = WMSP_GetAverageLinkLevel();
#endif

    if (status->mp_flag == FALSE) {
        return;
    }

    WMSP_DLOGF_INDICATES("MP.ind P:%d I:%d O:%d K:%d",
        (pInd->frame.txKeySts & RXSTS_POLLED_MP) != 0,
        (pInd->frame.txKeySts & RXSTS_KEY_IN) != 0,
        (pInd->frame.txKeySts & RXSTS_KEY_OUT) != 0,
        (pInd->frame.txKeySts & RXSTS_TX_KEY) != 0);

    if (status->mp_vsyncFlag == TRUE) {
        WM_DPRINTF("mp_vsyncFlag == TRUE, but received MP.ind before vsync.\n");
        status->mp_vsyncFlag = FALSE;
    }

    oldEmptyFlag = status->mp_bufferEmptyFlag;
    oldSentFlag = status->mp_sentDataFlag;

    status->mp_recvBufSel ^= 1;
    bufp = status->mp_recvBuf[status->mp_recvBufSel];

    size = pInd->frame.length + sizeof(WMMpRecvBuf) - 6U;
    if (status->mp_recvBufSize < size) {
        OS_Warning("receive buffer size is less than received data. %d < %d + %d",
            status->mp_recvBufSize,
            pInd->frame.length,
            sizeof(WMMpRecvBuf) - 6);
        size = status->mp_recvBufSize;
    }
    MI_CpuCopy8(&(pInd->frame),
        bufp,
        size
    );

    enabled = OS_DisableInterrupts();
    timeout = FALSE;

    if (status->mp_waitAckFlag == TRUE) {
        timeout = TRUE;
        WmspCancelMPAckAlarm();
        WM_DPRINTF("received MP.ind before receiving MPACK.ind.\n");
    }
    status->mp_waitAckFlag = TRUE;
    status->mp_ackTime = bufp->ackTimeStamp;
    txKeySts = pInd->frame.txKeySts;
    polled = ((txKeySts & RXSTS_POLLED_MP) ? TRUE : FALSE);
    status->mp_isPolledFlag = (u16)polled;
    OS_SetAlarm(
#ifdef WM_ALARM_VARIABLE_ON_WRAM
        &wmspMPAckAlarm,
#else
        &sys->mp_ackAlarm,
#endif
        OS_MilliSecondsToTicks(((u16)(bufp->ackTimeStamp - bufp->timeStamp) + 128) << 4) / 1024,
        WmspMaMultiPollAckAlarmCallback,
        NULL);

    {
        BOOL sent, empty;

        empty = ((txKeySts & (RXSTS_POLLED_MP | RXSTS_KEY_OUT)) == (RXSTS_POLLED_MP | RXSTS_KEY_OUT));
        sent = ((txKeySts & (RXSTS_POLLED_MP | RXSTS_TX_KEY)) == (RXSTS_POLLED_MP | RXSTS_TX_KEY));

        if (sent) {
            status->mp_setDataFlag = FALSE;
        }

        status->mp_bufferEmptyFlag = (u16)((empty) ? TRUE : FALSE);
        status->mp_sentDataFlag = (u16)((sent) ? TRUE : FALSE);
    }

    if (polled) {
        s32 childSize;

        childSize = ((((s32)bufp->txop - 96 - 6) / 4) - 24 - 4 - WM_HEADER_SIZE - WM_HEADER_CHILD_MAX_SIZE);
        if (childSize >= 0) {
            if (childSize > 0x200) {
                childSize = 0x200;
            }
            if (childSize != status->mp_childSize) {
                WMSP_DLOGF_PORT_SENDRECV("childSize: %d -> %d, txop: %04x", status->mp_childSize, childSize, bufp->txop);
                WMSP_SetChildSize((u16)childSize);
            }
        }
    }

    (void)OS_RestoreInterrupts(enabled);

    if (timeout) {
        if (oldEmptyFlag == TRUE) {
            (void)WMSP_FlushSendQueue(timeout, 0x0000);
        }

        callback = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_MP;
        callback->errcode = WM_ERRCODE_TIMEOUT;
        callback->state = WM_STATECODE_MPACK_IND;
        callback->recvBuf = NULL;

        WMSP_ReturnResult2Wm9(callback);

    }

    if (!polled) {
        if (bufp->length >= 2) {
            status->mp_vsyncOrderedFlag = ((bufp->wmHeader & WM_HEADER_VSYNC) != 0);
        }
        WMSP_DLOG_INDICATES("MP.ind done (not polled)");
        return;
    }

    MI_CpuCopy8(pInd->frame.destAdrs, bufp->destAdrs, WM_SIZE_MACADDR);
    MI_CpuCopy8(pInd->frame.srcAdrs, bufp->srcAdrs, WM_SIZE_MACADDR);

    if (bufp->length >= 2) {

        bufp->length -= 2;

        status->mp_vsyncOrderedFlag = ((bufp->wmHeader & WM_HEADER_VSYNC) != 0);

        callback = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_MP;
        callback->errcode = WM_ERRCODE_SUCCESS;
        callback->state = WM_STATECODE_MP_IND;
        callback->recvBuf = bufp;

        WMSP_ReturnResult2Wm9(callback);

        if (bufp->length > 0) {
            WMSP_ParsePortPacket(0, bufp->wmHeader, bufp->data, bufp->length, bufp);
#ifdef WMSP_MP_WAIT_FOR_ARM9
            WMSP_Wait4Wm9();
#endif
        }
    } else {

        bufp->length = 0;

        status->mp_vsyncOrderedFlag = FALSE;

        callback = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_MP;
        callback->errcode = WM_ERRCODE_NO_DATA;
        callback->state = WM_STATECODE_MP_IND;
        callback->recvBuf = bufp;

        WMSP_ReturnResult2Wm9(callback);
#ifdef WMSP_MP_WAIT_FOR_ARM9
        WMSP_Wait4Wm9();
#endif
    }

    if (status->mp_lifeTimeTick != 0ULL) {
        status->mp_lastRecvTick[0] = OS_GetTick() | 1;
    }

    WMSP_DLOG_INDICATES("MP.ind done");
}

static void WmspMaMultiPollAckAlarmCallback(void *arg)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    WlMaMpAckInd *pInd;
    BOOL result;

    WMSP_DLOG_INDICATES("MPACK.TO");

    pInd = (WlMaMpAckInd *)OS_AllocFromHeap(sys->arenaId, sys->heapHandle, sizeof(WlMaMpAckInd));
    WM_ASSERT(-1 != OS_CheckHeap(sys->arenaId, sys->heapHandle));

    pInd->header.code = WL_CMDCODE_MA_MPACK_IND;
    pInd->header.length = 0;

    result = OS_SendMessage(&(sys->fromWLmsgQ), (OSMessage)pInd, OS_MESSAGE_NOBLOCK);

    if (!result) {
        WmspFreeBufOfWL((WlCmdReq *)pInd);
        if (sys->wm7buf != NULL) {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->state = WM_STATECODE_FIFO_ERROR;
            cb->reason = WM_APIID_INDICATION;
            WMSP_ReturnResult2Wm9(cb);
        }
    }
}

static void WmspIndicateMaMultiPollAck(WlCmdReq *req)
{
    WMstartMPCallback *callback;
    WlMaMpAckInd *pInd = (WlMaMpAckInd *)req;
    WlRxMpAckFrame *pFrame = (WlRxMpAckFrame *)&(pInd->ack);
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    OSIntrMode enabled;
    BOOL failed;
    BOOL timeout;
    BOOL retryFlag = FALSE;
    BOOL sent, empty;
    BOOL polled;
    u16 timeStamp;

    if (status->mp_flag == FALSE) {
        return;
    }

    if (pInd->header.length == 0) {
        u16 lower0, lower1;
        u16 higher;

        timeout = TRUE;

        lower0 = *(vu16 *)MREG_TSF0;
        higher = *(vu16 *)MREG_TSF1;
        lower1 = *(vu16 *)MREG_TSF0;
        if (lower0 > lower1) {
            higher = *(vu16 *)MREG_TSF1;
        }
        timeStamp = (u16)((higher << (16 - 4)) | (lower1 >> 4));

        if (!((s16)(timeStamp - status->mp_ackTime) > 0)) {
            WM_DPRINTF("timeout callback is called but next mp is going.\n");
            return;
        }
    } else {
        timeout = FALSE;
    }

    enabled = OS_DisableInterrupts();
    if (status->mp_waitAckFlag == FALSE) {
        (void)OS_RestoreInterrupts(enabled);
        WMSP_DLOGF("received single MPACK.ind I:%d O:%d\n",
            (pFrame->txKeySts & RXSTS_KEY_IN) != 0,
            (pFrame->txKeySts & RXSTS_KEY_OUT) != 0);
        return;
    }
    status->mp_waitAckFlag = FALSE;
    polled = status->mp_isPolledFlag;
    WmspCancelMPAckAlarm();
    (void)OS_RestoreInterrupts(enabled);

    sent = status->mp_sentDataFlag;

    failed = !sent || (!timeout && (pFrame->bitmap & (1 << status->aid)));

#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
#ifdef WMSP_LINK_LEVEL_TREAT_ABSENCE_AS_ERROR
#else
    if (polled && (timeout || failed)) {
        status->minRssi = WMSP_RSSI_ERROR;
    }
#endif
#endif

    if (status->mp_sentDataFlag) {
        status->mp_sentDataFlag = FALSE;
    }
    empty = status->mp_bufferEmptyFlag;
    if (empty) {
        status->mp_bufferEmptyFlag = FALSE;
        retryFlag = WMSP_FlushSendQueue(timeout, (u16)((failed) ? 0x0001 : 0x0000));
    }

    WMSP_DLOGF_INDICATES("MPACK.ind TO:%d F:%d S:%d E:%d I:%d V:%d R:%d", timeout, failed, sent, empty, (pFrame->txKeySts & RXSTS_KEY_IN) != 0, status->mp_vsyncOrderedFlag, retryFlag);

    if (!polled) {
        WMSP_DLOG_INDICATES("MPACK.ind done (not polled)");
        return;
    }

    callback = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_START_MP;
    if (timeout) {
        callback->errcode = WM_ERRCODE_TIMEOUT;
    } else if (pFrame->bitmap & (1 << status->aid)) {
        callback->errcode = WM_ERRCODE_SEND_FAILED;
    } else {
        callback->errcode = WM_ERRCODE_SUCCESS;
    }
    callback->state = WM_STATECODE_MPACK_IND;
    callback->recvBuf = NULL;

    if (!timeout) {
        callback->timeStamp = pFrame->timeStamp;
        callback->rate_rssi = *(u16 *)&(pFrame->rate);
        MI_CpuCopy8(pFrame->destAdrs, callback->destAdrs, WM_SIZE_MACADDR);
        MI_CpuCopy8(pFrame->srcAdrs, callback->srcAdrs, WM_SIZE_MACADDR);
        callback->seqNum = pFrame->seqCtrl;
        callback->tmptt = pFrame->tmptt;
        callback->pollbmp = pFrame->bitmap;
    }

    WMSP_ReturnResult2Wm9(callback);
#ifdef WMSP_MP_WAIT_FOR_ARM9
    WMSP_Wait4Wm9();
#endif

    WMSP_DLOG_INDICATES("MPACK.ind done");

    if (polled) {
        if (retryFlag == TRUE || status->mp_vsyncOrderedFlag == FALSE) {
            if (status->mp_childInterval != 0) {
                WmspCancelMPIntervalAlarm();
                OS_SetAlarm(
#ifdef WM_ALARM_VARIABLE_ON_WRAM
                    &wmspMPIntervalAlarm,
#else
                    &sys->mp_intervalAlarm,
#endif
                    status->mp_childIntervalTick,
                    WmspMPChildIntervalAlarmCallback,
                    NULL);
            } else {
                WmspKickMPChild();
            }
        } else {
            status->mp_vsyncOrderedFlag = FALSE;
            status->mp_vsyncFlag = TRUE;
            status->mp_newFrameFlag = FALSE;
        }
    }
}

static void WmspMPChildIntervalAlarmCallback(void *arg)
{
#pragma unused(arg)
    WmspKickMPChild();
}

static void WmspKickMPChild(void)
{
    u32 *buf = WMSP_GetInternalRequestBuf();
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    int result;

    status->mp_vsyncOrderedFlag = FALSE;
    status->mp_vsyncFlag = FALSE;
    status->mp_newFrameFlag = FALSE;

    if (buf == NULL) {
        result = FALSE;
    } else {
        buf[0] = WM_APIID_KICK_MP_CHILD;

        WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

        result = OS_SendMessage(&(sys->requestQ), (OSMessage)buf, OS_MESSAGE_NOBLOCK);
    }

    if (!result) {
        if (sys->wm7buf != NULL) {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->state = WM_STATECODE_FIFO_ERROR;
            cb->reason = WM_APIID_KICK_MP_CHILD;
            WMSP_ReturnResult2Wm9(cb);
        }
    }
}

static void WmspIndicateMaData(WlCmdReq *req)
{
    WMStartDCFCallback *callback;
    WlMaDataInd *pInd = (WlMaDataInd *)req;
    WMStatus *status = WMSP_GetStatusStructure();

    WlRxFrame *pFrame;
    WMDcfRecvBuf *bufp;

    if (status->dcf_flag == FALSE) {
        return;
    }

    {
        WMSPWork *pw = WMSP_GetSystemWork();

        WMSP_AddRssiToList(WMSP_GetRssi8(pInd->frame.rssi));
        status->linkLevel = WMSP_GetAverageLinkLevel();
    }

    pFrame = (WlRxFrame *)&pInd->frame;

#ifdef WM_DEBUG_DUMP_RECV_MA_DATA
    OS_TPrintf("[WM7:recv] id: 0x%04x rate: 0x%02x rssi: 0x%02x len: 0x%04x\n",
        pFrame->frameId,
        pFrame->rate,
        pFrame->rssi,
        pFrame->length);
    OS_TPrintf("  dest: %02x%02x%02x%02x%02x%02x src %02x%02x%02x%02x%02x%02x\n",
        ((u8 *)pFrame->destAdrs)[0],
        ((u8 *)pFrame->destAdrs)[1],
        ((u8 *)pFrame->destAdrs)[2],
        ((u8 *)pFrame->destAdrs)[3],
        ((u8 *)pFrame->destAdrs)[4],
        ((u8 *)pFrame->destAdrs)[5],
        ((u8 *)pFrame->srcAdrs)[0],
        ((u8 *)pFrame->srcAdrs)[1],
        ((u8 *)pFrame->srcAdrs)[2],
        ((u8 *)pFrame->srcAdrs)[3],
        ((u8 *)pFrame->srcAdrs)[4],
        ((u8 *)pFrame->srcAdrs)[5]);
    {
        int i;
        for (i = 0; (i < pFrame->length) && (i < 64); i++) {
            OS_TPrintf(" %02x", pFrame->data[i]);
            if ((i % 16) == 7) {
                OS_TPrintf(" -");
            }
            if ((i % 16) == 15) {
                OS_TPrintf("\n");
            }
        }
        if (pFrame->length > 64) {
            OS_TPrintf(" %02x...", pFrame->data[i]);
        }
        OS_TPrintf("\n");
    }
#endif

    if (TRUE == WMSP_CheckMacAddress((u8 *)(pFrame->srcAdrs))) {
        return;
    }

    if (pFrame->length > WM_DCF_MAX_SIZE) {
        return;
    }

    status->dcf_recvBufSel ^= 1;
    bufp = status->dcf_recvBuf[status->dcf_recvBufSel];

    MI_CpuCopy8(pFrame,
        bufp,
        (pFrame->length + WM_SIZE_MADATA_HEADER + 1U) & ~1U
    );

    MI_CpuCopy8(pFrame->destAdrs, bufp->destAdrs, WM_SIZE_MACADDR);
    MI_CpuCopy8(pFrame->srcAdrs, bufp->srcAdrs, WM_SIZE_MACADDR);

    callback = (WMStartDCFCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_START_DCF;
    callback->errcode = WM_ERRCODE_SUCCESS;
    callback->state = WM_STATECODE_DCF_IND;
    callback->recvBuf = bufp;

    WMSP_ReturnResult2Wm9(callback);
}

static void WmspIndicateMaFatalErr(WlCmdReq *req)
{
    WlMaFatalErrInd *pInd = (WlMaFatalErrInd *)req;
    u32 *buf;
    BOOL result;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    OS_TPrintf("[ARM7] MaFatalErr.Indicate!! errCode:0x%04x\n", pInd->errCode);

    if (status->mp_current_ignoreFatalErrorMode == 1 && pInd->errCode == 0x20) {
        OS_TPrintf("    but ignored\n");
        {
            OSIntrMode enabled;

            enabled = OS_DisableInterrupts();
            WmspCancelMPAckAlarm();
            status->mp_waitAckFlag = FALSE;
            (void)OS_RestoreInterrupts(enabled);

            status->mp_setDataFlag = FALSE;
            (void)WMSP_FlushSendQueue(TRUE, 0x0000);
        }
        {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_SUCCESS;
            cb->state = WM_STATECODE_INFORMATION;
            cb->reason = WM_INFOCODE_FATAL_ERROR;
            WMSP_ReturnResult2Wm9(cb);
        }
    } else {
        buf = WMSP_GetInternalRequestBuf();
        if (buf == NULL) {
            result = FALSE;
        } else {
            buf[0] = WM_APIID_AUTO_DISCONNECT;
            buf[2] = WM_DISCONNECT_REASON_FATAL_ERROR;

            if (WMSPi_IsParent(status->state)) {
                buf[1] = 0x7ffe;
            } else if (WMSPi_IsChild(status->state)) {
                buf[1] = 0x0001;
            }

            WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

            result = OS_SendMessage(&(WMSP_GetSystemWork()->requestQ),
                (OSMessage)buf,
                OS_MESSAGE_NOBLOCK);
        }

        if (!result) {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->state = WM_STATECODE_FIFO_ERROR;
            cb->reason = WM_APIID_AUTO_DISCONNECT;
            WMSP_ReturnResult2Wm9(cb);
        }
    }
}

static void WmspIndicateMlmeDisAssociate(WlCmdReq *req)
{
#pragma unused(req)

    WMIndCallback *callback = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_INDICATION;
    callback->errcode = WM_ERRCODE_SUCCESS;
    callback->state = WM_STATECODE_DISASSOCIATE;
    WMSP_ReturnResult2Wm9(callback);
}

static void WmspIndicateMlmeReAssociate(WlCmdReq *req)
{
#pragma unused(req)

    WMIndCallback *callback = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_INDICATION;
    callback->errcode = WM_ERRCODE_SUCCESS;
    callback->state = WM_STATECODE_REASSOCIATE;
    WMSP_ReturnResult2Wm9(callback);
}

static void WmspIndicateMlmeAssociate(WlCmdReq *req)
{
    WlMlmeAssInd *pInd = (WlMlmeAssInd *)req;
    WMStatus *status = WMSP_GetStatusStructure();
    WMStartParentCallback *callback;
    u32 *buf;
    u32 aid;

    aid = pInd->aid;
    if (aid == 0 || aid >= (WM_NUM_MAX_CHILD + 1)) {
        OS_Warning("invalid aid %d !", aid);
        return;
    }

    if (!status->pparam.entryFlag) {
        BOOL result;

        buf = WMSP_GetInternalRequestBuf();
        if (buf == NULL) {
            result = FALSE;
        } else {
            buf[0] = WM_APIID_AUTO_DEAUTH;
            MI_CpuCopy8(pInd->peerMacAdrs, &buf[1], 6);

            WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

            result = OS_SendMessage(&(WMSP_GetSystemWork()->requestQ),
                (OSMessage)buf,
                OS_MESSAGE_NOBLOCK);
        }
        if (!result) {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->state = WM_STATECODE_FIFO_ERROR;
            cb->reason = WM_APIID_AUTO_DEAUTH;
            WMSP_ReturnResult2Wm9(cb);
        }
        return;
    }

    {
        OSIntrMode e = OS_DisableInterrupts();
        status->child_bitmap |= (0x0001 << aid);
        status->mp_readyBitmap &= ~(0x0001 << aid);
        status->mp_lastRecvTick[aid] = OS_GetTick() | 1;
        MI_CpuCopy8(pInd->peerMacAdrs, status->childMacAddress[aid - 1], WM_SIZE_MACADDR);
        (void)OS_RestoreInterrupts(e);
    }

    MI_CpuFill16(status->portSeqNo[aid], 0x0001, sizeof(status->portSeqNo[0]));


    callback = (WMStartParentCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_START_PARENT;
    callback->errcode = WM_ERRCODE_SUCCESS;
    callback->state = WM_STATECODE_CONNECTED;
    MI_CpuCopy8(pInd->peerMacAdrs, callback->macAddress, WM_SIZE_MACADDR);
    callback->aid = (u16)aid;
    MI_CpuCopy16(&(pInd->ssid[8]), callback->ssid, WM_SIZE_CHILD_SSID);
    callback->parentSize = status->mp_parentSize;
    callback->childSize = status->mp_childSize;

    WMSP_ReturnResult2Wm9(callback);
}

static void WmspIndicateMlmeDeAuthenticate(WlCmdReq *req)
{
    int i;
    WMStatus *status = WMSP_GetStatusStructure();
    WlMlmeDeAuthInd *pInd = (WlMlmeDeAuthInd *)req;
    u8 tmpMacAddress[6];
    u16 tmpAID;

    if (WM_STATE_PARENT == status->state || WM_STATE_MP_PARENT == status->state) {
        MI_CpuCopy8(pInd->peerMacAdrs, tmpMacAddress, WM_SIZE_MACADDR);

        tmpAID = 0;
        for (i = 0; i < 15; ++i) {
            OSIntrMode e = OS_DisableInterrupts();
            if ((status->child_bitmap & (0x0001 << (i + 1))) && status->childMacAddress[i][0] == tmpMacAddress[0] && status->childMacAddress[i][1] == tmpMacAddress[1] && status->childMacAddress[i][2] == tmpMacAddress[2] && status->childMacAddress[i][3] == tmpMacAddress[3] && status->childMacAddress[i][4] == tmpMacAddress[4] && status->childMacAddress[i][5] == tmpMacAddress[5]) {
                tmpAID = (u16)(i + 1);

                {
                    status->child_bitmap &= ~(0x0001 << tmpAID);
                    status->mp_readyBitmap &= ~(0x0001 << tmpAID);
                    status->mp_lastRecvTick[tmpAID] = 0ULL;
                }

                MI_CpuClear8(status->childMacAddress[i], WM_SIZE_MACADDR);

                (void)OS_RestoreInterrupts(e);
                break;
            }
            (void)OS_RestoreInterrupts(e);
        }

        if (tmpAID == 0) {
            return;
        } else {
            WMStartParentCallback *callback = (WMStartParentCallback *)WMSP_GetBuffer4Callback2Wm9();

            callback->apiid = WM_APIID_START_PARENT;
            callback->errcode = WM_ERRCODE_SUCCESS;
            callback->state = WM_STATECODE_DISCONNECTED;
            callback->reason = pInd->reasonCode;
            callback->aid = tmpAID;
            MI_CpuCopy8(pInd->peerMacAdrs, callback->macAddress, WM_SIZE_MACADDR);
            callback->parentSize = status->mp_parentSize;
            callback->childSize = status->mp_childSize;

            WMSP_ReturnResult2Wm9(callback);
#ifdef WM_CLEAN_SEND_QUEUE
            if (status->mp_flag == TRUE) {
                WMSP_CleanSendQueue((u16)(0x0001 << tmpAID));
            }
#endif
        }
    } else {
        BOOL fCleanQueue = FALSE;
        OSIntrMode e = OS_DisableInterrupts();

        if (status->child_bitmap == 0) {
            (void)OS_RestoreInterrupts(e);
            return;
        }

        if (status->mp_flag == TRUE) {
            status->mp_flag = FALSE;
            fCleanQueue = TRUE;
            WMSP_CancelVAlarm();

            WMSP_SetThreadPriorityLow();
        }
        status->child_bitmap = 0;
        status->mp_readyBitmap = 0;
        status->ks_flag = FALSE;
        status->dcf_flag = FALSE;
        status->VSyncFlag = FALSE;

        status->wep_flag = FALSE;
        status->wepMode = WL_CMDLABEL_WEP_NO;
        MI_CpuClear8(status->wepKey, WM_SIZE_WEPKEY);

        WMSP_ResetSizeVars();

        status->beaconIndicateFlag = 0;
        status->state = WM_STATE_CLASS1;

        (void)OS_RestoreInterrupts(e);

        {
            WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();

            callback->apiid = WM_APIID_START_CONNECT;
            callback->errcode = WM_ERRCODE_SUCCESS;
            callback->state = WM_STATECODE_DISCONNECTED;
            callback->reason = pInd->reasonCode;
            callback->aid = status->aid;
            MI_CpuCopy8(status->parentMacAddress, callback->macAddress, WM_SIZE_MACADDR);
            callback->parentSize = status->mp_parentSize;
            callback->childSize = status->mp_childSize;

            WMSP_ReturnResult2Wm9(callback);
        }

#ifdef WM_CLEAN_SEND_QUEUE
        if (fCleanQueue) {
            WMSP_CleanSendQueue(0x0001);
        }
#endif
    }
}

static void WmspIndicateMlmeAuthenticate(WlCmdReq *req)
{
    WMIndCallback *callback = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_INDICATION;
    callback->errcode = WM_ERRCODE_SUCCESS;
    callback->state = WM_STATECODE_AUTHENTICATE;
    WMSP_ReturnResult2Wm9(callback);
}

void WMSP_InitAlarm(void)
{
#ifdef WM_ALARM_VARIABLE_ON_WRAM
    OS_CreateAlarm(&wmspMPIntervalAlarm);
    OS_CreateAlarm(&wmspMPAckAlarm);
#else
    OS_CreateAlarm(&WMSP_GetSystemWork()->mp_intervalAlarm);
    OS_CreateAlarm(&WMSP_GetSystemWork()->mp_ackAlarm);
#endif
}

void WMSP_CancelAllAlarms(void)
{
#ifdef WM_ALARM_VARIABLE_ON_WRAM
    OS_CancelAlarm(&wmspMPIntervalAlarm);
    OS_CancelAlarm(&wmspMPAckAlarm);
#else
    OS_CancelAlarm(&WMSP_GetSystemWork()->mp_intervalAlarm);
    OS_CancelAlarm(&WMSP_GetSystemWork()->mp_ackAlarm);
#endif
}

