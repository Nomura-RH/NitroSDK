#include "wmsp_private.h"

WMSPWork wmspW;

#ifdef WM_THREAD_VARIABLE_ON_WRAM
#include <nitro/wram_begin.h>

static OSThread wmspRequestThread;
static OSThread wmspIndicateThread;

#include <nitro/wram_end.h>
#endif

static void WmspPxiCallback(PXIFifoTag tag, u32 data, BOOL err);
static void WmspError(u16 wmApiID, u16 wlCommand, u16 wlResult);

void WM_sp_init(WlInit *wlInit, WmInit *wmInit)
{
    wmspW.dmaNo = wmInit->dmaNo;
    wmspW.arenaId = wlInit->heapFunc.os.id;
    wmspW.heapHandle = wlInit->heapFunc.os.heapHandle;

    wmspW.wm7buf = NULL;
    wmspW.status = NULL;

#ifdef WM_DEBUG_HEAP
    WMSP_CheckWLHeap("WM_sp_init");
#endif

    OS_InitMessageQueue(&(wmspW.toWLmsgQ), wmspW.toWLmsg, WMSP_TO_WL_MSGQ_COUNT);
    OS_InitMessageQueue(&(wmspW.fromWLmsgQ), wmspW.fromWLmsg, WMSP_FROM_WL_MSGQ_COUNT);
    OS_InitMessageQueue(&(wmspW.confirmQ), wmspW.confirm, WMSP_FROM_WL_MSGQ_COUNT);
    OS_InitMessageQueue(&(wmspW.requestQ), wmspW.request, WMSP_REQUEST_MSGQ_COUNT);
    wlInit->sendMsgQueuep = &(wmspW.toWLmsgQ);
    wlInit->recvMsgQueuep = &(wmspW.fromWLmsgQ);

#if (SDK_VERSION_WL >= 20900)
    wmspW.indPrio_high = wmInit->indPrio_high;
    wmspW.wlPrio_high = wmInit->wlPrio_high;
    wmspW.reqPrio_high = wmInit->reqPrio_high;
    wmspW.indPrio_low = wmInit->indPrio_low;
    wmspW.wlPrio_low = wmInit->wlPrio_low;
    wmspW.reqPrio_low = wmInit->reqPrio_low;
#endif

    OS_InitMutex(&(wmspW.fifoExclusive));

    OS_CreateThread(
#ifdef WM_THREAD_VARIABLE_ON_WRAM
        &wmspIndicateThread,
#else
        &(wmspW.indicateThread),
#endif
        WMSP_IndicateThread,
        NULL,
        (void *)(wmspW.indicateStack + (WMSP_INDICATE_STACK_SIZE / sizeof(u32))),
        WMSP_INDICATE_STACK_SIZE,
#if (SDK_VERSION_WL >= 20900)
        wmInit->indPrio_low
#else
        wmInit->indPrio
#endif
    );

#ifdef WM_THREAD_VARIABLE_ON_WRAM
    OS_WakeupThreadDirect(&wmspIndicateThread);
#else
    OS_WakeupThreadDirect(&(wmspW.indicateThread));
#endif

    OS_CreateThread(
#ifdef WM_THREAD_VARIABLE_ON_WRAM
        &wmspRequestThread,
#else
        &(wmspW.requestThread),
#endif
        WMSP_RequestThread,
        NULL,
        (void *)(wmspW.requestStack + (WMSP_REQUEST_STACK_SIZE / sizeof(u32))),
        WMSP_REQUEST_STACK_SIZE,
#if (SDK_VERSION_WL >= 20900)
        wmInit->reqPrio_low
#else
        wmInit->reqPrio
#endif
    );

#ifdef WM_THREAD_VARIABLE_ON_WRAM
    OS_WakeupThreadDirect(&wmspRequestThread);
#else
    OS_WakeupThreadDirect(&(wmspW.requestThread));
#endif

    {
        s32 i;

        for (i = 0; i < WMSP_RSSI_HISTORY_MAX; i++) {
            wmspW.rssiHistory[i] = 0;
        }
        wmspW.rssiIndex = 0;
    }

    if (!OS_IsVAlarmAvailable()) {
        OS_InitVAlarm();
    }

    PXI_Init();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_WM, WmspPxiCallback);

