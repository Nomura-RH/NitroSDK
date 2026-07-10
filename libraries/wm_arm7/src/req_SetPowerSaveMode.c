#include "wmsp_private.h"

void WMSP_SetPowerSaveMode(OSMessage msg)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    BOOL powerSave;
    u16 pwrMgtMode;
    WMCallback *callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

    callback->apiid = WM_APIID_SET_PS_MODE;

    if (status->state != WM_STATE_DCF_CHILD) {
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    powerSave = (BOOL)(((u32 *)msg)[1]);

    if (powerSave == TRUE) {
        pwrMgtMode = WL_CMDLABEL_PMG_PS;
    } else {
        pwrMgtMode = WL_CMDLABEL_PMG_CONT_ACT;
    }

    {
        WlMlmePowerManagementCfm *p_confirm;

        u16 wakeUp;
        u16 recieveDtims;

        wakeUp = WL_CMDLABEL_WAKEUP_NORMAL;
        recieveDtims = WL_CMDLABEL_RX_ALL_DTIM;

        p_confirm = WMSP_WL_MlmePowerManagement(buf, pwrMgtMode, wakeUp, recieveDtims);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            callback->errcode = WM_ERRCODE_FAILED;
            callback->wlCmdID = WL_CMDCODE_MLME_PWRMGT;
            callback->wlResult = p_confirm->resultCode;
            WMSP_ReturnResult2Wm9(callback);
            return;
        }
    }

    {
        WlMaDataCfm *p_confirm;
        WlTxFrame wlTxFrame;

        WMSPWork *sys = WMSP_GetSystemWork();
        WMStatus *status = WMSP_GetStatusStructure();

        MI_CpuCopy8(status->parentMacAddress, status->dcf_destAdr, WM_SIZE_MACADDR);
        status->dcf_sendData = (u16 *)wlBuf;
        status->dcf_sendSize = 0;
        status->dcf_sendFlag = TRUE;

        MI_CpuClear16(&wlTxFrame, sizeof(wlTxFrame));
        wlTxFrame.frameId = (u16)0;
        wlTxFrame.length = 0;
        MI_WriteByte(&wlTxFrame.rate,
            (u8)((status->rate == WL_CMDLABEL_RATE_2M) ? WL_RATE_2MBPS : WL_RATE_1MBPS));
        MI_CpuCopy8(status->parentMacAddress, wlTxFrame.destAdrs, WM_SIZE_MACADDR);
        MI_CpuCopy8(status->MacAddress, wlTxFrame.srcAdrs, WM_SIZE_MACADDR);
        wlTxFrame.datap = (u16 *)wlBuf;

        p_confirm = WMSP_WL_MaData((u16 *)wlBuf, &wlTxFrame);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            callback->errcode = WM_ERRCODE_FAILED;
            callback->wlCmdID = WL_CMDCODE_MA_DATA;
            callback->wlResult = p_confirm->resultCode;
            WMSP_ReturnResult2Wm9(callback);
            return;
        }
    }

    callback->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9(callback);

    return;
}
