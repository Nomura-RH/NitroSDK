#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_GetWirelessCounter(OSMessage msg)
{
    u32 wlBuf[128];
    WMGetWirelessCounterCallback *cb;
    WlDevGetInfoCfm *pConfirm;

    pConfirm = (WlDevGetInfoCfm *)WMSP_WL_DevGetWirelessCounter((u16 *)wlBuf);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_DEV_GET_INFO, pConfirm->resultCode);
        return;
    }

    cb = (WMgetWirelessCounterCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_GET_W_COUNTER;
    cb->errcode = WM_ERRCODE_SUCCESS;
    MI_CpuCopy16(&(pConfirm->counter), &(cb->TX_Success), 180);

    WMSP_ReturnResult2Wm9((void *)cb);
    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMgetWirelessCounterCallback *cb;

    cb = (WMgetWirelessCounterCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_GET_W_COUNTER;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}