#if (SDK_VERSION_WL >= 17500)
    wlInit->dmaChannel = WM_SP_WL_DMA_NO;
#endif
#if (SDK_VERSION_WL >= 20900)
    wlInit->priority = wmInit->wlPrio_low;
#endif

    (void)WL_InitDriver(wlInit);
}

#include <nitro/wram_begin.h>
void WMSP_ReturnResult2Wm9(void *ptr)
{
    while (0 > PXI_SendWordByFifo(PXI_FIFO_TAG_WM, (u32)ptr, FALSE)) {
        SVC_WaitByLoop(256);
    }
    OS_UnlockMutex(&(wmspW.fifoExclusive));
}

void WMSP_Wait4Wm9(void)
{
    OS_LockMutex(&(wmspW.fifoExclusive));
    while (OS_GetSystemWork()->wm_callback_control & WM_EXCEPTION_CB_MASK) {
        SVC_WaitByLoop(256);
    }
    OS_UnlockMutex(&(wmspW.fifoExclusive));
}

void *WMSP_GetBuffer4Callback2Wm9(void)
{
    OS_LockMutex(&(wmspW.fifoExclusive));

    while (OS_GetSystemWork()->wm_callback_control & WM_EXCEPTION_CB_MASK) {
        SVC_WaitByLoop(256);
    }
    OS_GetSystemWork()->wm_callback_control |= WM_EXCEPTION_CB_MASK;

    return (void *)(wmspW.wm7buf->fifo7to9);
}

#include <nitro/wram_end.h>

u16 *WMSP_WlRequest(u16 *request)
{
    OSMessage msg;

    (void)OS_SendMessage(&(wmspW.toWLmsgQ), (OSMessage)request, OS_MESSAGE_BLOCK);
    (void)OS_ReceiveMessage(&(wmspW.confirmQ), &msg, OS_MESSAGE_BLOCK);
    WM_ASSERT(request == msg);

#if SDK_VERSION_WL >= 21000
    {
        WlCmdCfm *cfm = (WlCmdCfm *)WL_CalcConfirmPointer(msg);
        if (cfm->resultCode == WL_CMDRES_FLASH_ERR) {
            WMIndCallback *callback = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();
            callback->apiid = WM_APIID_INDICATION;
            callback->errcode = WM_ERRCODE_FLASH_ERROR;
            callback->state = WM_STATECODE_UNKNOWN;
            WMSP_ReturnResult2Wm9(callback);
            SND_BeginSleep();
            OS_Terminate();
        }
    }
#endif

    return (u16 *)msg;
}

static void WmspPxiCallback(PXIFifoTag tag, u32 data, BOOL err)
{
#pragma unused(tag)

    BOOL result;

    if (err) {
        return;
    }

    WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", *((u16 *)data) & ~WM_API_REQUEST_ACCEPTED);

    result = OS_SendMessage(&(wmspW.requestQ), (OSMessage)data, OS_MESSAGE_NOBLOCK);

    if (!result) {
        if (wmspW.wm7buf != NULL) {
            WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = *((u16 *)data);
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->wlCmdID = 0;
            cb->wlResult = 0;
            WMSP_ReturnResult2Wm9((void *)cb);
        }
    }
}

BOOL WMSP_CheckMacAddress(const u8 *macAdr)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u8 *myMacAddress = status->MacAddress;

    if ((macAdr[0] == myMacAddress[0]) && (macAdr[1] == myMacAddress[1]) && (macAdr[2] == myMacAddress[2]) && (macAdr[3] == myMacAddress[3]) && (macAdr[4] == myMacAddress[4]) && (macAdr[5] == myMacAddress[5])) {
        return TRUE;
    }
    return FALSE;
}

