#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_SetParentParam(OSMessage msg)
{
    u32 *reqBuf = (u32 *)msg;
    WMStatus *status = WMSP_GetStatusStructure();
    u32 wlBuf[128];
    WMCallback *cb;
    WMParentParam *param;
    WlCmdCfm *pConfirm;

    param = (WMParentParam *)(reqBuf[1]);

    MI_CpuCopy8((void *)param, (void *)&(status->pparam), WM_PARENT_PARAM_SIZE);

    {
        u16 parentChannel;

        parentChannel = status->pparam.channel;
        if (!((0x0001 << parentChannel) & status->enableChannel)) {
            cb = WMSP_GetBuffer4Callback2Wm9();
            cb->apiid = WM_APIID_SET_P_PARAM;
            cb->errcode = WM_ERRCODE_INVALID_PARAM;
            WMSP_ReturnResult2Wm9(cb);
            return;
        }
    }

    pConfirm = (WlCmdCfm *)WMSP_WL_ParamSetMaxConnectableChild((u16 *)wlBuf,
        status->pparam.maxEntry);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_PARAM_SET_MAXCONN, pConfirm->resultCode);
        return;
    }

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_SET_P_PARAM;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);

    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_SET_P_PARAM;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}

