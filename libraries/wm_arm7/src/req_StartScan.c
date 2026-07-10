#include "wmsp_private.h"

enum {
    SCAN_CHANNEL_MAX = 16
};

static u32 WmspGetScanExBufSize4WL(u16 arm9_BufSize);
static void WmspError(u16 wlCommand, u16 wlResult, BOOL exFlag);
static BOOL IsIncludeValidSSID(WMBssDesc *bssDesc);

void WMSP_StartScan(OSMessage msg)
{
    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    WMSPWork *const work = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    u16 current_state;
    u16 scanChannel;
    u16 scanBssid[3];
    u16 scanMaxChannelTime;

    WMStartScanReq *args = (WMStartScanReq *)msg;

    if (status->state != WM_STATE_IDLE && status->state != WM_STATE_CLASS1 && status->state != WM_STATE_SCAN) {
        WMStartScanCallback *callback = (WMStartScanCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_SCAN;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        callback->state = WM_STATECODE_PARENT_NOT_FOUND;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

#ifdef WM_DEBUG_HEAP
    WMSP_CheckWLHeap("WMSP_StartScan");
#endif

    status->pInfoBuf = args->scanBuf;
    status->scan_channel = scanChannel = args->channel;
    scanMaxChannelTime = args->maxChannelTime;
    MI_CpuCopy8(args->bssid, scanBssid, WM_SIZE_MACADDR);

    if ((scanBssid[0] != 0xffff) && (scanBssid[0] & 0x0001)) {
        OS_Printf("[ARM7] WM_StartScan: assigned Bssid is MulticastAddress. LSB is cleared.\n");
        scanBssid[0] &= ~(0x0001);
    }

    if (!scanChannel) {
        WMStartScanCallback *callback = (WMStartScanCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_SCAN;
        callback->errcode = WM_ERRCODE_INVALID_PARAM;
        callback->state = WM_STATECODE_PARENT_NOT_FOUND;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    if (!((0x0001 << scanChannel) & status->enableChannel)) {
        WMStartScanCallback *cb = (WMStartScanCallback *)WMSP_GetBuffer4Callback2Wm9();
        cb->apiid = WM_APIID_START_SCAN;
        cb->errcode = WM_ERRCODE_INVALID_PARAM;
        cb->state = WM_STATECODE_PARENT_NOT_FOUND;
        WMSP_ReturnResult2Wm9(cb);
        return;
    }

    status->mode = WL_CMDLABEL_MODE_CHILD;

    {

        WlDevGetStationStateCfm *p_confirm;
        p_confirm = WMSP_WL_DevGetStationState(buf);

        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_GET_STATE, p_confirm->resultCode, FALSE);
            return;
        }
        current_state = p_confirm->state;
    }

    if (current_state == WL_STA_IDLE) {
        if (!WMSP_SetAllParams(WM_APIID_START_SCAN, buf)) {
            return;
        }

        {
            WlDevClass1Cfm *p_confirm;
            p_confirm = WMSP_WL_DevClass1(buf);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_DEV_CLASS1, p_confirm->resultCode, FALSE);
                return;
            }
        }

        status->state = WM_STATE_CLASS1;

        {
            WlMlmePowerManagementCfm *p_confirm;

            u16 pwrMgtMode;
            u16 wakeUp;
            u16 recieveDtims;

            pwrMgtMode = WL_CMDLABEL_PMG_PS;
            wakeUp = WL_CMDLABEL_WAKEUP_NORMAL;
            recieveDtims = WL_CMDLABEL_RX_ALL_DTIM;

            p_confirm = WMSP_WL_MlmePowerManagement(buf, pwrMgtMode, wakeUp, recieveDtims);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_MLME_PWRMGT, p_confirm->resultCode, FALSE);
                return;
            }
            status->pwrMgtMode = pwrMgtMode;
        }
        current_state = WL_STA_CLASS1;
    }

    status->state = WM_STATE_SCAN;

    {
        WlMlmeScanCfm *p_confirm;
        WlMlmeScanReq *pReq = (WlMlmeScanReq *)buf;

        u32 bufSize;
        u16 ssidLength;
        u16 ssid[WM_SIZE_SSID / sizeof(u16)];
        u16 scanType;
        u8 channelList[SCAN_CHANNEL_MAX];

#ifdef WM_SCAN_SSID_FILTER_ON
        ssidLength = 2;
        MI_CpuFill16(ssid, 0x0000, WM_SIZE_SSID);
        ssid[0] = 0x424d;
#else
        ssidLength = 0;
        MI_CpuFill16(ssid, 0xffff, WM_SIZE_SSID);
#endif

        scanType = WL_CMDLABEL_SCAN_PASSIVE;

        MI_WriteByte(&channelList[0], (u8)scanChannel);
        MI_CpuFill8(&channelList[1], 0, sizeof(*channelList) * (SCAN_CHANNEL_MAX - 1));
        bufSize = (((sizeof(WlMlmeScanReq) + 1) / 2 * 2) + ((sizeof(WlCmdHeader) + 1) / 2 * 2) + 6 +
            ((64 + (WM_SIZE_GAMEINFO + 1) / 2 * 2) * 1));
        p_confirm = WMSP_WL_MlmeScan(buf,
            bufSize,
            scanBssid,
            ssidLength,
            (u8 *)ssid,
            scanType,
            channelList,
            scanMaxChannelTime);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_MLME_SCAN, p_confirm->resultCode, FALSE);
            return;
        } else {
            WMStartScanCallback *callback = (WMStartScanCallback *)WMSP_GetBuffer4Callback2Wm9();
            if (p_confirm->bssDescCount == 0) {
                callback->apiid = WM_APIID_START_SCAN;
                callback->errcode = WM_ERRCODE_SUCCESS;
                callback->state = WM_STATECODE_PARENT_NOT_FOUND;
                callback->channel = scanChannel;
                callback->linkLevel = WM_LINK_LEVEL_0;
            } else {
                MI_CpuClear16(&(status->pInfoBuf->gameInfo), sizeof(WMGameInfo));
                MI_CpuCopy8(&(p_confirm->bssDescList[0]),
                    status->pInfoBuf,
                    p_confirm->bssDescList[0].length * 2U);

                callback->apiid = WM_APIID_START_SCAN;
                callback->errcode = WM_ERRCODE_SUCCESS;
                callback->state = WM_STATECODE_PARENT_FOUND;
                callback->channel = p_confirm->bssDescList[0].channel;
                {
                    u8 tempRssi;

                    tempRssi = WMSP_GetRssi8((u8)p_confirm->bssDescList[0].rssi);
                    callback->linkLevel = WMSP_GetLinkLevel(tempRssi);
                    WMSP_AddRssiToRandomPool(tempRssi);
                }
                callback->ssidLength = p_confirm->bssDescList[0].ssidLength;
                MI_CpuCopy8(p_confirm->bssDescList[0].bssid, callback->macAddress, WM_SIZE_MACADDR);

                MI_CpuCopy16(&(p_confirm->bssDescList[0].ssid), &(callback->ssid), 32);

                callback->gameInfoLength = p_confirm->bssDescList[0].gameInfoLength;

                if (callback->gameInfoLength > 128) {
                    callback->apiid = WM_APIID_START_SCAN;
                    callback->errcode = WM_ERRCODE_SUCCESS;
                    callback->state = WM_STATECODE_PARENT_NOT_FOUND;
                    callback->channel = scanChannel;
                    callback->linkLevel = WM_LINK_LEVEL_0;
                } else {
                    MI_CpuClear16(&(callback->gameInfo), sizeof(WMGameInfo));
                    MI_CpuCopy16(&(p_confirm->bssDescList[0].gameInfo),
                        &(callback->gameInfo),
                        ((callback->gameInfoLength + 1U) & ~1U));
                }
            }
            WMSP_ReturnResult2Wm9(callback);
        }
    }

    return;
}

