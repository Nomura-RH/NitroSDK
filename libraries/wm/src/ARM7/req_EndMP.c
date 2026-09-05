#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_EndMP(OSMessage msg)
{
    WMStatus *status = WMSP_GetStatusStructure();

    u32 wlBuf[128];
    WlCmdCfm *pConfirm;

    BOOL fCleanQueue = FALSE;

    if (status->state != WM_STATE_MP_PARENT && status->state != WM_STATE_MP_CHILD) {
        WMCallback *callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_END_MP;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    {
        OSIntrMode e = OS_DisableInterrupts();

        if (status->mp_flag == TRUE) {
            fCleanQueue = TRUE;
        }
        status->mp_flag = FALSE;
        WMSP_CancelVAlarm();

        WMSP_SetThreadPriorityLow();

        if (status->state == WM_STATE_MP_CHILD) {
            status->state = WM_STATE_CHILD;
        } else if (status->state == WM_STATE_MP_PARENT) {
            status->state = WM_STATE_PARENT;
        }

        (void)OS_RestoreInterrupts(e);
    }

    pConfirm = (WlCmdCfm *)WMSP_WL_ParamSetNullKeyResponseMode((u16 *)wlBuf, 0);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_PARAM_SET_NULLKEYMODE, pConfirm->resultCode);
        return;
    }

    pConfirm = (WlCmdCfm *)WMSP_WL_MaClearData((u16 *)wlBuf, 0x0007);

    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_MA_CLRDATA, pConfirm->resultCode);
        return;
    }
    status->mp_setDataFlag = FALSE;

#ifdef WM_CLEAN_SEND_QUEUE
    if (fCleanQueue) {
        WMSP_CleanSendQueue(0xffff);
    }
#endif

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_END_MP;
        cb->errcode = WM_ERRCODE_SUCCESS;

        WMSP_ReturnResult2Wm9((void *)cb);
    }
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_END_MP;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