WMMpRecvData *WMSP_GetKeyDataAdr(WMMpRecvHeader *header, u16 aid)
{
    WMStatus *status = WMSP_GetStatusStructure();
    int i;

    WMMpRecvData *recvdata_p[15];
    if (0 == (status->child_bitmap & (0x0001 << aid))) {
        return NULL;
    }

    if (header->count == 0) {
        return NULL;
    }

    recvdata_p[0] = (WMMpRecvData *)(header->data);

    for (i = 1; i < header->count; ++i) {
        recvdata_p[i] = (WMMpRecvData *)((u32)(recvdata_p[i - 1]) + header->length);
    }

    for (i = 0; i < header->count; ++i) {
        if (recvdata_p[i]->aid == aid) {
            return recvdata_p[i];
        }
    }
    return NULL;
}

void WMSP_CopyParentParam(WMGameInfo *gameInfop, WMParentParam *pparamp)
{
    gameInfop->ggid = pparamp->ggid;
    gameInfop->tgid = pparamp->tgid;

    gameInfop->attribute = (u8)(((pparamp->entryFlag) ? WM_ATTR_FLAG_ENTRY : 0)
        | ((pparamp->multiBootFlag) ? WM_ATTR_FLAG_MB : 0)
        | ((pparamp->KS_Flag) ? WM_ATTR_FLAG_KS : 0)
        | 0
    );

    gameInfop->userGameInfoLength = (u8)pparamp->userGameInfoLength;
    gameInfop->magicNumber = WM_GAMEINFO_MAGIC_NUMBER;
    gameInfop->ver = WM_GAMEINFO_VERSION_NUMBER;
    gameInfop->platform = WM_GAMEINFO_PLATFORM_ID_NITRO;

    gameInfop->parentMaxSize = pparamp->parentMaxSize;
    gameInfop->childMaxSize = pparamp->multiBootFlag && pparamp->childMaxSize >= 8 ? 8 : pparamp->childMaxSize;

    if (0 != gameInfop->userGameInfoLength) {
        MI_CpuCopy8(pparamp->userGameInfo, gameInfop->userGameInfo, (gameInfop->userGameInfoLength + 1U) & ~1U);
    }
}

BOOL WMSP_SetAllParams(u16 wmApiID, u16 *buf)
{
#define WMSP_RETRY_LIMIT     7
#define WMSP_BEACONLOST_TH   16
#define WMSP_ACTIVEZONE_TIME 10

    WMStatus *status = WMSP_GetStatusStructure();
    WlParamSetCfm *pConfirm;

    {
        WlParamSetAllReq *pReq;
        pReq = (WlParamSetAllReq *)buf;

        MI_CpuCopy8(status->MacAddress, pReq->staMacAdrs, WM_SIZE_MACADDR);

        pReq->retryLimit = WMSP_RETRY_LIMIT;

#if SDK_VERSION_WL >= 15600
        pReq->enableChannel = status->enableChannel;
#else
        pReq->domain = WL_CMDLABEL_DOMAIN_MKK;
#endif

        pReq->rate = status->rate;

        pReq->mode = status->mode;

        if (status->wep_flag == FALSE) {
            pReq->wepMode = WL_CMDLABEL_WEP_NO;
            pReq->wepKeyId = 0;
            MI_CpuFill16(pReq->wepKey, 0x0000, WM_SIZE_WEPKEY);

            pReq->authAlgo = WL_CMDLABEL_AUTH_OPEN_SYSTEM;
        } else {
            u32 dma_no = WMSP_GetSystemWork()->dmaNo;
            pReq->wepMode = status->wepMode;
            pReq->wepKeyId = status->wepKeyId;

            MI_CpuCopy8(status->wepKey, pReq->wepKey, WM_SIZE_WEPKEY);
            pReq->authAlgo = WL_CMDLABEL_AUTH_SHARED_KEY;
        }

#ifdef WM_SCAN_SSID_FILTER_ON
        pReq->beaconType = WL_CMDLABEL_INCLUDE_SSID;
#else
        pReq->beaconType = WL_CMDLABEL_NOT_INCLUDE_SSID;
#endif

        pReq->probeRes = WL_CMDLABEL_NOT_RESPONSE_BC_PRBREQ;

        if (status->mode == WL_CMDLABEL_MODE_PARENT) {
            pReq->beaconLostTh = 0;
        } else {
            pReq->beaconLostTh = WMSP_BEACONLOST_TH;
        }

        pReq->activeZoneTime = WMSP_ACTIVEZONE_TIME;

        if (wmApiID == WM_APIID_START_SCAN_EX) {
            MI_CpuFill16(&(pReq->ssidMask[0]), 0x0000, 32);
        } else {
            MI_CpuFill16(&(pReq->ssidMask[0]), 0x0000, 8);
            MI_CpuFill16(&(pReq->ssidMask[8]), 0xffff, 24);
        }

        pReq->preambleType = status->preamble;

        pConfirm = WMSP_WL_ParamSetAll(pReq);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(wmApiID, WL_CMDCODE_PARAM_SET_ALL, pConfirm->resultCode);
            return FALSE;
        }
    }

    return TRUE;
}

