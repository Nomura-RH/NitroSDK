#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_SetLifeTime(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    u32 wlBuf[128];
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    WlParamSetCfm *pConfirm;

    u16 tableNumber, camLifeTime, frameLifeTime, mpLifeTime;

    tableNumber = (u16)(buf[1]);
    camLifeTime = (u16)(buf[2]);
    frameLifeTime = (u16)(buf[3]);

    mpLifeTime = (u16)(buf[4]);

    pConfirm = WMSP_WL_ParamSetLifeTime((u16 *)wlBuf, tableNumber, camLifeTime, frameLifeTime);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_PARAM_SET_LIFETIME, pConfirm->resultCode);
        return;
    }

    if (mpLifeTime != 0xffff) {
        OSTick mpLifeTimeTick;
        if (mpLifeTime == 0) {
            mpLifeTimeTick = 1;
        } else {
            mpLifeTimeTick = OS_MilliSecondsToTicks(mpLifeTime * 100);
        }
        status->mp_lifeTimeTick = mpLifeTimeTick;
    } else {
        status->mp_lifeTimeTick = 0ULL;
    }
    {
        OSTick now = OS_GetTick() | 1;
        int i;
        for (i = 0; i < 1 + WM_NUM_MAX_CHILD; i++) {
            status->mp_lastRecvTick[i] = now;
        }
    }

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_LIFETIME;
        cb->errcode = WM_ERRCODE_SUCCESS;

        WMSP_ReturnResult2Wm9((void *)cb);
    }
    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_SET_LIFETIME;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
