#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_SetWEPKey(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    WMStatus *status = WMSP_GetStatusStructure();

    u16 *wepkey;

    status->wepMode = (u16)buf[1];
    switch (status->wepMode) {
    case WL_CMDLABEL_WEP_NO:
        status->wep_flag = FALSE;
        break;
    case WL_CMDLABEL_WEP_RC4_40:
    case WL_CMDLABEL_WEP_RC4_104:
    case WL_CMDLABEL_WEP_RC4_128:
        status->wep_flag = TRUE;
        break;
    default:
        status->wep_flag = FALSE;
    }

    if (status->wep_flag == TRUE) {
        wepkey = (u16 *)buf[2];
        MI_CpuCopy8(wepkey, status->wepKey, WM_SIZE_WEPKEY);
    } else {
        MI_CpuClear8(status->wepKey, WM_SIZE_WEPKEY);
    }

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_WEPKEY;
        cb->errcode = WM_ERRCODE_SUCCESS;

        WMSP_ReturnResult2Wm9((void *)cb);
    }
}

void WMSP_SetWEPKeyEx(OSMessage msg)
{
    u32 *msgbuf = (u32 *)msg;
    WMStatus *status = WMSP_GetStatusStructure();

    u8 *wepkey;

    status->wepMode = (u16)msgbuf[1];
    switch (status->wepMode) {
    case WL_CMDLABEL_WEP_NO:
        status->wep_flag = FALSE;
        break;
    case WL_CMDLABEL_WEP_RC4_40:
    case WL_CMDLABEL_WEP_RC4_104:
    case WL_CMDLABEL_WEP_RC4_128:
        status->wep_flag = TRUE;
        break;
    default:
        status->wep_flag = FALSE;
    }

    if (status->wep_flag == TRUE) {
        wepkey = (u8 *)msgbuf[2];
        MI_CpuCopy8(wepkey, status->wepKey, WM_SIZE_WEPKEY);
    } else {
        MI_CpuClear8(status->wepKey, WM_SIZE_WEPKEY);
    }

    status->wepKeyId = (u16)msgbuf[3];

    {
        WlCmdCfm *pConfirm;
        u32 wlBuf[128];
        u16 *buf = (u16 *)wlBuf;

        pConfirm = (WlCmdCfm *)WMSP_WL_ParamSetWepKeyId(buf, status->wepKeyId);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_PARAM_SET_WEP_KEYID, pConfirm->resultCode);
        }
    }
    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_WEPKEY_EX;
        cb->errcode = WM_ERRCODE_SUCCESS;

        WMSP_ReturnResult2Wm9((void *)cb);
    }
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *callback = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_SET_WEPKEY;
    callback->errcode = WM_ERRCODE_FAILED;
    callback->wlCmdID = wlCommand;
    callback->wlResult = wlResult;
    WMSP_ReturnResult2Wm9(callback);
}

