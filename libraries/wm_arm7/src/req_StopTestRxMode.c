#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_StopTestRxMode(OSMessage msg)
{
    WMStopTestRxModeCallback *cb;

#ifdef WM_ENABLE_TESTMODE
#if SDK_VERSION_WL >= 26600
    u32 fcsOk, fcsErr;

    OS_Printf("WMSP_StopTestRxMode\n");

    {
        u32 wlBuf[128];
        WlDevGetWirelessCounterCfm *pConfirm;

        pConfirm = WMSP_WL_DevGetWirelessCounter((u16 *)wlBuf);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_TEST_RX, pConfirm->resultCode);
            return;
        }

        fcsOk = pConfirm->counter.rx.fcsOk;
        fcsErr = pConfirm->counter.rx.fcsErr;
    }

    {
        WMStatus *status = WMSP_GetStatusStructure();
        u32 wlBuf[128];
        WlCmdCfm *pConfirm;

        pConfirm = (WlCmdCfm *)WMSP_WL_DevTestRx((u16 *)wlBuf, WL_CMDLABEL_TEST_RX_OFF,
            1);

        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_TEST_RX, pConfirm->resultCode);
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
#endif

    cb = (WMStopTestRxModeCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_STOP_TESTRXMODE;
#ifdef WM_ENABLE_TESTMODE
#if SDK_VERSION_WL >= 26600
    cb->errcode = WM_ERRCODE_SUCCESS;
    cb->fcsOk = fcsOk;
    cb->fcsErr = fcsErr;
#else
    cb->errcode = WM_ERRCODE_WM_DISABLE;
#endif
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
    cb->apiid = WM_APIID_STOP_TESTRXMODE;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}

