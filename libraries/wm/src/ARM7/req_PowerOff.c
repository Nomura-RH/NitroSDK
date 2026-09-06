#include "wmsp_private.h"

void WMSP_PowerOff(OSMessage msg)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u32 wlBuf[128];
    WMCallback *cb;
    WlCmdCfm *pConfirm;

    if (status->state != WM_STATE_IDLE) {
        cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        cb->apiid = WM_APIID_POWER_OFF;
        cb->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9((void *)cb);
        return;
    }

    pConfirm = (WlCmdCfm *)WMSP_WL_DevShutdown((u16 *)wlBuf);

    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        cb->apiid = WM_APIID_POWER_OFF;
        cb->errcode = WM_ERRCODE_FAILED;
        cb->wlCmdID = WL_CMDCODE_DEV_SHUTDOWN;
        cb->wlResult = pConfirm->resultCode;
        WMSP_ReturnResult2Wm9((void *)cb);
        return;
    }

    status->state = WM_STATE_STOP;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_POWER_OFF;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);
    return;
}
