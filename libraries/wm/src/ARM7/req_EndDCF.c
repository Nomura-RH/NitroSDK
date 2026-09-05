#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_EndDCF(OSMessage msg)
{
    WMStatus *status = WMSP_GetStatusStructure();
    OSIntrMode e;

    u32 wlBuf[128];
    WlCmdCfm *pConfirm;

    e = OS_DisableInterrupts();
    if (status->state != WM_STATE_DCF_CHILD) {
        WMCallback *callback;

        (void)OS_RestoreInterrupts(e);
        callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_END_DCF;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    status->dcf_flag = FALSE;
    status->state = WM_STATE_CHILD;

    (void)OS_RestoreInterrupts(e);

    pConfirm = (WlCmdCfm *)WMSP_WL_MaClearData((u16 *)wlBuf,
        (u16)(WL_CMDLABEL_CLRDATA_KEYDATA | WL_CMDLABEL_CLRDATA_MP | WL_CMDLABEL_CLRDATA_DATA));

    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_MA_CLRDATA, pConfirm->resultCode);
        return;
    }

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_END_DCF;
        cb->errcode = WM_ERRCODE_SUCCESS;

        WMSP_ReturnResult2Wm9((void *)cb);
    }
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_END_DCF;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}

