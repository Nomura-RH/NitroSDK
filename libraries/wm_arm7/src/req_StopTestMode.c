#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_StopTestMode(OSMessage msg)
{
    WMCallback *cb;

#ifdef WM_ENABLE_TESTMODE
    {
        WMStatus *status = WMSP_GetStatusStructure();
        u32 wlBuf[128];
        WlCmdCfm *pConfirm;

        pConfirm = (WlCmdCfm *)WMSP_WL_DevTestSignal((u16 *)wlBuf, WL_CMDLABEL_TEST_SIGNAL_OFF,
            0,
            WL_CMDLABEL_TEST_SIGNAL_2M,
            1);

        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_TEST_SIGNAL, pConfirm->resultCode);
            return;
        }
        status->state = WM_STATE_IDLE;
    }

    {
        u32 wlBuf[128];
        WlParamSetCfm *pSECConfirm;
        WlParamGetEnableChannelCfm *pGECConfirm;

        pSECConfirm = (WlParamSetCfm *)WMSP_WL_ParamSetEnableChannel((u16 *)wlBuf, 0x3ffe);
        if (pSECConfirm->resultCode != WL_CMDRES_SUCCESS) {
            OS_Printf("SetEnableChannel Error!\n");
            WmspError(WL_CMDCODE_PARAM_SET_ENABLECHANNEL, pSECConfirm->resultCode);
            return;
        }

        pGECConfirm = (WlParamGetEnableChannelCfm *)WMSP_WL_ParamGetEnableChannel((u16 *)wlBuf);
        if (pGECConfirm->resultCode != WL_CMDRES_SUCCESS) {
            OS_Printf("GetEnableChannel Error!\n");
            WmspError(WL_CMDCODE_PARAM_GET_ENABLECHANNEL, pGECConfirm->resultCode);
            return;
        }

        OS_Printf("EnableChannel = 0x%x\n", pGECConfirm->enableChannel);
    }

#endif

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_STOP_TESTMODE;
#ifdef WM_ENABLE_TESTMODE
    cb->errcode = WM_ERRCODE_SUCCESS;
#else
    cb->errcode = WM_ERRCODE_WM_DISABLE;
#endif
    WMSP_ReturnResult2Wm9((void *)cb);
    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_STOP_TESTMODE;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
