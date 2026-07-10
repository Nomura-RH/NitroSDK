#include "wmsp_private.h"

#define MLME_START_SS_ID_LENGTH     6
#define MLME_START_BEACON_PERIOD    200
#define MLME_START_DTIM_PERIOD      2
#define MLME_START_BASIC_RATE_SET   3
#define MLME_START_SUPPORT_RATE_SET 3
#define DEBUG_MLME_START_CONST_SSID 0

#if DEBUG_MLME_START_CONST_SSID
static const u8 c_ssid[32] = "TEST0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#endif

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_StartParent(OSMessage msg)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    WMStartParentCallback *cb;
    WlCmdCfm *pConfirm;
    u16 ssidLength;
    BOOL powerSave;

    if (status->state != WM_STATE_IDLE || (status->miscFlags & 1)) {
        WMStartParentCallback *callback = (WMStartParentCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_PARENT;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        callback->state = WM_STATECODE_PARENT_START;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    powerSave = (BOOL)(((u32 *)msg)[1]);

#ifdef WM_DEBUG_HEAP
    WMSP_CheckWLHeap("WMSP_StartParent");
#endif

    if (!(status->allowedChannel & ((0x0001 << status->pparam.channel) >> 1))) {
        cb = WMSP_GetBuffer4Callback2Wm9();
        cb->apiid = WM_APIID_START_PARENT;
        cb->errcode = WM_ERRCODE_INVALID_PARAM;
        cb->state = WM_STATECODE_PARENT_START;
        WMSP_ReturnResult2Wm9(cb);
        return;
    }

    status->mode = WL_CMDLABEL_MODE_PARENT;
    status->aid = 0;
    {
        OSIntrMode e = OS_DisableInterrupts();
        status->child_bitmap = 0;
        status->mp_readyBitmap = 0;
        (void)OS_RestoreInterrupts(e);
    }

    status->preamble = WL_CMDLABEL_PREAMBLE_SHORT;

    if (!WMSP_SetAllParams(WM_APIID_START_PARENT, buf)) {
        return;
    }

    pConfirm = (WlCmdCfm *)WMSP_WL_DevClass1(buf);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_DEV_CLASS1, pConfirm->resultCode);
        return;
    }

    {
        u16 pwrMgtMode;
        pwrMgtMode = (u16)(powerSave ? WL_CMDLABEL_PMG_PS : WL_CMDLABEL_PMG_CONT_ACT);
        pConfirm = (WlCmdCfm *)WMSP_WL_MlmePowerManagement(buf,
            pwrMgtMode,
            WL_CMDLABEL_WAKEUP_NORMAL,
            WL_CMDLABEL_RX_ALL_DTIM);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_MLME_PWRMGT, pConfirm->resultCode);
            return;
        }
        status->pwrMgtMode = pwrMgtMode;
    }

    {
#if DEBUG_MLME_START_CONST_SSID
        u16 d_ssid[16];
#else
        u16 ssid[WM_SIZE_SSID / sizeof(u16)] ATTRIBUTE_ALIGN(2);
#endif

        WMParentParam *pparam;
        WMGameInfo GameInfo ATTRIBUTE_ALIGN(32);

        pparam = &(status->pparam);

        MI_CpuClear16(&GameInfo, sizeof(WMGameInfo));
        WMSP_CopyParentParam(&GameInfo, pparam);

#if DEBUG_MLME_START_CONST_SSID
        MI_CpuCopy8(c_ssid, d_ssid, 32);
#else
        MI_CpuClear16(ssid, sizeof(u8) * WM_SIZE_SSID);

#ifdef WM_SCAN_SSID_FILTER_ON
        ssid[0] = 0x424d;
        ssidLength = 2;
#else
        ssid[0] = (u16)(pparam->ggid & 0x0ffff);
        ssid[1] = (u16)(pparam->ggid >> 16);
        ssid[2] = pparam->tgid;
        ssid[3] = 0;
        ssidLength = 32;
#endif

#endif

        pConfirm = (WlCmdCfm *)WMSP_WL_MlmeStart(buf,
#if DEBUG_MLME_START_CONST_SSID
            5,
            (u8 *)d_ssid,
#else
            ssidLength,
            (u8 *)ssid,
#endif
            pparam->beaconPeriod,
            MLME_START_DTIM_PERIOD,
            pparam->channel,
            MLME_START_BASIC_RATE_SET,
            MLME_START_SUPPORT_RATE_SET,
            (u16)(WM_SIZE_SYSTEM_GAMEINFO
                + pparam->userGameInfoLength),
            &GameInfo);

        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_MLME_START, pConfirm->resultCode);
            return;
        }

        WMSP_SetParentMaxSize((u16)(pparam->parentMaxSize
            + ((pparam->KS_Flag)
                    ? WM_SIZE_KS_PARENT_DATA + WM_SIZE_MP_PARENT_PADDING
                    : 0)));
        WMSP_SetChildMaxSize((u16)(pparam->childMaxSize
            + ((pparam->KS_Flag)
                    ? WM_SIZE_KS_CHILD_DATA + WM_SIZE_MP_CHILD_PADDING
                    : 0)));
    }

    cb = (WMStartParentCallback *)WMSP_GetBuffer4Callback2Wm9();
    status->state = WM_STATE_PARENT;
    cb->apiid = WM_APIID_START_PARENT;
    cb->errcode = WM_ERRCODE_SUCCESS;
    cb->state = WM_STATECODE_PARENT_START;
    cb->parentSize = status->mp_parentSize;
    cb->childSize = status->mp_childSize;
    WMSP_ReturnResult2Wm9((void *)cb);
    status->beaconIndicateFlag = 1;

    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMStartParentCallback *cb;

    cb = (WMStartParentCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_START_PARENT;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->state = WM_STATECODE_PARENT_START;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}
