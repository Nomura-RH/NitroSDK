
#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_InitWirelessCounter(OSMessage msg)
{
    u32 wlBuf[128];
    WMCallback *cb;
    WlCmdCfm *pConfirm;

    pConfirm = (WlCmdCfm *)WMSP_WL_DevSetInitializeWirelessCounter((u16 *)wlBuf);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_DEV_INIT_INFO, pConfirm->resultCode);
        return;
    }

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_INIT_W_COUNTER;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);
    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_INIT_W_COUNTER;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}

