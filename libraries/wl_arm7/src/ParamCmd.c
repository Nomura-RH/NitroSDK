#define __PARAMCMD_C_
#define __INSYSROM__

#include "WlSys.h"

#include "WlLib.h"
#include "WlNic.h"
#include "WlCmdIf.h"
#include "Param.h"
#include "ParamCmd.h"

#include "MAC.h"

u16 PARAMSET_AllReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    u16 ret;
    WlParamSetAllReq *pReq = (WlParamSetAllReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA != STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    ret = WSetMacAdrs(pReq->staMacAdrs);
    ret |= WSetRetryLimit(pReq->retryLimit);
    ret |= WSetEnableChannel(pReq->enableChannel);
    ret |= WSetMode(pReq->mode);
    ret |= WSetRate(pReq->rate);
    ret |= WSetWepMode(pReq->wepMode);
    ret |= WSetWepKeyId(pReq->wepKeyId);
    ret |= WSetWepKey((u16 *)pReq->wepKey);
    ret |= WSetBeaconType(pReq->beaconType);
    ret |= WSetBcSsidResponse(pReq->probeRes);
    ret |= WSetBeaconLostThreshold(pReq->beaconLostTh);
    ret |= WSetActiveZoneTime(pReq->activeZoneTime, FALSE);
    ret |= WSetSsidMask((u16 *)pReq->ssidMask);
    ret |= WSetPreambleType(pReq->preambleType);
    ret |= WSetAuthAlgo(pReq->authAlgo);

    return ret;
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_MacAdrsReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetMacAdrsReq *pReq = (WlParamSetMacAdrsReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA != STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetMacAdrs(pReq->staMacAdrs);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_RetryReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetRetryLimitReq *pReq = (WlParamSetRetryLimitReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetRetryLimit(pReq->retryLimit);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_EnableChannelReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetEnableChannelReq *pReq = (WlParamSetEnableChannelReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA != STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetEnableChannel(pReq->enableChannel);
}
#endif

u16 PARAMSET_ModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetModeReq *pReq = (WlParamSetModeReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA > STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }
    if ((wlMan->Work.STA == STA_CLASS1) && (wlMan->Work.bSynchro)) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetMode(pReq->mode);
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_RateReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetRateReq *pReq = (WlParamSetRateReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetRate(pReq->rate);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_WepModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetWepModeReq *pReq = (WlParamSetWepModeReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetWepMode(pReq->wepMode);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_WepKeyIdReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetWepKeyIdReq *pReq = (WlParamSetWepKeyIdReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetWepKeyId(pReq->wepKeyId);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_WepKeyReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetWepKeyReq *pReq = (WlParamSetWepKeyReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetWepKey((u16 *)pReq->wepKey);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_BeaconTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetBeaconTypeReq *pReq = (WlParamSetBeaconTypeReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA > STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetBeaconType(pReq->beaconType);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_ResBcSsidReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetProbeResReq *pReq = (WlParamSetProbeResReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetBcSsidResponse(pReq->probeRes);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_BeaconLostThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetBeaconLostThReq *pReq = (WlParamSetBeaconLostThReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetBeaconLostThreshold(pReq->beaconLostTh);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_ActiveZoneReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetActiveZoneReq *pReq = (WlParamSetActiveZoneReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetActiveZoneTime(pReq->activeZoneTime, FALSE);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_SSIDMaskReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetSsidMaskReq *pReq = (WlParamSetSsidMaskReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetSsidMask((u16 *)pReq->mask);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_PreambleTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetPreambleTypeReq *pReq = (WlParamSetPreambleTypeReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetPreambleType(pReq->type);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_AuthAlgoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetAuthAlgoReq *pReq = (WlParamSetAuthAlgoReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetAuthAlgo(pReq->type);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_CCAModeEDThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetCCAModeEDThReq *pReq = (WlParamSetCCAModeEDThReq *)pReqt;
    u16 ret;

    pCfmt->header.length = 1;

    if (pReq->agcLimit > MAX_AGC_LIMIT) {
        return WL_CMDRES_INVALID_PARAM;
    }

    ret = WSetCCA_ED(pReq->ccaMode, pReq->edThreshold);

    if (ret == WL_CMDRES_SUCCESS) {
        BBP_Write(0x2E, pReq->agcLimit);
    }

    return ret;
}
#endif

u16 PARAMSET_LifeTimeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetLifeTimeReq *pReq = (WlParamSetLifeTimeReq *)pReqt;
    LPCAM_ELEMENT pCAM = wlMan->Config.pCAM;
    u32 i;

    pCfmt->header.length = 1;

    if ((pReq->tableNumber >= wlMan->Config.MaxStaNum) && (pReq->tableNumber != 0xFFFF)) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if ((pReq->frameLifeTime > MAX_FRM_LIFETIME) && (pReq->frameLifeTime != 0xFFFF)) {
        return WL_CMDRES_INVALID_PARAM;
    }

    if (pReq->tableNumber == 0xFFFF) {
        for (i = 1; i < wlMan->Config.MaxStaNum; i++) {
            pCAM[i].maxLifeTime = pReq->camLifeTime;

            if (pCAM[i].lifeTime != 0) {
                pCAM[i].lifeTime = pReq->camLifeTime;
            }
        }
    } else if (pReq->tableNumber != 0) {
        pCAM[pReq->tableNumber].maxLifeTime = pReq->camLifeTime;

        if (pCAM[pReq->tableNumber].lifeTime != 0) {
            pCAM[pReq->tableNumber].lifeTime = pReq->camLifeTime;
        }
    }

    if (pReq->frameLifeTime != 0) {
        WSetFrameLifeTime(pReq->frameLifeTime);
    }

    return WL_CMDRES_SUCCESS;
}

u16 PARAMSET_MaxConnReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetMaxConnReq *pReq = (WlParamSetMaxConnReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA > STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }

    pReq->count++;
    if (pReq->count > wlMan->Config.CamMaxStaNum) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.MaxStaNum = pReq->count;

    return WL_CMDRES_SUCCESS;
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_MainAntennaReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetMainAntennaReq *pReq = (WlParamSetMainAntennaReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetMainAntenna(pReq->mainAntenna);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_DiversityReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetDiversityReq *pReq = (WlParamSetDiversityReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetDiversity(pReq->diversity, pReq->useAntenna);
}
#endif

u16 PARAMSET_BcnSendRecvIndReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetBeaconSendRecvIndReq *pReq = (WlParamSetBeaconSendRecvIndReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetBeaconSendRecvIndicate(pReq->enableMessage);
}

u16 PARAMSET_NullKeyModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetNullKeyModeReq *pReq = (WlParamSetNullKeyModeReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    return WSetNullKeyMode(pReq->mode);
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_BSSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetBssidReq *pReq = (WlParamSetBssidReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetBssid(pReq->bssid);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_SSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetSsidReq *pReq = (WlParamSetSsidReq *)pReqt;

    pCfmt->header.length = 1;

    return WSetSsid(pReq->ssidLength, pReq->ssid);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_BeaconPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetBeaconPeriodReq *pReq = (WlParamSetBeaconPeriodReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Config.Mode != WL_CMDLABEL_MODE_PARENT) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    return WSetBeaconPeriod(pReq->beaconPeriod);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_DTIMPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetDtimPeriodReq *pReq = (WlParamSetDtimPeriodReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Config.Mode != WL_CMDLABEL_MODE_PARENT) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    return WSetDTIMPeriod(pReq->dtimPeriod);
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMSET_ListenIntervalReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetIntervalReq *pReq = (WlParamSetIntervalReq *)pReqt;

    pCfmt->header.length = 1;

    if ((wlMan->Config.Mode != WL_CMDLABEL_MODE_CHILD) && (wlMan->Config.Mode != WL_CMDLABEL_MODE_HOTSPOT)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    return WSetListenInterval(pReq->listenInterval);
}
#endif

u16 PARAMSET_GameInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlParamSetGameInfoReq *pReq = (WlParamSetGameInfoReq *)pReqt;

    pCfmt->header.length = 1;

    if (wlMan->Config.Mode != WL_CMDLABEL_MODE_PARENT) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pReq->header.length < (1 + (pReq->gameInfoLength + 1) / 2)) {
        return WL_CMDRES_LENGTH_ERR;
    }

    return WSetGameInfo(pReq->gameInfoLength, (u8 *)pReq->gameInfo);
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_AllReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetAllCfm *pCfm = (WlParamGetAllCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetAllCfm);

    WSetMacAdrs1(pCfm->staMacAdrs, wlMan->Config.MacAdrs);
    pCfm->retryLimit = wlMan->Config.RetryLimit;
    pCfm->enableChannel = wlMan->Config.EnableChannel;
    pCfm->channel = wlMan->Work.CurrChannel;
    pCfm->mode = wlMan->Config.Mode;
    pCfm->rate = wlMan->Config.Rate;
    pCfm->wepMode = wlMan->Config.WepMode;
    pCfm->wepKeyId = wlMan->Config.WepKeyId;
    pCfm->beaconType = wlMan->Config.BeaconType;
    pCfm->probeRes = wlMan->Config.BcSsidResponse;
    pCfm->beaconLostTh = wlMan->Work.BeaconLostTh;
    pCfm->activeZoneTime = wlMan->Config.ActiveZone;
    WLLIB_DmaCopy16((u32)wlMan->Work.SSIDMask, (u32)pCfm->ssidMask, 32);
    pCfm->preambleType = wlMan->Config.PreambleType;
    pCfm->authAlgo = wlMan->Config.AuthAlgo;

    return WL_CMDRES_SUCCESS;
}
#endif

u16 PARAMGET_MacAdrsReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetMacAdrsCfm *pCfm = (WlParamGetMacAdrsCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetMacAdrsCfm);

    WSetMacAdrs1(pCfm->staMacAdrs, wlMan->Config.MacAdrs);

    return WL_CMDRES_SUCCESS;
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_RetryReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetRetryLimitCfm *pCfm = (WlParamGetRetryLimitCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetRetryLimitCfm);

    pCfm->retryLimit = wlMan->Config.RetryLimit;

    return WL_CMDRES_SUCCESS;
}
#endif

u16 PARAMGET_EnableChannelReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetEnableChannelCfm *pCfm = (WlParamGetEnableChannelCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetEnableChannelCfm);

    pCfm->enableChannel = wlMan->Config.EnableChannel;
    pCfm->channel = wlMan->Work.CurrChannel;

    return WL_CMDRES_SUCCESS;
}

u16 PARAMGET_ModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetModeCfm *pCfm = (WlParamGetModeCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetModeCfm);

    pCfm->mode = wlMan->Config.Mode;

    return WL_CMDRES_SUCCESS;
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_RateReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetRateCfm *pCfm = (WlParamGetRateCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetRateCfm);

    pCfm->rate = wlMan->Config.Rate;

    return WL_CMDRES_SUCCESS;
}
#endif

u16 PARAMGET_WepModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetWepModeCfm *pCfm = (WlParamGetWepModeCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetWepModeCfm);

    pCfm->wepMode = wlMan->Config.WepMode;

    return WL_CMDRES_SUCCESS;
}

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_WepKeyIdReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetWepKeyIdCfm *pCfm = (WlParamGetWepKeyIdCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetWepKeyIdCfm);

    pCfm->wepKeyId = wlMan->Config.WepKeyId;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_BeaconTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetBeaconTypeCfm *pCfm = (WlParamGetBeaconTypeCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetBeaconTypeCfm);

    pCfm->beaconType = wlMan->Config.BeaconType;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_ResBcSsidReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetProbeResCfm *pCfm = (WlParamGetProbeResCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetProbeResCfm);

    pCfm->probe = wlMan->Config.BcSsidResponse;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_BeaconLostThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetBeaconLostThCfm *pCfm = (WlParamGetBeaconLostThCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetBeaconLostThCfm);

    pCfm->beaconLostTh = wlMan->Work.BeaconLostTh;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_ActiveZoneReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetActiveZoneCfm *pCfm = (WlParamGetActiveZoneCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetActiveZoneCfm);

    pCfm->activeZoneTime = wlMan->Config.ActiveZone;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_SSIDMaskReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    u32 i;
    WlParamGetSsidMaskCfm *pCfm = (WlParamGetSsidMaskCfm *)pCfmt;
    u16 *p1, *p2;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetSsidMaskCfm);

    p1 = (u16 *)&pCfm->mask[0];
    p2 = (u16 *)&wlMan->Work.SSIDMask[0];
    for (i = 0; i < MAX_SSID_LENGTH / 2; i++) {
        *p1++ = *p2++;
    }

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_PreambleTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetPreambleTypeCfm *pCfm = (WlParamGetPreambleTypeCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetPreambleTypeCfm);

    pCfm->type = wlMan->Config.PreambleType;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_AuthAlgoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetAuthAlgoCfm *pCfm = (WlParamGetAuthAlgoCfm *)pCfmt;

    pCfmt->header.length = CalcCfmMsgLength(WlParamGetAuthAlgoCfm);

    pCfm->type = wlMan->Config.AuthAlgo;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_CCAModeEDThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetCCAModeEDThCfm *pCfm = (WlParamGetCCAModeEDThCfm *)pCfmt;

    pCfmt->header.length = CalcCfmMsgLength(WlParamGetCCAModeEDThCfm);

    pCfm->ccaMode = BBP_Read(BBPREG_CCA_MODE);
    pCfm->edThreshold = BBP_Read(BBPREG_ED_TH);
    pCfm->agcLimit = BBP_Read(BBPREG_AGC_LIMIT);

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_MaxConnReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetMaxConnCfm *pCfm = (WlParamGetMaxConnCfm *)pCfmt;

    pCfmt->header.length = CalcCfmMsgLength(WlParamGetMaxConnCfm);

    pCfm->count = wlMan->Config.MaxStaNum - 1;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_MainAntennaReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetMainAntennaCfm *pCfm = (WlParamGetMainAntennaCfm *)pCfmt;

    pCfmt->header.length = CalcCfmMsgLength(WlParamGetMainAntennaCfm);

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    pCfm->mainAntenna = wlMan->Config.MainAntenna;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_DiversityReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetDiversityCfm *pCfm = (WlParamGetDiversityCfm *)pCfmt;

    pCfmt->header.length = CalcCfmMsgLength(WlParamGetDiversityCfm);

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    pCfm->diversity = wlMan->Config.Diversity;
    pCfm->useAntenna = wlMan->Config.UseAntenna ^ wlMan->Config.MainAntenna;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_BcnSendRecvIndReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetBeaconSendRecvIndCfm *pCfm = (WlParamGetBeaconSendRecvIndCfm *)pCfmt;

    pCfmt->header.length = 2;

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    pCfm->enableMessage = wlMan->Config.BcnTxRxIndMsg;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_NullKeyModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetNullKeyModeCfm *pCfm = (WlParamGetNullKeyModeCfm *)pCfmt;

    pCfmt->header.length = 2;

    if (wlMan->Work.STA < STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    pCfm->mode = wlMan->Config.NullKeyRes;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_BSSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetBssidCfm *pCfm = (WlParamGetBssidCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetBssidCfm);

    WSetMacAdrs1(pCfm->bssid, wlMan->Work.BSSID);

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_SSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    u32 i;

    WlParamGetSsidCfm *pCfm = (WlParamGetSsidCfm *)pCfmt;
    u16 *p1, *p2;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetSsidCfm);

    pCfm->ssidLength = wlMan->Work.SSIDLength;
    p1 = (u16 *)&pCfm->ssid[0];
    p2 = (u16 *)&wlMan->Work.SSID[0];
    for (i = 0; i < MAX_SSID_LENGTH; i += 2) {
        *p1++ = *p2++;
    }

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_BeaconPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetBeaconPeriodCfm *pCfm = (WlParamGetBeaconPeriodCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetBeaconPeriodCfm);

    pCfm->beaconPeriod = wlMan->Work.BeaconPeriod;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_DTIMPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetDtimPeriodCfm *pCfm = (WlParamGetDtimPeriodCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetDtimPeriodCfm);

    pCfm->dtimPeriond = wlMan->Work.DTIMPeriod;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_ListenIntervalReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetIntervalCfm *pCfm = (WlParamGetIntervalCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlParamGetIntervalCfm);

    pCfm->listenInterval = wlMan->Work.ListenInterval;

    return WL_CMDRES_SUCCESS;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 PARAMGET_GameInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
#pragma unused(pReqt)
    WlParamGetGameInfoCfm *pCfm = (WlParamGetGameInfoCfm *)pCfmt;
    u8 *p1, *p2;
    u32 i;

    if (pCfm->header.length > 1) {
        pCfm->gameInfoLength = wlMan->Work.GameInfoLength;
    }

    if (wlMan->Work.GameInfoLength > (pCfm->header.length - 2) * 2) {
        return WL_CMDRES_LENGTH_ERR;
    }

    if (pCfm->gameInfoLength != 0) {
        if (wlMan->Work.GameInfoAlign & 1) {
            p1 = (u8 *)((u32)wlMan->Work.GameInfoAdrs + 1);
            p2 = (u8 *)pCfm->gameInfo;
            for (i = 0; i < pCfm->gameInfoLength; i++) {
                WL_WriteByte(p2, WL_ReadByte(p1));
                p2++;
                p1++;
            }
        } else {
            WLLIB_DmaCopy16((u32)wlMan->Work.GameInfoAdrs, (u32)pCfm->gameInfo, pCfm->gameInfoLength + 1);
        }
    }

    pCfm->header.length = 2 + (pCfm->gameInfoLength + 1) / 2;

    return WL_CMDRES_SUCCESS;
}
#endif