void WMSP_StartScanEx(OSMessage msg)
{
    u32 wlBuf[WM_SIZE_SCAN_EX_BUF / sizeof(u32) + (sizeof(WlMlmeScanReq) + 1) / sizeof(u32) + (sizeof(WlCmdHeader) + 1) / sizeof(u32)];
    u16 *buf = (u16 *)wlBuf;
    WMSPWork *const work = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    u16 current_state;
    u16 scanChannelList;
    u16 scanBssid[3];
    u16 scanMaxChannelTime;
    u16 scanType;
#if WM_SSID_MASK_CUSTOMIZE
    u16 ssidMatchLength;
    BOOL ssidMaskCustomize;
#endif
    u16 scanSsidLength;
    u8 scanSsid[WM_SIZE_SSID];
    u16 scanBufSize;

    WMStartScanExReq *args = (WMStartScanExReq *)msg;

    if (status->state != WM_STATE_IDLE && status->state != WM_STATE_CLASS1 && status->state != WM_STATE_SCAN) {
        WMStartScanExCallback *callback = (WMStartScanExCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_SCAN_EX;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        callback->state = WM_STATECODE_PARENT_NOT_FOUND;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    status->pInfoBuf = args->scanBuf;
    status->scan_channel = scanChannelList = args->channelList;
    scanMaxChannelTime = args->maxChannelTime;
    MI_CpuCopy8(args->bssid, scanBssid, WM_SIZE_MACADDR);
#if WM_SSID_MASK_CUSTOMIZE
    ssidMatchLength = args->ssidMatchLength;
    switch (args->scanType) {
    case WM_SCANTYPE_ACTIVE_CUSTOM:
        ssidMaskCustomize = TRUE;
        scanType = WM_SCANTYPE_ACTIVE;
        break;
    case WM_SCANTYPE_PASSIVE_CUSTOM:
        ssidMaskCustomize = TRUE;
        scanType = WM_SCANTYPE_PASSIVE;
        break;
    default:
        ssidMaskCustomize = FALSE;
        scanType = args->scanType;
    }
#else
    scanType = args->scanType;
#endif
    scanSsidLength = args->ssidLength;
    MI_CpuCopy8(args->ssid, scanSsid, WM_SIZE_SSID);
    scanBufSize = args->scanBufSize;

    if ((scanBssid[0] != 0xffff) && (scanBssid[0] & 0x0001)) {
        OS_Printf("[ARM7] WM_StartScanEx: assigned Bssid is MulticastAddress. LSB is cleared.\n");
        scanBssid[0] &= ~(0x0001);
    }

    scanChannelList <<= 1;
    scanChannelList &= status->enableChannel;

    if (!scanChannelList
            || ((status->miscFlags & 1) && scanType != WM_SCANTYPE_PASSIVE)
            || (args->scanBuf == NULL)
            || ((u32)(args->scanBuf) & 0x00000003)
            || (args->scanBufSize < (sizeof(WlBssDesc) - 4))) {
        WMStartScanExCallback *callback = (WMStartScanExCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_SCAN_EX;
        callback->errcode = WM_ERRCODE_INVALID_PARAM;
        callback->state = WM_STATECODE_PARENT_NOT_FOUND;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

    status->mode = WL_CMDLABEL_MODE_CHILD;

    {

        WlDevGetStationStateCfm *p_confirm;
        p_confirm = WMSP_WL_DevGetStationState(buf);

        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_GET_STATE, p_confirm->resultCode, TRUE);
            return;
        }
        current_state = p_confirm->state;
    }

    if (current_state == WL_STA_IDLE) {
        if (!WMSP_SetAllParams(WM_APIID_START_SCAN_EX, buf)) {
            return;
        }

        {
            WlDevClass1Cfm *p_confirm;
            p_confirm = WMSP_WL_DevClass1(buf);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_DEV_CLASS1, p_confirm->resultCode, TRUE);
                return;
            }
        }

        status->state = WM_STATE_CLASS1;

        {
            WlMlmePowerManagementCfm *p_confirm;

            u16 pwrMgtMode;
            u16 wakeUp;
            u16 recieveDtims;

            pwrMgtMode = WL_CMDLABEL_PMG_PS;
            wakeUp = WL_CMDLABEL_WAKEUP_NORMAL;
            recieveDtims = WL_CMDLABEL_RX_ALL_DTIM;

            p_confirm = WMSP_WL_MlmePowerManagement(buf, pwrMgtMode, wakeUp, recieveDtims);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_MLME_PWRMGT, p_confirm->resultCode, TRUE);
                return;
            }
            status->pwrMgtMode = pwrMgtMode;
        }
        current_state = WL_STA_CLASS1;
    }

    if (scanType == WL_CMDLABEL_SCAN_ACTIVE) {
        if (status->preamble == WL_CMDLABEL_PREAMBLE_SHORT) {
            WlParamSetCfm *p_confirm;

            p_confirm = WMSP_WL_ParamSetPreambleType(buf, WL_CMDLABEL_PREAMBLE_LONG);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_PARAM_SET_PREAMBLE_TYPE, p_confirm->resultCode, TRUE);
                return;
            }
            status->preamble = WL_CMDLABEL_PREAMBLE_LONG;
        }
    } else {
        if (status->preamble == WL_CMDLABEL_PREAMBLE_LONG) {
            WlParamSetCfm *p_confirm;

            p_confirm = WMSP_WL_ParamSetPreambleType(buf, WL_CMDLABEL_PREAMBLE_SHORT);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_PARAM_SET_PREAMBLE_TYPE, p_confirm->resultCode, TRUE);
                return;
            }
            status->preamble = WL_CMDLABEL_PREAMBLE_SHORT;
        }
    }

