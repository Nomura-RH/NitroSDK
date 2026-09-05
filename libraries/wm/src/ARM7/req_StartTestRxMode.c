#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_StartTestRxMode(OSMessage msg)
{
    WMCallback *cb;
    WMStatus *status = WMSP_GetStatusStructure();

#ifdef WM_ENABLE_TESTMODE
#if SDK_VERSION_WL >= 26600
    OS_Printf("WMSP_StartTestRxMode\n");

    {
        u32 wlBuf[128];
        WlParamSetCfm *pSECConfirm;
        WlParamGetEnableChannelCfm *pGECConfirm;

        pSECConfirm = (WlParamSetCfm *)WMSP_WL_ParamSetEnableChannel((u16 *)wlBuf, 0x7ffe);
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

    {
        u32 wlBuf[128];
        WlCmdCfm *pConfirm;
        WMStartTestRxModeReq *args = (WMStartTestRxModeReq *)msg;


        pConfirm = (WlCmdCfm *)WMSP_WL_DevTestRx((u16 *)wlBuf, WL_CMDLABEL_TEST_RX_ON,
            args->channel
        );

        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_TEST_RX, pConfirm->resultCode);
            return;
        }

        if (status->state != WM_STATE_TESTMODE_RX) {
            status->state = WM_STATE_TESTMODE_RX;

            pConfirm = (WlCmdCfm *)WMSP_WL_ParamSetMode((u16 *)wlBuf, WL_CMDLABEL_MODE_TEST);
            if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_PARAM_SET_MODE, pConfirm->resultCode);
                return;
            }
        }

        {
            WlDevSetInitializeWirelessCounterCfm *pConfirm;
            u32 wlBuf[128];

            OS_Printf("Init Wireless Counter\n");
            pConfirm = WMSP_WL_DevSetInitializeWirelessCounter((u16 *)wlBuf);
            if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_DEV_INIT_INFO, pConfirm->resultCode);
                return;
            }
        }
    }
#endif
#endif

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_START_TESTRXMODE;
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
    cb->apiid = WM_APIID_START_TESTRXMODE;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
