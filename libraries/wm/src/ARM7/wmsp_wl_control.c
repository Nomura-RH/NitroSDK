#include "wmsp_private.h"

#define WMSPi_FillReserved6(buf) \
    do {                         \
        buf[0] = 0;              \
        buf[1] = 0;              \
        buf[2] = 0;              \
        buf[3] = 0;              \
        buf[4] = 0;              \
        buf[5] = 0;              \
    } while (0)

static WlCmdCfm *WMSPi_WL_NoArg(u16 *buf, u16 code, u16 cfmLength);


WlMlmeResetCfm *WMSP_WL_MlmeReset(u16 *buf, u16 mib)
{
    WlMlmeResetReq *req;
    WlMlmeResetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeResetReq *)buf;

    req->header.code = WL_CMDCODE_MLME_RESET;
    req->header.length = 1;

    req->mib = mib;

    cfm = (WlMlmeResetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmePowerManagementCfm *WMSP_WL_MlmePowerManagement(u16 *buf, u16 pwrMgtMode, u16 wakeUp, u16 recieveDtims)
{
    WlMlmePowerManagementReq *req;
    WlMlmePowerManagementCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmePowerManagementReq *)buf;

    req->header.code = WL_CMDCODE_MLME_PWRMGT;
    req->header.length = 3;

    req->pwrMgtMode = pwrMgtMode;
    req->wakeUp = wakeUp;
    req->recieveDtims = recieveDtims;

    cfm = (WlMlmePowerManagementCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeScanCfm *WMSP_WL_MlmeScan(u16 *buf, u32 bufSize, u16 *bssid, u16 ssidLength, u8 *ssid, u16 scanType, u8 *channelList, u16 maxChannelTime)
{
    WlMlmeScanReq *req;
    WlMlmeScanCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeScanReq *)buf;

    req->header.code = WL_CMDCODE_MLME_SCAN;

#if SDK_VERSION_WL >= 20300
    req->header.length = 31;
#else
    req->header.length = 30;
#endif

    MI_CpuCopy16(bssid, req->bssid, sizeof(u16) * 3);
    req->ssidLength = ssidLength;
    MI_CpuCopy16(ssid, req->ssid, sizeof(u8) * 32);
    req->scanType = scanType;
    MI_CpuCopy16(channelList, req->channelList, sizeof(u8) * 16);
    req->maxChannelTime = maxChannelTime;

#if SDK_VERSION_WL >= 20300
    req->bssidMaskCount = 0;
#endif

    cfm = (WlMlmeScanCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = (u16)(bufSize / 2 - (sizeof(WlMlmeScanReq) + 1) / 2 - (sizeof(WlCmdHeader) + 1) / 2);

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeJoinCfm *WMSP_WL_MlmeJoin(u16 *buf, u16 timeOut, WlBssDesc *bssDesc)
{
    WlMlmeJoinReq *req;
    WlMlmeJoinCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeJoinReq *)buf;

    req->header.code = WL_CMDCODE_MLME_JOIN;
#if SDK_VERSION_WL < 17100
    req->header.length = 33;
#else
    req->header.length = 34;
#endif

    req->timeOut = timeOut;
    req->rsv = 0;
    MI_CpuCopy16(bssDesc, &(req->bssDesc), sizeof(WlBssDesc));

    cfm = (WlMlmeJoinCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 5;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeAuthenticateCfm *WMSP_WL_MlmeAuthenticate(u16 *buf, u16 *peerMacAdrs, u16 algorithm, u16 timeOut)
{
    WlMlmeAuthenticateReq *req;
    WlMlmeAuthenticateCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeAuthenticateReq *)buf;

    req->header.code = WL_CMDCODE_MLME_AUTH;
    req->header.length = 5;

    MI_CpuCopy16(peerMacAdrs, req->peerMacAdrs, sizeof(u16) * 3);
    req->algorithm = algorithm;
    req->timeOut = timeOut;

    cfm = (WlMlmeAuthenticateCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 6;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeDeAuthenticateCfm *WMSP_WL_MlmeDeAuthenticate(u16 *buf, u16 *peerMacAdrs, u16 reasonCode)
{
    WlMlmeDeAuthenticateReq *req;
    WlMlmeDeAuthenticateCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeDeAuthenticateReq *)buf;

    req->header.code = WL_CMDCODE_MLME_DEAUTH;
    req->header.length = 4;

    MI_CpuCopy16(peerMacAdrs, req->peerMacAdrs, sizeof(u16) * 3);
    req->reasonCode = reasonCode;

    cfm = (WlMlmeDeAuthenticateCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 4;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeAssociateCfm *WMSP_WL_MlmeAssociate(u16 *buf, u16 *peerMacAdrs, u16 listenInterval, u16 timeOut)
{
    WlMlmeAssociateReq *req;
    WlMlmeAssociateCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeAssociateReq *)buf;

    req->header.code = WL_CMDCODE_MLME_ASS;
    req->header.length = 5;

    MI_CpuCopy16(peerMacAdrs, req->peerMacAdrs, sizeof(u16) * 3);
    req->listenInterval = listenInterval;
    req->timeOut = timeOut;

    cfm = (WlMlmeAssociateCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 3;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeReAssociateCfm *WMSP_WL_MlmeReAssociate(u16 *buf, u16 *newApMacAdrs, u16 listenInterval, u16 timeOut)
{
    WlMlmeReAssociateReq *req;
    WlMlmeReAssociateCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeReAssociateReq *)buf;

    req->header.code = WL_CMDCODE_MLME_REASS;
    req->header.length = 5;

    MI_CpuCopy16(newApMacAdrs, req->newApMacAdrs, sizeof(u16) * 3);
    req->listenInterval = listenInterval;
    req->timeOut = timeOut;

    cfm = (WlMlmeReAssociateCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 3;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeDisAssociateCfm *WMSP_WL_MlmeDisAssociate(u16 *buf, u16 *peerMacAdrs, u16 reasonCode)
{
    WlMlmeDisAssociateReq *req;
    WlMlmeDisAssociateCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeDisAssociateReq *)buf;

    req->header.code = WL_CMDCODE_MLME_DISASS;
    req->header.length = 4;

    MI_CpuCopy16(peerMacAdrs, req->peerMacAdrs, sizeof(u16) * 3);
    req->reasonCode = reasonCode;

    cfm = (WlMlmeDisAssociateCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeStartCfm *WMSP_WL_MlmeStart(u16 *buf, u16 ssidLength, u8 *ssid, u16 beaconPeriod, u16 dtimPeriod, u16 channel, u16 basicRateSet, u16 supportRateSet, u16 gameInfoLength, WMGameInfo *gameInfo)
{
    WlMlmeStartReq *req;
    WlMlmeStartCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeStartReq *)buf;

    req->header.code = WL_CMDCODE_MLME_START;
    req->header.length = (u16)(23 + WL_CalcHeaderLength(gameInfoLength));

    req->ssidLength = ssidLength;
    MI_CpuCopy16(ssid, req->ssid, sizeof(u8) * 32);
    req->beaconPeriod = beaconPeriod;
    req->dtimPeriod = dtimPeriod;
    req->channel = channel;
    req->basicRateSet = basicRateSet;
    req->supportRateSet = supportRateSet;
    req->gameInfoLength = gameInfoLength;
    MI_CpuCopy16(gameInfo, req->gameInfo, gameInfoLength);

    cfm = (WlMlmeStartCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMlmeMeasureChannelCfm *WMSP_WL_MlmeMeasureChannel(u16 *buf, u16 ccaMode, u16 edThreshold, u16 measureTime, u8 *channelList)
{
    WlMlmeMeasureChannelReq *req;
    WlMlmeMeasureChannelCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMlmeMeasureChannelReq *)buf;

    req->header.code = WL_CMDCODE_MLME_MEASCH;
    req->header.length = 12;

    req->rsv = 0;
    req->ccaMode = ccaMode;
    req->edThreshold = edThreshold;
    req->measureTime = measureTime;
    MI_CpuCopy16(channelList, req->channelList, sizeof(u8) * 16);

    cfm = (WlMlmeMeasureChannelCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 18;

    (void)WMSP_WlRequest(buf);

    return cfm;
}


WlMaDataCfm *WMSP_WL_MaData(u16 *buf, WlTxFrame *frame)
{
    WlMaDataReq *req;
    WlMaDataCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMaDataReq *)buf;

    req->header.code = WL_CMDCODE_MA_DATA;
    req->header.length = 24;

    MI_CpuCopy16(frame, &(req->frame), sizeof(WlTxFrame));
    {
        WMDcfRecvBuf *pFrame = (WMDcfRecvBuf *)frame;
        pFrame->rsv1[0] = 0;
        pFrame->rsv1[1] = 0;
        pFrame->rsv2[0] = 0;
        pFrame->rsv2[1] = 0;
        pFrame->rsv2[2] = 0;
        pFrame->rsv3[0] = 0;
        pFrame->rsv3[1] = 0;
        pFrame->rsv3[2] = 0;
        pFrame->rsv3[3] = 0;
        pFrame->rsv4[0] = 0;
        pFrame->rsv4[1] = 0;
        pFrame->rsv4[2] = 0;
        pFrame->rsv4[3] = 0;
    }

    cfm = (WlMaDataCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 2;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMaKeyDataCfm *WMSP_WL_MaKeyData(u16 *buf, u16 length, u16 wmHeader, u16 *keyDatap)
{
    WlMaKeyDataReq *req;
    WlMaKeyDataCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMaKeyDataReq *)buf;

    req->header.code = WL_CMDCODE_MA_KEY;
    req->header.length = 4;

    req->length = length;
    req->wmHeader = wmHeader;
    req->keyDatap = keyDatap;

    cfm = (WlMaKeyDataCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlMaMpCfm *WMSP_WL_MaMp(u16 *buf, u16 resume, u16 retryLimit, u16 txop, u16 pollBitmap, u16 tmptt, u16 currTsf, u16 dataLength, u16 wmHeader, u16 *datap)
{
    WlMaMpReq *req;
    WlMaMpCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMaMpReq *)buf;

    req->header.code = WL_CMDCODE_MA_MP;
    req->header.length = 10;

    req->resume = resume;
    req->retryLimit = retryLimit;
    req->txop = txop;
    req->pollBitmap = pollBitmap;
    req->tmptt = tmptt;
    req->currTsf = currTsf;
    req->dataLength = dataLength;
    req->wmHeader = wmHeader;
    req->datap = datap;

    cfm = (WlMaMpCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

#if SDK_VERSION_WL >= 19600
WlMaClearDataCfm *WMSP_WL_MaClearData(u16 *buf, u16 flag)
{
    WlMaClearDataReq *req;
    WlMaClearDataCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlMaClrDataReq *)buf;

    req->header.code = WL_CMDCODE_MA_CLRDATA;
    req->header.length = 1;

    req->flag = flag;

    cfm = (WlMaClearDataCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}
#endif


WlParamSetCfm *WMSP_WL_ParamSetAll(WlParamSetAllReq *req)
{
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(((u16 *)req));

    req->header.code = WL_CMDCODE_PARAM_SET_ALL;
    req->header.length = 72;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest((u16 *)req);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetMacAddress(u16 *buf, u16 *staMacAdrs)
{
    WlParamSetMacAddressReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetMacAddressReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_MAC_ADRS;
    req->header.length = 3;

    MI_CpuCopy16(staMacAdrs, req->staMacAdrs, sizeof(u16) * 3);

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetRetryLimit(u16 *buf, u16 retryLimit)
{
    WlParamSetRetryLimitReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetRetryLimitReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_RETRY_LIMIT;
    req->header.length = 1;

    req->retryLimit = retryLimit;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

#if SDK_VERSION_WL >= 15600
WlParamSetCfm *WMSP_WL_ParamSetEnableChannel(u16 *buf, u16 enableChannel)
{
    WlParamSetEnableChannelReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetEnableChannelReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_ENABLECHANNEL;
    req->header.length = 1;

    req->enableChannel = enableChannel;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}
#endif

WlParamSetCfm *WMSP_WL_ParamSetMode(u16 *buf, u16 mode)
{
    WlParamSetModeReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetModeReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_MODE;
    req->header.length = 1;

    req->mode = mode;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetRate(u16 *buf, u16 rate)
{
    WlParamSetRateReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetRateReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_RATE;
    req->header.length = 1;

    req->rate = rate;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetWepMode(u16 *buf, u16 wepMode)
{
    WlParamSetWepModeReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetWepModeReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_WEP_MODE;
    req->header.length = 1;

    req->wepMode = wepMode;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetWepKeyId(u16 *buf, u16 wepKeyId)
{
    WlParamSetWepKeyIdReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetWepKeyIdReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_WEP_KEYID;
    req->header.length = 1;

    req->wepKeyId = wepKeyId;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetWepKey(u16 *buf, u8 *wepKey)
{
    WlParamSetWepKeyReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetWepKeyReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_WEP_KEY;
    req->header.length = 40;

    MI_CpuCopy16(wepKey, req->wepKey, sizeof(u8) * 20 * 4);

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetBeaconType(u16 *buf, u16 beaconType)
{
    WlParamSetBeaconTypeReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetBeaconTypeReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_BEACON_TYPE;
    req->header.length = 1;

    req->beaconType = beaconType;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetProbeResponse(u16 *buf, u16 probeRes)
{
    WlParamSetProbeResponseReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetProbeResponseReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_PROBE_RES;
    req->header.length = 1;

    req->probeRes = probeRes;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetBeaconLostThreshold(u16 *buf, u16 beaconLostTh)
{
    WlParamSetBeaconLostThresholdReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetBeaconLostThresholdReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_BEACON_LOST;
    req->header.length = 1;

    req->beaconLostTh = beaconLostTh;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetActiveZone(u16 *buf, u16 activeZoneTime)
{
    WlParamSetActiveZoneReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetActiveZoneReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_ACTIVE_ZONE;
    req->header.length = 1;

    req->activeZoneTime = activeZoneTime;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetSsidMask(u16 *buf, u8 *mask)
{
    WlParamSetSsidMaskReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetSsidMaskReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_SSID_MASK;
    req->header.length = 16;

    MI_CpuCopy16(mask, req->mask, sizeof(u8) * 32);

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetPreambleType(u16 *buf, u16 type)
{
    WlParamSetPreambleTypeReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetPreambleTypeReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_PREAMBLE_TYPE;
    req->header.length = 1;

    req->type = type;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetAuthenticationAlgorithm(u16 *buf, u16 type)
{
    WlParamSetAuthenticationAlgorithmReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetAuthenticationAlgorithmReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_AUTHALGO;
    req->header.length = 1;

    req->type = type;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

#if (SDK_VERSION_WL >= 24900)
WlParamSetCfm *WMSP_WL_ParamSetCCAModeEDThreshold(u16 *buf, u16 ccaMode, u16 edThreshold, u16 agcLimit)
{
    WlParamSetCCAModeEDThresholdReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetCCAModeEDThReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_CCAMODE;
    req->header.length = 3;

    req->ccaMode = ccaMode;
    req->edThreshold = edThreshold;

    req->agcLimit = agcLimit;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}
#else
WlParamSetCfm *WMSP_WL_ParamSetCCAModeEDThreshold(u16 *buf, u16 ccaMode, u16 edThreshold)
{
    WlParamSetCCAModeEDThresholdReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetCCAModeEDThReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_CCAMODE;
    req->header.length = 3;

    req->ccaMode = ccaMode;
    req->edThreshold = edThreshold;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}
#endif

WlParamSetCfm *WMSP_WL_ParamSetLifeTime(u16 *buf, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime)
{
    WlParamSetLifeTimeReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetLifeTimeReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_LIFETIME;
    req->header.length = 3;

    req->tableNumber = tableNumber;
    req->camLifeTime = camLifeTime;
    req->frameLifeTime = frameLifeTime;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetMaxConnectableChild(u16 *buf, u16 count)
{
    WlParamSetMaxConnectableChildReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetMaxConnectableChildReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_MAXCONN;
    req->header.length = 1;

    req->count = count;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetMainAntenna(u16 *buf, u16 mainAntenna)
{
    WlParamSetMainAntennaReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetMainAntennaReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_MAIN_ANTENNA;
    req->header.length = 1;

    req->mainAntenna = mainAntenna;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetDiversity(u16 *buf, u16 diversity, u16 useAntenna)
{
    WlParamSetDiversityReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetDiversityReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_DIVERSITY;
    req->header.length = 2;

    req->diversity = diversity;
    req->useAntenna = useAntenna;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetBeaconSendRecvInd(u16 *buf, u16 enableMessage)
{
    WlParamSetBeaconSendRecvIndReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetBeaconSendRecvIndReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_BCNTXRX_IND;
    req->header.length = 1;

    req->enableMessage = enableMessage;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

#if SDK_VERSION_WL >= 25700
WlParamSetCfm *WMSP_WL_ParamSetNullKeyResponseMode(u16 *buf, u16 mode)
{
    WlParamSetNullKeyModeReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetNullKeyModeReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_NULLKEYMODE;
    req->header.length = 1;

    req->mode = mode;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}
#endif

WlParamSetCfm *WMSP_WL_ParamSetBssid(u16 *buf, u16 *bssid)
{
    WlParamSetBssidReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetBssidReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_BSSID;
    req->header.length = 3;

    MI_CpuCopy16(bssid, req->bssid, sizeof(u16) * 3);

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetSsid(u16 *buf, u16 ssidLength, u8 *ssid)
{
    WlParamSetSsidReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetSsidReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_SSID;
    req->header.length = 17;

    req->ssidLength = ssidLength;
    MI_CpuCopy16(ssid, req->ssid, sizeof(u8) * 32);

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetBeaconPeriod(u16 *buf, u16 beaconPeriod)
{
    WlParamSetBeaconPeriodReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetBeaconPeriodReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_BEACON_PERIOD;
    req->header.length = 1;

    req->beaconPeriod = beaconPeriod;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetDtimPeriod(u16 *buf, u16 dtimPeriod)
{
    WlParamSetDtimPeriodReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetDtimPeriodReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_DTIM_PERIOD;
    req->header.length = 1;

    req->dtimPeriod = dtimPeriod;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetInterval(u16 *buf, u16 listenInterval)
{
    WlParamSetIntervalReq *req;
    WlParamSetCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlParamSetIntervalReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_LISTEN_INT;
    req->header.length = 1;

    req->listenInterval = listenInterval;

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

WlParamSetCfm *WMSP_WL_ParamSetGameInfo(u16 *buf, u16 gameInfoLength, u16 *gameInfo)
{
    WlParamSetGameInfoReq *req;
    WlParamSetCfm *cfm;
    WMSPWork *sys = WMSP_GetSystemWork();

    WMSPi_FillReserved6(buf);

    req = (WlParamSetGameInfoReq *)buf;

    req->header.code = WL_CMDCODE_PARAM_SET_GAME_INFO;
    req->header.length = (u16)(1 + WL_CalcHeaderLength(gameInfoLength));
    req->gameInfoLength = gameInfoLength;
    MI_CpuCopy16(gameInfo, req->gameInfo, gameInfoLength);

    cfm = (WlParamSetCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}


WlParamGetAllCfm *WMSP_WL_ParamGetAll(u16 *buf)
{
    return (WlParamGetAllCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_ALL, 33);
}

WlParamGetMacAddressCfm *WMSP_WL_ParamGetMacAddress(u16 *buf)
{
    return (WlParamGetMacAddressCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_MAC_ADRS, 4);
}

WlParamGetRetryLimitCfm *WMSP_WL_ParamGetRetryLimit(u16 *buf)
{
    return (WlParamGetRetryLimitCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_RETRY_LIMIT, 2);
}

#if SDK_VERSION_WL >= 15600
WlParamGetEnableChannelCfm *WMSP_WL_ParamGetEnableChannel(u16 *buf)
{
    return (WlParamGetEnableChannelCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_ENABLECHANNEL, 3);
}
#endif

WlParamGetModeCfm *WMSP_WL_ParamGetMode(u16 *buf)
{
    return (WlParamGetModeCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_MODE, 2);
}

WlParamGetRateCfm *WMSP_WL_ParamGetRate(u16 *buf)
{
    return (WlParamGetRateCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_RATE, 2);
}

WlParamGetWepModeCfm *WMSP_WL_ParamGetWepMode(u16 *buf)
{
    return (WlParamGetWepModeCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_WEP_MODE, 2);
}

WlParamGetWepKeyIdCfm *WMSP_WL_ParamGetWepKeyId(u16 *buf)
{
    return (WlParamGetWepKeyIdCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_WEP_KEYID, 2);
}

WlParamGetBeaconTypeCfm *WMSP_WL_ParamGetBeaconType(u16 *buf)
{
    return (WlParamGetBeaconTypeCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_BEACON_TYPE, 2);
}

WlParamGetProbeResponseCfm *WMSP_WL_ParamGetProbeResponse(u16 *buf)
{
    return (WlParamGetProbeResponseCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_PROBE_RES, 2);
}

WlParamGetBeaconLostThresholdCfm *WMSP_WL_ParamGetBeaconLostThreshold(u16 *buf)
{
    return (WlParamGetBeaconLostThresholdCfm *)WMSPi_WL_NoArg(buf,
        WL_CMDCODE_PARAM_GET_BEACON_LOST,
        2);
}

WlParamGetActiveZoneCfm *WMSP_WL_ParamGetActiveZone(u16 *buf)
{
    return (WlParamGetActiveZoneCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_ACTIVE_ZONE, 2);
}

WlParamGetSsidMaskCfm *WMSP_WL_ParamGetSsidMask(u16 *buf)
{
    return (WlParamGetSsidMaskCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_SSID_MASK, 17);
}

WlParamGetPreambleTypeCfm *WMSP_WL_ParamGetPreambleType(u16 *buf)
{
    return (WlParamGetPreambleTypeCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_PREAMBLE_TYPE, 2);
}

WlParamGetAuthenticationAlgorithmCfm *WMSP_WL_ParamGetAuthenticationAlgorithm(u16 *buf)
{
    return (WlParamGetAuthenticationAlgorithmCfm *)WMSPi_WL_NoArg(buf,
        WL_CMDCODE_PARAM_GET_AUTHALGO,
        2);
}

WlParamGetCCAModeEDThCfm *WMSP_WL_ParamGetCCAModeEDThreshold(u16 *buf)
{
    return (WlParamGetCCAModeEDThCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_CCAMODE, 4);
}

WlParamGetMaxConnectableChildCfm *WMSP_WL_ParamGetMaxConnectableChild(u16 *buf)
{
    return (WlParamGetMaxConnectableChildCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_MAXCONN, 2);
}

WlParamGetMainAntennaCfm *WMSP_WL_ParamGetMainAntenna(u16 *buf)
{
    return (WlParamGetMainAntennaCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_MAIN_ANTENNA, 2);
}

WlParamGetDiversityCfm *WMSP_WL_ParamGetDiversity(u16 *buf)
{
    return (WlParamGetDiversityCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_DIVERSITY, 3);
}

WlParamGetBeaconSendRecvIndCfm *WMSP_WL_ParamGetBeaconSendRecvInd(u16 *buf)
{
    return (WlParamGetBeaconSendRecvIndCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_BCNTXRX_IND, 2);
}

#if SDK_VERSION_WL >= 25700
WlParamGetNullKeyModeCfm *WMSP_WL_ParamGetNullKeyModeCfm(u16 *buf)
{
    return (WlParamGetNullKeyModeCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_NULLKEYMODE, 2);
}
#endif

WlParamGetBssidCfm *WMSP_WL_ParamGetBssid(u16 *buf)
{
    return (WlParamGetBssidCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_BSSID, 4);
}

WlParamGetSsidCfm *WMSP_WL_ParamGetSsid(u16 *buf)
{
    return (WlParamGetSsidCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_SSID, 18);
}

WlParamGetBeaconPeriodCfm *WMSP_WL_ParamGetBeaconPeriod(u16 *buf)
{
    return (WlParamGetBeaconPeriodCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_BEACON_PERIOD, 2);
}

WlParamGetDtimPeriodCfm *WMSP_WL_ParamGetDtimPeriod(u16 *buf)
{
    return (WlParamGetDtimPeriodCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_DTIM_PERIOD, 2);
}

WlParamGetIntervalCfm *WMSP_WL_ParamGetInterval(u16 *buf)
{
    return (WlParamGetIntervalCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_LISTEN_INT, 2);
}

WlParamGetGameInfoCfm *WMSP_WL_ParamGetGameInfo(u16 *buf)
{
    return (WlParamGetGameInfoCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_PARAM_GET_GAME_INFO, 2 + WL_CalcHeaderLength(128));
}

WlDevShutdownCfm *WMSP_WL_DevShutdown(u16 *buf)
{
    return (WlDevShutdownCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_SHUTDOWN, 1);
}

WlDevIdleCfm *WMSP_WL_DevIdle(u16 *buf)
{
    return (WlDevIdleCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_IDLE, 1);
}

WlDevClass1Cfm *WMSP_WL_DevClass1(u16 *buf)
{
    return (WlDevClass1Cfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_CLASS1, 1);
}

#if SDK_VERSION_WL < 15600
WlDevIfcCfm *WMSP_WL_DevIfc(u16 *buf)
{
    WlDevIfcReq *req;
    WlDevIfcCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlDevIfcReq *)buf;

    req->header.code = WL_CMDCODE_DEV_IFC;
    req->header.length = 8;

    cfm = (WlDevIfcCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}
#else
WlDevRestartCfm *WMSP_WL_DevRestart(u16 *buf)
{
    return (WlDevRestartCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_REBOOT, 1);
}
#endif

WlDevSetInitializeWirelessCounterCfm *WMSP_WL_DevSetInitializeWirelessCounter(u16 *buf)
{
    return (WlDevSetInitializeWirelessCounterCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_INIT_INFO, 1);
}

WlDevGetVersionCfm *WMSP_WL_DevGetVersion(u16 *buf)
{
    return (WlDevGetVersionCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_GET_VERINFO, 9);
}

WlDevGetWirelessCounterCfm *WMSP_WL_DevGetWirelessCounter(u16 *buf)
{
#if SDK_VERSION_WL >= 16700
    return (WlDevGetWirelessCounterCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_GET_INFO, 92);
#else
    return (WlDevGetWirelessCounterCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_GET_INFO, 96);
#endif
}

WlDevGetStationStateCfm *WMSP_WL_DevGetStationState(u16 *buf)
{
    return (WlDevGetStationStateCfm *)WMSPi_WL_NoArg(buf, WL_CMDCODE_DEV_GET_STATE, 2);
}

WlDevTestSignalCfm *WMSP_WL_DevTestSignal(u16 *buf, u16 control, u16 signal, u16 rate, u16 channel)
{
    WlDevTestSignalReq *req;
    WlDevTestSignalCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlDevTestSignalReq *)buf;

    req->header.code = WL_CMDCODE_DEV_TEST_SIGNAL;
    req->header.length = 4;

    req->control = control;
    req->signal = signal;
    req->rate = rate;
    req->channel = channel;

    cfm = (WlDevTestSignalCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

#if SDK_VERSION_WL >= 26600
WlDevTestRxCfm *WMSP_WL_DevTestRx(u16 *buf, u16 control, u16 channel)
{
    WlDevTestRxReq *req;
    WlDevTestRxCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlDevTestRxReq *)buf;

    req->header.code = WL_CMDCODE_DEV_TEST_RX;
    req->header.length = 2;

    req->control = control;
    req->channel = channel;

    cfm = (WlDevTestRxCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = 1;

    (void)WMSP_WlRequest(buf);

    return cfm;
}
#endif


static WlCmdCfm *WMSPi_WL_NoArg(u16 *buf, u16 code, u16 cfmLength)
{
    WlCmdReq *req;
    WlCmdCfm *cfm;

    WMSPi_FillReserved6(buf);

    req = (WlCmdReq *)buf;

    req->header.code = code;
    req->header.length = 0;

    cfm = (WlCmdCfm *)WL_CalcConfirmPointer(req);
    cfm->header.code = req->header.code;
    cfm->header.length = cfmLength;

    (void)WMSP_WlRequest(buf);

    return cfm;
}

