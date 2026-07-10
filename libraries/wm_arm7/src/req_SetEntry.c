#include "wmsp_private.h"

void WMSP_SetEntry(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    WMStatus *status = WMSP_GetStatusStructure();
    WlParamSetCfm *wlResult;
    u32 wlBuf[128];

    status->pparam.entryFlag = (u16)(buf[1]);

    {
        WMGameInfo GameInfo ATTRIBUTE_ALIGN(32);
        u16 length;

        WMSP_CopyParentParam(&GameInfo, &(status->pparam));
        length = (u16)(WM_SIZE_SYSTEM_GAMEINFO + status->pparam.userGameInfoLength);
        wlResult = WMSP_WL_ParamSetGameInfo((u16 *)wlBuf, length, (u16 *)(&GameInfo));
    }

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_ENTRY;
        if (wlResult->resultCode == WL_CMDRES_SUCCESS) {
            cb->errcode = WM_ERRCODE_SUCCESS;
        } else {
            cb->errcode = WM_ERRCODE_FAILED;
            cb->wlCmdID = WL_CMDCODE_PARAM_SET_GAME_INFO;
            cb->wlResult = wlResult->resultCode;
        }
        WMSP_ReturnResult2Wm9(cb);
    }
}
