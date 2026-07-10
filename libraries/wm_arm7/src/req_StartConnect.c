#include "wmsp_private.h"

#define WMSP_JOIN_TIMEOUT           2000
#define WMSP_AUTHENTICATE_TIMEOUT   2000
#define WMSP_ASSOCIATE_TIMEOUT      2000
#define DEBUG_MLME_START_CONST_SSID 0

#if DEBUG_MLME_START_CONST_SSID
static const u8 c_ssid[32] = "TEST0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#endif

static void WmspError(u16 wlCommand, u16 wlResult, u16 wlStatus);

void WMSP_StartConnectEx(OSMessage msg)
{
    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    WMSPWork *const work = WMSP_GetSystemWork();
    WMArm7Buf *const p = work->wm7buf;
    WMStatus *status = WMSP_GetStatusStructure();

    WMStartConnectReq *args = (WMStartConnectReq *)msg;
    WlMlmeAssociateCfm *assConfirm;

    if (status->state != WM_STATE_IDLE || (status->miscFlags & 1)) {
        WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_CONNECT;
        callback->errcode = WM_ERRCODE_ILLEGAL_STATE;
        callback->state = WM_STATECODE_CONNECT_START;
        WMSP_ReturnResult2Wm9(callback);
        return;
    }

#ifdef WM_DEBUG_HEAP
    WMSP_CheckWLHeap("WMSP_StartConnect");
#endif

    MI_CpuCopy8((void *)(args->pInfo), &(p->connectPInfo), WM_BSS_DESC_SIZE);

    {
        if (p->connectPInfo.gameInfoLength >= 16) {
            if (!(p->connectPInfo.gameInfo.attribute & WM_ATTR_FLAG_ENTRY)) {
                WMStartConnectCallback *cb = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
                cb->apiid = WM_APIID_START_CONNECT;
                cb->errcode = WM_ERRCODE_NO_ENTRY;
                cb->state = WM_STATECODE_CONNECT_START;
                WMSP_ReturnResult2Wm9(cb);
                return;
            }
        }
    }

    {
        u16 connectChannel;

        connectChannel = p->connectPInfo.channel;
        if ((!((0x0001 << connectChannel) & status->enableChannel)) || (!(((0x0001 << connectChannel) >> 1) & WMSP_ENABLE_CHANNEL_MASK))) {
            WMStartConnectCallback *cb = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
            cb->apiid = WM_APIID_START_CONNECT;
            cb->errcode = WM_ERRCODE_INVALID_PARAM;
            cb->state = WM_STATECODE_CONNECT_START;
            WMSP_ReturnResult2Wm9(cb);
            return;
        }
    }

    {
        WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
        callback->apiid = WM_APIID_START_CONNECT;
        callback->errcode = WM_ERRCODE_SUCCESS;
        callback->state = WM_STATECODE_CONNECT_START;
        (void)WMSP_ReturnResult2Wm9((void *)callback);
    }

    {
        if (status->rate == WL_CMDLABEL_RATE_1M) {
            if (p->connectPInfo.rateSet.basic & 0x0001) {
                status->rate = WL_CMDLABEL_RATE_1M;
            } else {
                status->rate = WL_CMDLABEL_RATE_2M;
            }
        } else {
            if (p->connectPInfo.rateSet.basic & 0x0002) {
                status->rate = WL_CMDLABEL_RATE_2M;
            } else {
                status->rate = WL_CMDLABEL_RATE_1M;
            }
        }
        if (p->connectPInfo.capaInfo & 0x0020) {
            status->preamble = WL_CMDLABEL_PREAMBLE_SHORT;
        } else {
            status->preamble = WL_CMDLABEL_PREAMBLE_LONG;
        }
        if (p->connectPInfo.gameInfoLength == 0) {
            status->mode = WL_CMDLABEL_MODE_HOTSPOT;
        } else {
            status->mode = WL_CMDLABEL_MODE_CHILD;
        }

        if (!WMSP_SetAllParams(WM_APIID_START_CONNECT, buf)) {
            return;
        }
    }

    {
        WlCmdCfm *p_confirm;
        p_confirm = (WlCmdCfm *)WMSP_WL_ParamSetNullKeyResponseMode(buf, 0);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_PARAM_SET_NULLKEYMODE, p_confirm->resultCode, 0 /* statusCode³µ */);
            return;
        }
    }

    {
#define WMSP_BEACONLOST_TH_MAX    255
#define WMSP_BEACONLOST_TH_SECOND 10

        if (p->connectPInfo.gameInfoLength < 16) {
            u16 beaconLostTh;
            WlParamSetCfm *p_confirm;

            beaconLostTh = (u16)(p->connectPInfo.beaconPeriod > 0 ? (WMSP_BEACONLOST_TH_SECOND * 1000 / p->connectPInfo.beaconPeriod) + 1 : 1);
            beaconLostTh = (beaconLostTh > (u16)WMSP_BEACONLOST_TH_MAX) ? (u16)WMSP_BEACONLOST_TH_MAX : beaconLostTh;

            p_confirm = WMSP_WL_ParamSetBeaconLostThreshold(buf, beaconLostTh);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_PARAM_SET_BEACON_LOST, p_confirm->resultCode, 0 /* statusCode³µ */);
                return;
            }
        }
    }

    {
        WlDevClass1Cfm *p_confirm;

        p_confirm = WMSP_WL_DevClass1(buf);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_CLASS1, p_confirm->resultCode, 0 /* statusCode³µ */);
            return;
        }
    }

    status->state = WM_STATE_CLASS1;

    {
        WlMlmePowerManagementCfm *p_confirm;

        u16 pwrMgtMode;
        u16 wakeUp;
        u16 recieveDtims;

        pwrMgtMode = (u16)(args->powerSave ? WL_CMDLABEL_PMG_PS : WL_CMDLABEL_PMG_CONT_ACT);
        wakeUp = WL_CMDLABEL_WAKEUP_NORMAL;
        recieveDtims = WL_CMDLABEL_RX_ALL_DTIM;

        p_confirm = WMSP_WL_MlmePowerManagement(buf, pwrMgtMode, wakeUp, recieveDtims);
        if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_MLME_PWRMGT, p_confirm->resultCode, 0 /* statusCode³µ */);
            return;
        }
        status->pwrMgtMode = pwrMgtMode;
    }

    {
        WlMlmeJoinCfm *p_confirm;

        u16 timeOut;
        WlBssDesc bss_desc;

        enum {
            BSS_DESC_BLIT_SIZE = 64
        };
        SDK_COMPILER_ASSERT(BSS_DESC_BLIT_SIZE == sizeof(bss_desc) - sizeof(bss_desc.gameInfo));

        timeOut = WMSP_JOIN_TIMEOUT;
        MI_CpuCopy8(&p->connectPInfo, &bss_desc, BSS_DESC_BLIT_SIZE);

        if (status->mode == WL_CMDLABEL_MODE_CHILD) {
#if DEBUG_MLME_START_CONST_SSID
            bss_desc.ssidLength = 5;
            MI_CpuCopy8(c_ssid, &(bss_desc.ssid[0]), 32);
#else

#ifdef WM_SCAN_SSID_FILTER_ON
#else
            bss_desc.ssidLength = 32;
            *(u16 *)&(bss_desc.ssid[0]) = (u16)(p->connectPInfo.gameInfo.ggid & 0x0ffff);
            *(u16 *)&(bss_desc.ssid[2]) = (u16)(p->connectPInfo.gameInfo.ggid >> 16);
            *(u16 *)&(bss_desc.ssid[4]) = p->connectPInfo.gameInfo.tgid;
            *(u16 *)&(bss_desc.ssid[6]) = 0;

            MI_CpuCopy8(args->ssid, &(bss_desc.ssid[8]), WM_SIZE_CHILD_SSID);

#endif

#endif
        }

        p_confirm = WMSP_WL_MlmeJoin(buf, timeOut, &bss_desc);
        if ((p_confirm->resultCode != WL_CMDRES_SUCCESS) || (p_confirm->statusCode != WL_CMDLABEL_STS_SUCCESSFUL)) {
            WmspError(WL_CMDCODE_MLME_JOIN, p_confirm->resultCode, p_confirm->statusCode);
            return;
        }
        MI_CpuCopy8(p_confirm->peerMacAdrs, status->parentMacAddress, WM_SIZE_MACADDR);
    }

    {
        WlMlmeAuthenticateCfm *p_confirm;

        u16 algorithm;
        u16 timeOut;
        u16 mac[3];

        MI_CpuCopy8(status->parentMacAddress, mac, WM_SIZE_MACADDR);

        algorithm = args->authMode;

        timeOut = WMSP_AUTHENTICATE_TIMEOUT;

        p_confirm = WMSP_WL_MlmeAuthenticate(buf, mac, algorithm, timeOut);

        if ((p_confirm->resultCode == WL_CMDRES_FAILURE) && ((p_confirm->statusCode == WL_CMDLABEL_STS_NO_ENTRY))) {
            WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
            callback->apiid = WM_APIID_START_CONNECT;
            callback->errcode = WM_ERRCODE_OVER_MAX_ENTRY;
            callback->state = WM_STATECODE_CONNECT_START;
            WMSP_ReturnResult2Wm9((void *)callback);
            return;
        }

        if ((p_confirm->resultCode != WL_CMDRES_SUCCESS) || (p_confirm->statusCode != WL_CMDLABEL_STS_SUCCESSFUL)) {
            WmspError(WL_CMDCODE_MLME_AUTH, p_confirm->resultCode, p_confirm->statusCode);
            return;
        }
    }

    {
        u16 mac[3];
        u16 listenInterval;
        u16 timeOut;

        MI_CpuCopy8(status->parentMacAddress, mac, WM_SIZE_MACADDR);
        listenInterval = 1;
        timeOut = WMSP_ASSOCIATE_TIMEOUT;
        assConfirm = WMSP_WL_MlmeAssociate(buf, mac, listenInterval, timeOut);
    }

    {
        OSIntrMode e = OS_DisableInterrupts();

        if ((assConfirm->resultCode == WL_CMDRES_FAILURE) && ((assConfirm->statusCode == WL_CMDLABEL_STS_NO_ENTRY))) {
            WMStartConnectCallback *callback;

            (void)OS_RestoreInterrupts(e);
            callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
            callback->apiid = WM_APIID_START_CONNECT;
            callback->errcode = WM_ERRCODE_OVER_MAX_ENTRY;
            callback->state = WM_STATECODE_CONNECT_START;
            WMSP_ReturnResult2Wm9((void *)callback);
            return;
        }

        if ((assConfirm->resultCode != WL_CMDRES_SUCCESS) || (assConfirm->statusCode != WL_CMDLABEL_STS_SUCCESSFUL)) {
            (void)OS_RestoreInterrupts(e);
            WmspError(WL_CMDCODE_MLME_ASS, assConfirm->resultCode, assConfirm->statusCode);
            return;
        }

        status->aid = assConfirm->aid;
        status->curr_tgid = p->connectPInfo.gameInfo.tgid;

        MI_CpuFill16(status->portSeqNo[0], 0x0001, sizeof(status->portSeqNo[0]));

        {
            u8 tempRssi8;
            u16 linkLevel;

            tempRssi8 = WMSP_GetRssi8((u8)p->connectPInfo.rssi);
            linkLevel = WMSP_GetLinkLevel(tempRssi8);
            status->linkLevel = linkLevel;
            WMSP_FillRssiIntoList(tempRssi8);
        }

        {
            OSIntrMode e = OS_DisableInterrupts();

            status->child_bitmap = 0x0001;
            status->mp_readyBitmap = 0x0001;
            if (status->mp_lifeTimeTick != 0ULL) {
                status->mp_lastRecvTick[0] = OS_GetTick() | 1;
            }

            status->state = WM_STATE_CHILD;

            WMSP_SetParentMaxSize((u16)(p->connectPInfo.gameInfo.parentMaxSize
                + ((p->connectPInfo.gameInfo.attribute & WM_ATTR_FLAG_KS) ? WM_SIZE_KS_PARENT_DATA + WM_SIZE_MP_PARENT_PADDING : 0)));
            WMSP_SetChildMaxSize((u16)(p->connectPInfo.gameInfo.childMaxSize
                + ((p->connectPInfo.gameInfo.attribute & WM_ATTR_FLAG_KS) ? WM_SIZE_KS_CHILD_DATA + WM_SIZE_MP_CHILD_PADDING : 0)));

            (void)OS_RestoreInterrupts(e);
        }

        status->beaconIndicateFlag = 1;

        {
            WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
            callback->apiid = WM_APIID_START_CONNECT;
            callback->errcode = WM_ERRCODE_SUCCESS;
            callback->state = WM_STATECODE_CONNECTED;
            callback->aid = status->aid;
            MI_CpuCopy8(status->parentMacAddress, callback->macAddress, WM_SIZE_MACADDR);
            callback->parentSize = status->mp_parentSize;
            callback->childSize = status->mp_childSize;
            WMSP_ReturnResult2Wm9((void *)callback);
        }

        (void)OS_RestoreInterrupts(e);
    }

#ifndef WM_CLEAN_SEND_QUEUE
    WMSP_InitSendQueue();
#endif

    return;
}

static void WmspError(u16 wlCommand, u16 wlResult, u16 wlStatus)
{
    WMStartConnectCallback *callback = (WMStartConnectCallback *)WMSP_GetBuffer4Callback2Wm9();
    callback->apiid = WM_APIID_START_CONNECT;
    callback->errcode = WM_ERRCODE_FAILED;
    callback->wlCmdID = wlCommand;
    callback->wlResult = wlResult;
    callback->wlStatus = wlStatus;
    WMSP_ReturnResult2Wm9(callback);
}

