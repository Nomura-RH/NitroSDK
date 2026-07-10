#include "wmsp_private.h"

void WMSP_SetGameInfo(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    u8 attribute;
    u16 length;

    u32 wlBuf[128];
    WlParamSetCfm *wlResult;

    status->pparam.userGameInfo = (u16 *)(buf[1]);
    status->pparam.userGameInfoLength = (u16)(buf[2]);
    status->pparam.ggid = buf[3];
    status->pparam.tgid = (u16)(buf[4]);
    attribute = (u8)(buf[5]);

    status->pparam.entryFlag = (u16)((attribute & WM_ATTR_FLAG_ENTRY) ? 1 : 0);
    status->pparam.multiBootFlag = (u16)((attribute & WM_ATTR_FLAG_MB) ? 1 : 0);
    status->pparam.KS_Flag = (u16)((attribute & WM_ATTR_FLAG_KS) ? 1 : 0);
    status->pparam.CS_Flag = (u16)((attribute & WM_ATTR_FLAG_CS) ? 1 : 0);

    {
        WMGameInfo GameInfo ATTRIBUTE_ALIGN(32);

        WMSP_CopyParentParam(&GameInfo, &(status->pparam));
        length = (u16)(WM_SIZE_SYSTEM_GAMEINFO + status->pparam.userGameInfoLength);
        wlResult = WMSP_WL_ParamSetGameInfo((u16 *)wlBuf, length, (u16 *)(&GameInfo));
    }

    {
        WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_GAMEINFO;
        cb->errcode = (wlResult->resultCode == WL_CMDRES_SUCCESS) ? WM_ERRCODE_SUCCESS : WM_ERRCODE_FAILED;
        if (wlResult->resultCode != WL_CMDRES_SUCCESS) {
            cb->wlCmdID = WL_CMDCODE_PARAM_SET_GAME_INFO;
            cb->wlResult = wlResult->resultCode;
        }

        WMSP_ReturnResult2Wm9((void *)cb);
    }
}
