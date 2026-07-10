#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_EndScan(OSMessage msg)
{
    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    WMSPWork *const work = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    if (status->state != WM_STATE_SCAN) {
        WMCallback *callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_END_SCAN;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    {
        WlDevIdleCfm *p_confirm = WMSP_WL_DevIdle(buf);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_IDLE, p_confirm->resultCode);
            return;
        }
    }

    status->state = WM_STATE_IDLE;

    if (status->preamble == WL_CMDLABEL_PREAMBLE_LONG) {
        WlParamSetCfm *p_confirm;

        p_confirm = WMSP_WL_ParamSetPreambleType(buf, WL_CMDLABEL_PREAMBLE_SHORT);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_PARAM_SET_PREAMBLE_TYPE, p_confirm->resultCode);
            return;
        }

        status->preamble = WL_CMDLABEL_PREAMBLE_SHORT;
    }

    {
        WMCallback *callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_END_SCAN;
        callback->errcode = WM_ERRCODE_SUCCESS;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_END_SCAN;
    callback->errcode = WM_ERRCODE_FAILED;
    callback->wlCmdID = wlCommand;
    callback->wlResult = wlResult;
    WMSP_ReturnResult2Wm9(callback);
}

