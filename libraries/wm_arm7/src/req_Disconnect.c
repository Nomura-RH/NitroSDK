#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult, u16 tryBmp, u16 resBmp);
static void WmspIndError(u16 wlCommand, u16 wlResult, u16 tryBmp, u16 resBmp);

static inline BOOL WMSPi_IsChild(u32 stat)
{
    return (stat == WM_STATE_MP_CHILD) || (stat == WM_STATE_CHILD);
}

static inline BOOL WMSPi_IsParent(u32 stat)
{
    return (stat == WM_STATE_MP_PARENT) || (stat == WM_STATE_PARENT);
}

void WMSP_Disconnect(OSMessage msg)
{
    BOOL result;
    u16 tryBmp;
    u16 resBmp;

    tryBmp = (u16)(((u32 *)msg)[1]);
    result = WMSP_DisconnectCore(msg, FALSE, &resBmp);

    if (result == TRUE) {
        WMDisconnectCallback *const callback = (WMDisconnectCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_DISCONNECT;
        callback->errcode = WM_ERRCODE_SUCCESS;
        callback->tryBitmap = tryBmp;
        callback->disconnectedBitmap = resBmp;
        (void)WMSP_ReturnResult2Wm9(callback);
        return;
    }
}

BOOL WMSP_DisconnectCore(u32 *args, BOOL indicateFlag, u16 *disconnected)
{
    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    WlCmdCfm *pConfirm;
    WMStatus *status = WMSP_GetStatusStructure();

    u16 aidBitmap = (u16)args[1];
    u16 reason = (u16)(indicateFlag ? args[2] : 0);
    u16 resBitmap = 0x0000;

    BOOL fCleanQueue = FALSE;

    if (WMSPi_IsParent(status->state)) {
        if (status->mp_flag == TRUE) {
            fCleanQueue = TRUE;
        }
    } else if (WMSPi_IsChild(status->state)) {
        OSIntrMode e = OS_DisableInterrupts();

        if (status->child_bitmap == 0) {
            (void)OS_RestoreInterrupts(e);
            if (!indicateFlag) {
                WMDisconnectCallback *callback = (WMDisconnectCallback *)WMSP_GetBuffer4Callback2Wm9();
                callback->apiid = WM_APIID_DISCONNECT;
                callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
                callback->wlCmdID = 0;
                callback->wlResult = 0;
                callback->tryBitmap = aidBitmap;
                callback->disconnectedBitmap = resBitmap;
                WMSP_ReturnResult2Wm9(callback);
            }
            return FALSE;
        }
        if (status->mp_flag == TRUE) {
            status->mp_flag = FALSE;
            fCleanQueue = TRUE;
            WMSP_CancelVAlarm();

            WMSP_SetThreadPriorityLow();

            if (status->state == WM_STATE_MP_CHILD) {
                status->state = WM_STATE_CHILD;
            }
        }
        status->child_bitmap = 0;
        status->mp_readyBitmap = 0;
        status->ks_flag = FALSE;
        status->dcf_flag = FALSE;
        status->VSyncFlag = FALSE;

        (void)OS_RestoreInterrupts(e);
    } else {
        if (!indicateFlag) {
            WMDisconnectCallback *callback = (WMDisconnectCallback *)WMSP_GetBuffer4Callback2Wm9();
            callback->apiid = WM_APIID_DISCONNECT;
            callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
            callback->wlCmdID = 0;
            callback->wlResult = 0;
            callback->tryBitmap = aidBitmap;
            callback->disconnectedBitmap = resBitmap;
            WMSP_ReturnResult2Wm9(callback);
        } else {
        }
        return FALSE;
    }

    if (WMSPi_IsChild(status->state)) {
        u16 wMac[3];

        MI_CpuCopy8(status->parentMacAddress, wMac, WM_SIZE_MACADDR);

        {
            s32 auth_retry;

            for (auth_retry = 0; auth_retry < WMSP_DEAUTH_RETRY_MAX;) {
                pConfirm = (WlCmdCfm *)WMSP_WL_MlmeDeAuthenticate(buf, wMac, WL_CMDLABEL_RSN_DEAUTH_LEAVING);
                switch (pConfirm->resultCode) {
                case WL_CMDRES_SUCCESS:
                    break;
                case WL_CMDRES_STATE_WRONG:
                    break;
                case WL_CMDRES_TIMEOUT:
                case WL_CMDRES_FAILURE:
                    auth_retry++;
                    continue;
                default:
                    if (indicateFlag) {
                        WmspIndError(WL_CMDCODE_MLME_DEAUTH, pConfirm->resultCode, aidBitmap, resBitmap);
                    } else {
                        WmspError(WL_CMDCODE_MLME_DEAUTH, pConfirm->resultCode, aidBitmap, resBitmap);
                    }
#ifdef WM_CLEAN_SEND_QUEUE
                    if (fCleanQueue) {
                        WMSP_CleanSendQueue(0x0001);
                    }
#endif
                    return FALSE;
                }
                break;
            }
        }
        resBitmap = 0x0001;

        status->beaconIndicateFlag = 0;
        status->state = WM_STATE_CLASS1;

        pConfirm = (WlCmdCfm *)WMSP_WL_MlmeReset(buf, WL_CMDLABEL_RST_MIB_CLR);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            if (indicateFlag) {
                WmspIndError(WL_CMDCODE_MLME_RESET, pConfirm->resultCode, aidBitmap, resBitmap);
            } else {
                WmspError(WL_CMDCODE_MLME_RESET, pConfirm->resultCode, aidBitmap, resBitmap);
            }
#ifdef WM_CLEAN_SEND_QUEUE
            if (fCleanQueue) {
                WMSP_CleanSendQueue(0x0001);
            }
#endif
            return FALSE;
        }

        pConfirm = (WlCmdCfm *)WMSP_WL_DevIdle(buf);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            if (indicateFlag) {
                WmspIndError(WL_CMDCODE_DEV_IDLE, pConfirm->resultCode, aidBitmap, resBitmap);
            } else {
                WmspError(WL_CMDCODE_DEV_IDLE, pConfirm->resultCode, aidBitmap, resBitmap);
            }
#ifdef WM_CLEAN_SEND_QUEUE
            if (fCleanQueue) {
                WMSP_CleanSendQueue(0x0001);
            }
#endif
            return FALSE;
        }

        status->state = WM_STATE_IDLE;

        status->wep_flag = FALSE;
        status->wepMode = WL_CMDLABEL_WEP_NO;
        MI_CpuClear8(status->wepKey, WM_SIZE_WEPKEY);

        WMSP_ResetSizeVars();

        if (indicateFlag == TRUE) {
            WMStartConnectCallback *cb = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_START_CONNECT;
            cb->errcode = WM_ERRCODE_SUCCESS;
            cb->state = WM_STATECODE_DISCONNECTED;
            cb->reason = reason;
            cb->aid = status->aid;
            MI_CpuCopy8(wMac, cb->macAddress, WM_SIZE_MACADDR);

            cb->parentSize = status->mp_parentSize;
            cb->childSize = status->mp_childSize;

            WMSP_ReturnResult2Wm9(cb);
        } else {
            WMSP_IndicateDisconnectionFromMyself(FALSE, 0, wMac);
        }
#ifdef WM_CLEAN_SEND_QUEUE
        if (fCleanQueue) {
            WMSP_CleanSendQueue(0x0001);
        }
#endif
    } else {
        u16 wMac[3];
        s32 i;
        u16 aid;

        for (i = 1; i < (WM_NUM_MAX_CHILD + 1); i++) {
            if (status->child_bitmap & aidBitmap & (0x0001 << i)) {
                aid = (u16)i;

                MI_CpuCopy8(status->childMacAddress[i - 1], wMac, WM_SIZE_MACADDR);

                {
                    s32 auth_retry;

                    for (auth_retry = 0; auth_retry < WMSP_DEAUTH_RETRY_MAX;) {
                        pConfirm = (WlCmdCfm *)WMSP_WL_MlmeDeAuthenticate(buf, wMac, WL_CMDLABEL_RSN_DEAUTH_LEAVING);
                        switch (pConfirm->resultCode) {
                        case WL_CMDRES_SUCCESS:
                            break;
                        case WL_CMDRES_TIMEOUT:
                        case WL_CMDRES_FAILURE:
                            auth_retry++;
                            continue;
                        default:
                            if (indicateFlag) {
                                WmspIndError(WL_CMDCODE_MLME_DEAUTH, pConfirm->resultCode, aidBitmap, resBitmap);
                            } else {
                                WmspError(WL_CMDCODE_MLME_DEAUTH, pConfirm->resultCode, aidBitmap, resBitmap);
                            }
#ifdef WM_CLEAN_SEND_QUEUE
                            if (fCleanQueue) {
                                WMSP_CleanSendQueue(0x0001);
                            }
#endif
                            return FALSE;
                        }
                        break;
                    }
                }

                {
                    OSIntrMode e = OS_DisableInterrupts();

                    if (status->child_bitmap & (0x0001 << i)) {
                        resBitmap |= (0x0001 << aid);
                        status->child_bitmap &= ~(0x0001 << i);
                        status->mp_readyBitmap &= ~(0x0001 << i);
                        status->mp_lastRecvTick[aid] = 0ULL;

                        MI_CpuClear8(status->childMacAddress[i - 1], WM_SIZE_MACADDR);

                        (void)OS_RestoreInterrupts(e);

                        if (indicateFlag == TRUE) {
                            WMStartParentCallback *cb = (WMStartParentCallback *)WMSP_GetBuffer4Callback2Wm9();
                            cb->apiid = WM_APIID_START_PARENT;
                            cb->errcode = WM_ERRCODE_SUCCESS;
                            cb->state = WM_STATECODE_DISCONNECTED;
                            cb->reason = reason;
                            cb->aid = aid;
                            MI_CpuCopy8(wMac, cb->macAddress, WM_SIZE_MACADDR);
                            cb->parentSize = status->mp_parentSize;
                            cb->childSize = status->mp_childSize;

                            WMSP_ReturnResult2Wm9(cb);
                        } else {
                            WMSP_IndicateDisconnectionFromMyself(TRUE, (u16)i, wMac);
                        }

#ifdef WM_CLEAN_SEND_QUEUE
                        if (fCleanQueue) {
                            WMSP_CleanSendQueue((u16)(0x0001 << i));
                        }
#endif
                    } else {
                        (void)OS_RestoreInterrupts(e);
                    }
                }
            }
        }
    }
    if (disconnected != NULL) {
        *disconnected = resBitmap;
    }
    return TRUE;
}

