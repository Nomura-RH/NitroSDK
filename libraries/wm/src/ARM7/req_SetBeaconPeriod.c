#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_SetBeaconPeriod(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    u32 wlBuf[128];
    WMStatus *status = WMSP_GetStatusStructure();
    WlParamSetCfm *pConfirm;

    u16 beaconPeriod;

    beaconPeriod = (u16)(buf[1]);


    pConfirm = WMSP_WL_ParamSetBeaconPeriod((u16 *)wlBuf, beaconPeriod);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_PARAM_SET_BEACON_PERIOD, pConfirm->resultCode);
        return;
    }

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_BEACON_PERIOD;
        cb->errcode = WM_ERRCODE_SUCCESS;

        WMSP_ReturnResult2Wm9((void *)cb);
    }
    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_SET_BEACON_PERIOD;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