#include <nitro/wram_begin.h>
u16 WMSP_GetAllowedChannel(u16 bitField)
{
    u16 temp;
    s32 max;
    s32 min;
    s32 center;

    temp = (u16)(bitField & WMSP_ENABLE_CHANNEL_MASK);
    if (temp == 0x0000) {
        return 0x0000;
    }

    for (min = 0; min < 16; min++) {
        if (temp & (0x0001 << min)) {
            break;
        }
    }
    for (max = 15; max; max--) {
        if (temp & (0x0001 << max)) {
            break;
        }
    }

    if ((max - min) < WMSP_ENABLE_CHANNEL_PERIOD) {
        return (u16)(0x0001 << min);
    }

    {
        s32 i;

        center = (max + min) / 2;
        for (i = 0; i < (max - min); i++) {
            center = center + ((((i % 2) * 2) - 1) * i);
            if (temp & (0x0001 << center)) {
                break;
            }
        }
    }

    if (((max - center) < WMSP_ENABLE_CHANNEL_PERIOD) || ((center - min) < WMSP_ENABLE_CHANNEL_PERIOD)) {
        return (u16)((0x0001 << min) | (0x0001 << max));
    }

    return (u16)((0x0001 << max) | (0x0001 << center) | (0x0001 << min));
}

#include <nitro/wram_end.h>

void WMSP_AddRssiToList(u8 rssi8)
{
    wmspW.rssiHistory[wmspW.rssiIndex] = rssi8;
    wmspW.rssiIndex = (u32)((wmspW.rssiIndex + 1) % WMSP_RSSI_HISTORY_MAX);
    WMSP_AddRssiToRandomPool(rssi8);
}

void WMSP_FillRssiIntoList(u8 rssi8)
{
    int i;
    for (i = 0; i < WMSP_RSSI_HISTORY_MAX; i++) {
        wmspW.rssiHistory[i] = rssi8;
    }
    wmspW.rssiIndex = 0;
}

u16 WMSP_GetAverageLinkLevel()
{
    s32 i;
    u32 temp = 0;

    WM_DPRINTF_LINKLEVEL("L:");

    for (i = 0; i < WMSP_RSSI_HISTORY_MAX; i++) {
        WM_DPRINTF_LINKLEVEL(" %d", wmspW.rssiHistory[i]);

        temp += wmspW.rssiHistory[i];
    }

    temp /= WMSP_RSSI_HISTORY_MAX;

    WM_DPRINTF_LINKLEVEL("-> %d\n", temp);

    return WMSP_GetLinkLevel(temp);
}

