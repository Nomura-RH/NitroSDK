#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_EndParent(OSMessage msg)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u32 wlBuf[128];
    WMCallback *cb;
    WlCmdCfm *pConfirm;


    if (status->state != WM_STATE_PARENT) {
        WMCallback *callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_END_PARENT;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    status->pparam.entryFlag = FALSE;

    {
        u16 wMac[3];
        s32 i;
        OSIntrMode e;

        for (i = 1; i < (WM_NUM_MAX_CHILD + 1); i++) {
            if (status->child_bitmap & (0x0001 << i)) {
                MI_CpuCopy8(status->childMacAddress[i - 1], wMac, WM_SIZE_MACADDR);
                {
                    s32 auth_retry;

                    for (auth_retry = 0; auth_retry < WMSP_DEAUTH_RETRY_MAX;) {
                        pConfirm = (WlCmdCfm *)WMSP_WL_MlmeDeAuthenticate((u16 *)wlBuf, wMac, WL_CMDLABEL_RSN_DEAUTH_LEAVING);
                        switch (pConfirm->resultCode) {
                        case WL_CMDRES_SUCCESS:
                            break;
                        case WL_CMDRES_TIMEOUT:
                        case WL_CMDRES_FAILURE:
                            auth_retry++;
                            continue;
                        default:
                            break;
                        }
                        break;
                    }
                }
                e = OS_DisableInterrupts();
                if (status->child_bitmap & (0x0001 << i)) {
                    status->child_bitmap &= ~(0x0001 << i);
                    status->mp_readyBitmap &= ~(0x0001 << i);
                    status->mp_lastRecvTick[i] = 0ULL;
                    (void)OS_RestoreInterrupts(e);

                    WMSP_IndicateDisconnectionFromMyself(TRUE, (u16)i, wMac);
                } else {
                    (void)OS_RestoreInterrupts(e);
                }
            }
        }
    }

    pConfirm = (WlCmdCfm *)WMSP_WL_MlmeReset((u16 *)wlBuf, WL_CMDLABEL_RST_MIB_CLR);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_MLME_RESET, pConfirm->resultCode);
        return;
    }
    status->beaconIndicateFlag = 0;
    status->state = WM_STATE_CLASS1;

    pConfirm = (WlCmdCfm *)WMSP_WL_DevIdle((u16 *)wlBuf);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_DEV_IDLE, pConfirm->resultCode);
        return;
    }

    status->state = WM_STATE_IDLE;

    status->wep_flag = FALSE;
    status->wepMode = WL_CMDLABEL_WEP_NO;
    MI_CpuClear8(status->wepKey, WM_SIZE_WEPKEY);

    WMSP_ResetSizeVars();

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_END_PARENT;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);

    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_END_PARENT;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}