void WMSP_IndicateDisconnectionFromMyself(BOOL parent, u16 aid, void *mac)
{
    WMStatus *status = WMSP_GetStatusStructure();
    WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

    cb->errcode = WM_ERRCODE_SUCCESS;

    if (parent) {
        WMStartParentCallback *cb_Parent = (WMStartParentCallback *)cb;
        cb_Parent->apiid = WM_APIID_START_PARENT;
        cb_Parent->state = WM_STATECODE_DISCONNECTED_FROM_MYSELF;
        cb_Parent->reason = WM_DISCONNECT_REASON_FROM_MYSELF;
        cb_Parent->aid = aid;
        MI_CpuCopy8(mac, cb_Parent->macAddress, WM_SIZE_MACADDR);
        cb_Parent->parentSize = status->mp_parentSize;
        cb_Parent->childSize = status->mp_childSize;
    } else {
        WMStartConnectCallback *cb_Child = (WMStartConnectCallback *)cb;
        cb_Child->apiid = WM_APIID_START_CONNECT;
        cb_Child->state = WM_STATECODE_DISCONNECTED_FROM_MYSELF;
        cb_Child->reason = WM_DISCONNECT_REASON_FROM_MYSELF;
        cb_Child->aid = status->aid;
        MI_CpuCopy8(mac, cb_Child->macAddress, WM_SIZE_MACADDR);
        cb_Child->parentSize = status->mp_parentSize;
        cb_Child->childSize = status->mp_childSize;
    }

    WMSP_ReturnResult2Wm9(cb);
}

static void WmspError(u16 wlCommand, u16 wlResult, u16 tryBmp, u16 resBmp)
{
    WMDisconnectCallback *callback = (WMDisconnectCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_DISCONNECT;
    callback->errcode = WM_ERRCODE_FAILED;
    callback->wlCmdID = wlCommand;
    callback->wlResult = wlResult;
    callback->tryBitmap = tryBmp;
    callback->disconnectedBitmap = resBmp;
    WMSP_ReturnResult2Wm9(callback);
}

static void WmspIndError(u16 wlCommand, u16 wlResult, u16 tryBmp, u16 resBmp)
{
    WMDisconnectCallback *callback = (WMDisconnectCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_AUTO_DISCONNECT;
    callback->errcode = WM_ERRCODE_FAILED;
    callback->wlCmdID = wlCommand;
    callback->wlResult = wlResult;
    callback->tryBitmap = tryBmp;
    callback->disconnectedBitmap = resBmp;
    WMSP_ReturnResult2Wm9(callback);
}

