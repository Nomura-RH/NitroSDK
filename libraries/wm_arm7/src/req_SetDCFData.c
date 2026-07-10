#include "wmsp_private.h"

void WMSP_SetDCFData(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    WMStatus *status = WMSP_GetStatusStructure();

    u32 wlBuf[128];
    WlTxFrame wlTxFrame;
    WlMaDataCfm *wlResult;

    MI_CpuCopy8(&buf[1], status->dcf_destAdr, 6);
    status->dcf_sendData = (u16 *)buf[3];
    status->dcf_sendSize = (u16)buf[4];
    status->dcf_sendFlag = TRUE;

    MI_CpuClear16(&wlTxFrame, sizeof(wlTxFrame));
    wlTxFrame.frameId = (u16)0;
    wlTxFrame.length = (u16)buf[4];
    MI_WriteByte(&wlTxFrame.rate,
        (u8)((status->rate == WL_CMDLABEL_RATE_2M) ? WL_RATE_2MBPS : WL_RATE_1MBPS));
    MI_CpuCopy8(&buf[1], wlTxFrame.destAdrs, WM_SIZE_MACADDR);
    MI_CpuCopy8(status->MacAddress, wlTxFrame.srcAdrs, WM_SIZE_MACADDR);
    wlTxFrame.datap = (u16 *)buf[3];

    wlResult = WMSP_WL_MaData((u16 *)wlBuf, &wlTxFrame);

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_DCF_DATA;
        cb->errcode = (wlResult->resultCode == WL_CMDRES_SUCCESS) ? WM_ERRCODE_SUCCESS : WM_ERRCODE_FAILED;
        if (wlResult->resultCode != WL_CMDRES_SUCCESS) {
            cb->wlCmdID = WL_CMDCODE_MA_DATA;
            cb->wlResult = wlResult->resultCode;
        }

        WMSP_ReturnResult2Wm9((void *)cb);
    }
}
