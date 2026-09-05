#include "wmsp_private.h"

void WMSP_AutoDeAuth(OSMessage msg)
{
    u32 wlBuf[128];
    u32 *arg = (u32 *)msg;
    u16 wMac[3];
    WlCmdCfm *pConfirm;

    MI_CpuCopy8(&arg[1], wMac, 6);

    {
        s32 auth_retry;

        for (auth_retry = 0; auth_retry < WMSP_DEAUTH_RETRY_MAX;) {
            pConfirm = (WlCmdCfm *)WMSP_WL_MlmeDeAuthenticate((u16 *)wlBuf, wMac, WL_CMDLABEL_RSN_NO_ENTRY);
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

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_AUTO_DEAUTH;
        if (pConfirm->resultCode == WL_CMDRES_SUCCESS) {
            cb->errcode = WM_ERRCODE_SUCCESS;
        } else {
            cb->errcode = WM_ERRCODE_FAILED;
            cb->wlCmdID = WL_CMDCODE_MLME_DEAUTH;
            cb->wlResult = pConfirm->resultCode;
        }
        WMSP_ReturnResult2Wm9(cb);
    }
}