u16 WMSP_GetLinkLevel(u32 rssi)
{
    WMSPWork *const work = WMSP_GetSystemWork();
    WMArm7Buf *const p = work->wm7buf;

    if (p->connectPInfo.gameInfo.platform == WM_GAMEINFO_PLATFORM_ID_REVOLUTION) {
        if (rssi < WMSP_RSSI_REVOLUTION_LINK_LEVEL_1) {
            return WM_LINK_LEVEL_0;
        }
        if (rssi < WMSP_RSSI_REVOLUTION_LINK_LEVEL_2) {
            return WM_LINK_LEVEL_1;
        }
        if (rssi < WMSP_RSSI_REVOLUTION_LINK_LEVEL_3) {
            return WM_LINK_LEVEL_2;
        }

        return WM_LINK_LEVEL_3;
    } else {
        if (rssi < WMSP_RSSI_NITRO_LINK_LEVEL_1) {
            return WM_LINK_LEVEL_0;
        }
        if (rssi < WMSP_RSSI_NITRO_LINK_LEVEL_2) {
            return WM_LINK_LEVEL_1;
        }
        if (rssi < WMSP_RSSI_NITRO_LINK_LEVEL_3) {
            return WM_LINK_LEVEL_2;
        }

        return WM_LINK_LEVEL_3;
    }
}

#if (SDK_VERSION_WL >= 20900)

void WMSP_SetThreadPriorityLow(void)
{
    OSIntrMode e = OS_DisableInterrupts();

    (void)OS_DisableScheduler();

#ifdef WM_THREAD_VARIABLE_ON_WRAM
    (void)OS_SetThreadPriority(&wmspRequestThread, wmspW.reqPrio_low);
#else
    (void)OS_SetThreadPriority(&(wmspW.requestThread), wmspW.reqPrio_low);
#endif

    (void)OS_SetThreadPriority(WL_GetThreadStruct(), wmspW.wlPrio_low);

#ifdef WM_THREAD_VARIABLE_ON_WRAM
    (void)OS_SetThreadPriority(&wmspIndicateThread, wmspW.indPrio_low);
#else
    (void)OS_SetThreadPriority(&(wmspW.indicateThread), wmspW.indPrio_low);
#endif

    (void)OS_EnableScheduler();
    (void)OS_RestoreInterrupts(e);
}

void WMSP_SetThreadPriorityHigh(void)
{
    OSIntrMode e = OS_DisableInterrupts();

    (void)OS_DisableScheduler();

#ifdef WM_THREAD_VARIABLE_ON_WRAM
    (void)OS_SetThreadPriority(&wmspIndicateThread, wmspW.indPrio_high);
#else
    (void)OS_SetThreadPriority(&(wmspW.indicateThread), wmspW.indPrio_high);
#endif

    (void)OS_SetThreadPriority(WL_GetThreadStruct(), wmspW.wlPrio_high);

#ifdef WM_THREAD_VARIABLE_ON_WRAM
    (void)OS_SetThreadPriority(&wmspRequestThread, wmspW.reqPrio_high);
#else
    (void)OS_SetThreadPriority(&(wmspW.requestThread), wmspW.reqPrio_high);
#endif

    (void)OS_EnableScheduler();
    (void)OS_RestoreInterrupts(e);
}

#endif

void WMSP_CheckWLHeap(char *name)
{
#pragma unused(name)
    WMSPWork *work = WMSP_GetSystemWork();
    long freeSize;

    freeSize = OS_CheckHeap(OS_ARENA_WRAM_SUBPRIV, work->heapHandle);
    if (freeSize < 0) {
        OS_TWarning("CheckWLHeap: heap is corrupted !! in %s", name);
    } else {
        WM_DPRINTF_HEAP("CheckWLHeap: free size = %d in %s\n", freeSize, name);
    }
}

OSThread *WMSP_GetRequestThread(void)
{
#ifdef WM_THREAD_VARIABLE_ON_WRAM
    return &wmspRequestThread;
#else
    return &(wmspW.requestThread);
#endif
}

OSThread *WMSP_GetIndicateThread(void)
{
#ifdef WM_THREAD_VARIABLE_ON_WRAM
    return &wmspIndicateThread;
#else
    return &(wmspW.indicateThread);
#endif
}

