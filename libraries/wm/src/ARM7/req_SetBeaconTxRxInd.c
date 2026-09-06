#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_SetBeaconTxRxInd(OSMessage msg)
{
#pragma unused(msg)

    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    WlParamSetCfm *pConfirm;
    WMCallback *cb;
    u32 *reqBuf = (u32 *)msg;

    pConfirm = WMSP_WL_ParamSetBeaconSendRecvInd(buf, (u16)reqBuf[1]);

    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_PARAM_SET_BCNTXRX_IND, pConfirm->resultCode);
        return;
    }

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_SET_BEACON_IND;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);

    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_SET_BEACON_IND;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