#if WM_SSID_MASK_CUSTOMIZE
    if (ssidMaskCustomize == TRUE) {
        u8 mask[32];
        WlParamSetCfm *p_confirm;

        MI_CpuFill8(mask, 0xff, 32);
        if (ssidMatchLength <= 32) {
            MI_CpuFill8(mask, 0x00, ssidMatchLength);
        }
        p_confirm = WMSP_WL_ParamSetSsidMask(buf, mask);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_PARAM_SET_SSID_MASK, p_confirm->resultCode, TRUE);
            return;
        }
    }
#endif

    status->state = WM_STATE_SCAN;

    {
        WlMlmeScanCfm *p_confirm;
        WlMlmeScanReq *pReq = (WlMlmeScanReq *)buf;

        u32 bufSize;
        u8 channelList[SCAN_CHANNEL_MAX];
        u16 i, j;

        j = 0;
        MI_CpuFill8(channelList, 0, sizeof(*channelList) * SCAN_CHANNEL_MAX);
        for (i = 1; i < 15; ++i)
        {
            if (scanChannelList & (0x0001 << i)) {
                MI_WriteByte(&channelList[j], (u8)i);
                ++j;
            }
        }

        bufSize = WmspGetScanExBufSize4WL(scanBufSize);

        p_confirm = WMSP_WL_MlmeScan(buf,
            bufSize,
            scanBssid,
            scanSsidLength,
            (u8 *)scanSsid,
            scanType,
            channelList,
            scanMaxChannelTime);

        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_MLME_SCAN, p_confirm->resultCode, TRUE);
            return;
        } else {
            WMStartScanExCallback *callback = (WMStartScanExCallback *)WMSP_GetBuffer4Callback2Wm9();
            if (p_confirm->bssDescCount == 0) {
                callback->apiid = WM_APIID_START_SCAN_EX;
                callback->errcode = WM_ERRCODE_SUCCESS;
                callback->state = WM_STATECODE_PARENT_NOT_FOUND;
                callback->bssDescCount = 0;
                callback->channelList = (u16)(scanChannelList >> 1);
            } else {
                int i;
                WMBssDesc *src, *dest;

                src = (WMBssDesc *)&(p_confirm->bssDescList[0]);
                dest = status->pInfoBuf;

                MI_CpuClear16(dest, scanBufSize);

                for (i = 0; i < p_confirm->bssDescCount; ++i) {
                    u16 length_byte;

                    length_byte = (u16)(src->length * 2);

                    MI_CpuCopy8(src, dest, length_byte);

                    if (scanSsidLength > 0) {
                        if (!IsIncludeValidSSID(dest)) {
                            dest->ssidLength = scanSsidLength;
                            MI_CpuCopy8(scanSsid, dest->ssid, WM_SIZE_SSID);
                        }
                    }

                    callback->bssDesc[i] = dest;

                    {
                        u8 tempRssi;

                        tempRssi = WMSP_GetRssi8((u8)(src->rssi));
                        callback->linkLevel[i] = WMSP_GetLinkLevel(tempRssi);
                        WMSP_AddRssiToRandomPool(tempRssi);
                    }

                    src = (WMBssDesc *)((u32)src + length_byte);
                    dest = (WMBssDesc *)((u32)dest + length_byte);
                    if ((u32)dest & 0x02) {
                        dest = (WMBssDesc *)(((u32)dest + 2) & ~(0x00000003));
                    }
                }

                callback->apiid = WM_APIID_START_SCAN_EX;
                callback->errcode = WM_ERRCODE_SUCCESS;
                callback->state = WM_STATECODE_PARENT_FOUND;
                callback->bssDescCount = p_confirm->bssDescCount;
                callback->channelList = (u16)(scanChannelList >> 1);
            }
            WMSP_ReturnResult2Wm9(callback);
        }
    }

    return;
}