static void WmspError(u16 wmApiID, u16 wlCommand, u16 wlResult)
{
    WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = wmApiID;
    callback->errcode = WM_ERRCODE_FAILED;
    callback->wlCmdID = wlCommand;
    callback->wlResult = wlResult;
    WMSP_ReturnResult2Wm9(callback);
}

u32 *WMSP_GetInternalRequestBuf(void)
{
    s32 i;
    u32 *ret = NULL;
    OSIntrMode e;

    e = OS_DisableInterrupts();
    if (wmspW.wm7buf != NULL) {
        for (i = 0; i < 32; i++) {
            if (wmspW.wm7buf->requestBuf[i * 4] & WM_API_REQUEST_ACCEPTED) {
                ret = &(wmspW.wm7buf->requestBuf[i * 4]);
                *ret &= ~(WM_API_REQUEST_ACCEPTED);
                break;
            }
        }
    }
    (void)OS_RestoreInterrupts(e);
    return ret;
}

void WMSP_ResetSizeVars(void)
{
    WMStatus *status = WMSP_GetStatusStructure();

    status->mp_sendSize = 0;
    status->mp_recvSize = 0;
    status->mp_parentSize = 0;
    status->mp_childSize = 0;
    status->mp_maxSendSize = 0;
    status->mp_maxRecvSize = 0;
    status->mp_parentMaxSize = 0;
    status->mp_childMaxSize = 0;
}

void WMSP_SetParentMaxSize(u16 parentMaxSize)
{
    if (parentMaxSize > 0x200) {
        parentMaxSize = 0x200;
    }

    WMStatus *status = WMSP_GetStatusStructure();

    status->mp_parentSize = parentMaxSize;
    status->mp_parentMaxSize = parentMaxSize;

    if (status->aid == 0) {
        u16 maxSendSize = (u16)(parentMaxSize + WM_HEADER_PARENT_MAX_SIZE);
        status->mp_maxSendSize = maxSendSize;
        status->mp_sendSize = maxSendSize;
    } else {
        u16 maxRecvSize = (u16)(parentMaxSize + WM_HEADER_PARENT_MAX_SIZE);
        status->mp_maxRecvSize = maxRecvSize;
        status->mp_recvSize = maxRecvSize;
    }
}

void WMSP_SetChildMaxSize(u16 childMaxSize)
{
    if (childMaxSize > 0x200) {
        childMaxSize = 0x200;
    }

    WMStatus *status = WMSP_GetStatusStructure();

    status->mp_childMaxSize = childMaxSize;
    status->mp_childSize = childMaxSize;

    if (status->aid == 0) {
        u16 maxRecvSize = (u16)(childMaxSize + WM_HEADER_CHILD_MAX_SIZE);
        status->mp_maxRecvSize = maxRecvSize;
        status->mp_recvSize = maxRecvSize;
    } else {
        u16 maxSendSize = (u16)(childMaxSize + WM_HEADER_CHILD_MAX_SIZE);
        status->mp_maxSendSize = maxSendSize;
        status->mp_sendSize = maxSendSize;
    }
}

void WMSP_SetParentSize(u16 parentSize)
{
    WMStatus *status = WMSP_GetStatusStructure();

    status->mp_parentSize = parentSize;

    if (status->aid == 0) {
        u16 sendSize = (u16)(parentSize + WM_HEADER_PARENT_MAX_SIZE);
        status->mp_sendSize = sendSize;
    } else {
        u16 recvSize = (u16)(parentSize + WM_HEADER_PARENT_MAX_SIZE);
        status->mp_recvSize = recvSize;
    }
}

void WMSP_SetChildSize(u16 childSize)
{
    WMStatus *status = WMSP_GetStatusStructure();

    status->mp_childSize = childSize;

    if (status->aid == 0) {
        u16 recvSize = (u16)(childSize + WM_HEADER_CHILD_MAX_SIZE);
        status->mp_recvSize = recvSize;
    } else {
        u16 sendSize = (u16)(childSize + WM_HEADER_CHILD_MAX_SIZE);
        status->mp_sendSize = sendSize;
    }
}

