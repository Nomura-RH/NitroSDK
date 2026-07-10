#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

#ifdef WM_ENABLE_TESTMODE
extern u32 CheckPllLock(void);
extern u32 RF_Read(u32 data);
#endif

void WMSP_StartTestMode(OSMessage msg)
{

    WMStartTestModeCallback *cb;
    WMStatus *status = WMSP_GetStatusStructure();

#ifdef WM_ENABLE_TESTMODE

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
        WMStartTestModeReq *args = (WMStartTestModeReq *)msg;
        u16 rate;

        if (args->rate == 1) {
            rate = WL_CMDLABEL_TEST_SIGNAL_1M;
        } else {
            rate = WL_CMDLABEL_TEST_SIGNAL_2M;
        }

        pConfirm = (WlCmdCfm *)WMSP_WL_DevTestSignal((u16 *)wlBuf, WL_CMDLABEL_TEST_SIGNAL_ON,
            args->signal,
            rate,
            args->channel
        );

        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_TEST_SIGNAL, pConfirm->resultCode);
            return;
        }

        if (status->state != WM_STATE_TESTMODE) {
            status->state = WM_STATE_TESTMODE;

            pConfirm = (WlCmdCfm *)WMSP_WL_ParamSetMode((u16 *)wlBuf, WL_CMDLABEL_MODE_TEST);
            if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_PARAM_SET_MODE, pConfirm->resultCode);
                return;
            }
        }
    }
#endif

    cb = (WMStartTestModeCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_START_TESTMODE;
#ifdef WM_ENABLE_TESTMODE
    cb->errcode = WM_ERRCODE_SUCCESS;
    cb->PllLockCheck = (u16)CheckPllLock();
    {
        if (status->rfVersion == 2) {
            cb->RFadr5 = RF_Read(0x00800000 + (5 << 18));
            cb->RFadr6 = RF_Read(0x00800000 + (6 << 18));
            cb->RFMDflag = 1;
        } else {
            cb->RFMDflag = 0;
        }
    }
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
    cb->apiid = WM_APIID_START_TESTMODE;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