static BOOL IsIncludeValidSSID(WMBssDesc *bssDesc)
{
    s32 i;

    if (bssDesc->ssidLength == 0) {
        return FALSE;
    }
    if (bssDesc->ssidLength > WM_SIZE_SSID) {
        return FALSE;
    }
    for (i = 0; i < bssDesc->ssidLength; i++) {
        if (bssDesc->ssid[i] != 0x00) {
            return TRUE;
        }
    }
    return FALSE;
}

static u32 WmspGetScanExBufSize4WL(u16 arm9_BufSize)
{
    u32 tempSize;

    tempSize = (u32)(arm9_BufSize - (((arm9_BufSize - (sizeof(WlBssDesc) - 4)) / (sizeof(WlBssDesc) - 2)) * 2));
    return (u32)(tempSize + (((sizeof(WlMlmeScanReq) + 1) / 2) * 2) + (((sizeof(WlCmdHeader) + 1) / 2) * 2) + 6
    );
}

static void WmspError(u16 wlCommand, u16 wlResult, BOOL exFlag)
{
    WMStartScanCallback *callback = (WMStartScanCallback *)WMSP_GetBuffer4Callback2Wm9();
    if (exFlag) {
        callback->apiid = WM_APIID_START_SCAN_EX;
    } else {
        callback->apiid = WM_APIID_START_SCAN;
    }
    callback->errcode = WM_ERRCODE_FAILED;
    callback->state = WM_STATECODE_PARENT_NOT_FOUND;
    callback->wlCmdID = wlCommand;
    callback->wlResult = wlResult;

    WMSP_ReturnResult2Wm9(callback);
}

