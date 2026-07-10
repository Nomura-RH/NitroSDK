#define __MLME_C_
#define __INSYSROM__

#include "WlSys.h"
#include "TaskMan.h"

#include "WlLib.h"
#include "Param.h"
#include "WlNic.h"
#include "WlCmdIf.h"
#include "MLME.h"
#include "MAC.h"
#include "Flash.h"

#include "TxCtrl.h"

u16 MLME_ResetReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMlmeResetReq *pReq = (WlMlmeResetReq *)pReqt;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeResetCfm);

    if (pReq->mib > MAX_DEFAULT_MIB) {
        return WL_CMDRES_INVALID_PARAM;
    }

    WStop();

    if (pReq->mib == WL_CMDLABEL_RST_MIB_CLR) {
        WInitCounter();
    }

    return WL_CMDRES_SUCCESS;
}

u16 MLME_PwrMgtReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMlmePowerMgtReq *pReq = (WlMlmePowerMgtReq *)pReqt;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmePowerMgtReq);

    if (pReq->pwrMgtMode > MAX_PWRMGT_MODE) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->wakeUp > MAX_PWRMGT_WAVEUP) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->recieveDtims > MAX_PWRMGT_RXDTIMS) {
        return WL_CMDRES_INVALID_PARAM;
    }

    WSetPowerMgtMode(pReq->pwrMgtMode);
    if (pReq->pwrMgtMode == WL_CMDLABEL_PMG_PS) {
        if (pReq->wakeUp == WL_CMDLABEL_WAKEUP_CONT_SLEEP) {
            WSetForcePowerState(FPWR_STS_SLEEP);
        } else {
            WSetForcePowerState(FPWR_STS_DISABLE);
        }

        wlMan->Work.RxDtims = pReq->recieveDtims;
    } else {
        WSetForcePowerState(FPWR_STS_ACTIVE);

        WSetPowerState(PWRSTS_ACT);
    }

    return WL_CMDRES_SUCCESS;
}

u16 MLME_ScanReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    u32 i, ch;
    WlMlmeScanReq *pReq = (WlMlmeScanReq *)pReqt;
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPCONFIG_PARAM pConfig = &wlMan->Config;

    pMLME->Work.Scan.MaxConfirmLength = pCfmt->header.length - 3;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeScanCfm) - sizeof(WlBssDesc) / 2;

    if ((pConfig->Mode != WL_CMDLABEL_MODE_PARENT) && (pConfig->Mode != WL_CMDLABEL_MODE_HOTSPOT) && (pConfig->Mode != WL_CMDLABEL_MODE_CHILD)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (wlMan->Work.STA < STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (pReq->ssidLength > MAX_SSID_LENGTH) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->scanType > MAX_SCAN_TYPE) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (WL_ReadByte(&pReq->channelList[0]) == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->maxChannelTime > MAX_CHANNEL_TIME) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->maxChannelTime < MIN_CHANNEL_TIME) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssidMaskCount > MAX_BSSIDMASK_NUM) {
        return WL_CMDRES_INVALID_PARAM;
    }
    for (i = 0; i < sizeof(pReq->channelList); i++) {
        ch = WL_ReadByte(&pReq->channelList[i]);

        if (ch == 0) {
            break;
        }

        if (CheckEnableChannel(ch) == 0) {
            return WL_CMDRES_INVALID_PARAM;
        }
    }

    WSetBssid(pReq->bssid);
    WSetSsid(pReq->ssidLength, pReq->ssid);

    pMLME->pReq.Scan = (WlMlmeScanReq *)pReqt;
    pMLME->pCfm.Scan = (WlMlmeScanCfm *)pCfmt;

    pMLME->State = MLME_STATE_SCAN_STARTUP;

    AddTask(TASK_NORMAL_PRIORITY, MLME_SCAN_TASK_ID);

    return WL_CMDRES_OPERATING_MLME;
}

u16 MLME_JoinReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPMLME_MAN pMLME = &wlMan->MLME;
    WlMlmeJoinReq *pReq = (WlMlmeJoinReq *)pReqt;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeJoinCfm);

    if ((pConfig->Mode != WL_CMDLABEL_MODE_HOTSPOT) && (pConfig->Mode != WL_CMDLABEL_MODE_CHILD)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pWork->STA < STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }
    WSetStaState(STA_CLASS1);

    if (pReq->bssDesc.bssid[0] & 0x0001) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.ssidLength == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.ssidLength > MAX_SSID_LENGTH) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.beaconPeriod < MIN_BEACON_PERIOD) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.beaconPeriod > MAX_BEACON_PERIOD) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.dtimPeriod > MAX_DTIM_PERIOD) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.channel & 0xFFF0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (CheckEnableChannel(pReq->bssDesc.channel) == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.rateSet.basic & ~MASK_BRS) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.rateSet.support & ~MASK_SRS) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->bssDesc.rateSet.basic == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if ((pReq->bssDesc.rateSet.support | pReq->bssDesc.rateSet.basic) == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->timeOut > MAX_JOIN_TIMEOUT) {
        return WL_CMDRES_INVALID_PARAM;
    }

#ifndef SDK_NOCHK_ERR_WL
    if (FLASH_VerifyCheckSum(NULL) != 0) {
        return WL_CMDRES_FLASH_ERR;
    }
#endif

    if (pReq->bssDesc.capaInfo & B_CAPA_SHORTPREAMBLE) {
        WSetPreambleType(WL_CMDLABEL_PREAMBLE_SHORT);
    } else {
        WSetPreambleType(WL_CMDLABEL_PREAMBLE_LONG);
    }
    WSetBssid(pReq->bssDesc.bssid);
    WSetSsid(pReq->bssDesc.ssidLength, pReq->bssDesc.ssid);
    WSetBeaconPeriod(pReq->bssDesc.beaconPeriod);
    WSetChannel(pReq->bssDesc.channel, FALSE);
    WSetRateSet((LPRATE_SET)&pReq->bssDesc.rateSet);

    pMLME->pReq.Join = (WlMlmeJoinReq *)pReqt;
    pMLME->pCfm.Join = (WlMlmeJoinCfm *)pCfmt;

    pMLME->State = MLME_STATE_JOIN_STARTUP;

    AddTask(TASK_NORMAL_PRIORITY, MLME_JOIN_TASK_ID);

    return WL_CMDRES_OPERATING_MLME;
}

u16 MLME_AuthReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPMLME_MAN pMLME = &wlMan->MLME;
    WlMlmeAuthReq *pReq = (WlMlmeAuthReq *)pReqt;
    WlMlmeAuthCfm *pCfm = (WlMlmeAuthCfm *)pCfmt;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeAuthCfm);

    if ((pConfig->Mode != WL_CMDLABEL_MODE_HOTSPOT) && (pConfig->Mode != WL_CMDLABEL_MODE_CHILD)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pWork->STA < STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (pReq->peerMacAdrs[0] & 0x0001) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->algorithm > MAX_AUTHTYPE) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->timeOut > MAX_AUTH_TIMEOUT) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->timeOut < MIN_AUTH_TIMEOUT) {
        return WL_CMDRES_INVALID_PARAM;
    }

    WSetStaState(STA_CLASS1);

    pMLME->pReq.Auth = pReq;
    pMLME->pCfm.Auth = pCfm;

    pMLME->State = MLME_STATE_AUTH_STARTUP;

    pCfm->algorithm = pMLME->pReq.Auth->algorithm;
    WSetMacAdrs1(pCfm->peerMacAdrs, pMLME->pReq.Auth->peerMacAdrs);

    MLME_AuthTask();

    return WL_CMDRES_OPERATING_MLME;
}

u16 MLME_DeAuthReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMlmeDeAuthReq *pReq = (WlMlmeDeAuthReq *)pReqt;
    WlMlmeDeAuthCfm *pCfm = (WlMlmeDeAuthCfm *)pCfmt;
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPTXFRM pFrm;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeDeAuthCfm);

    if ((wlMan->Config.Mode != WL_CMDLABEL_MODE_HOTSPOT) && (wlMan->Config.Mode != WL_CMDLABEL_MODE_CHILD) && (wlMan->Config.Mode != WL_CMDLABEL_MODE_PARENT)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (wlMan->Work.STA < STA_CLASS2) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (((wlMan->Config.Mode == WL_CMDLABEL_MODE_HOTSPOT) || (wlMan->Config.Mode == WL_CMDLABEL_MODE_CHILD)) && (pReq->peerMacAdrs[0] & 0x0001)) {
        return WL_CMDRES_INVALID_PARAM;
    }

    WSetMacAdrs1(pCfm->peerMacAdrs, pReq->peerMacAdrs);

    pFrm = (LPTXFRM)MakeDeAuthFrame(pCfm->peerMacAdrs, pReq->reasonCode, FALSE);
    if ((u32)pFrm == HEAPBUF_NOT_ENOUGH_MEMORY) {
        return WL_CMDRES_NOT_ENOUGH_MEM;
    }

    pMLME->pReq.DeAuth = pReq;
    pMLME->pCfm.DeAuth = pCfm;

    pMLME->Work.DeAuth.pTxFrm = pFrm;
    pMLME->State = MLME_STATE_DEAUTH_ING;

    if (pReq->peerMacAdrs[0] & 0x0001) {
        pFrm->FirmHeader.FrameTime = wlMan->Work.IntervalCount;

        CAM_AddBcFrame(&wlMan->HeapMan.TmpBuf, CalcReqAdrsFromFrame(pFrm));

        if ((wlMan->CamMan.PowerMgtMode & ~wlMan->CamMan.NotClass3) == 0) {
            TxqPri(QID_BROADCAST);
        }
    } else {
        DeleteTxFrameByAdrs(pReq->peerMacAdrs);

        TxManCtrlFrame((LPTXFRM)pFrm);
    }

    return WL_CMDRES_OPERATING_MLME;
}

u16 MLME_AssReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMlmeAssReq *pReq = (WlMlmeAssReq *)pReqt;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPMLME_MAN pMLME = &wlMan->MLME;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeAssCfm);

    if ((pConfig->Mode != WL_CMDLABEL_MODE_HOTSPOT) && (pConfig->Mode != WL_CMDLABEL_MODE_CHILD)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pWork->STA < STA_CLASS2) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (pReq->peerMacAdrs[0] & 0x0001 != 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->listenInterval == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->listenInterval > MAX_LISTEN_INTERVAL) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->timeOut > MAX_ASS_TIMEOUT) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->timeOut < MIN_ASS_TIMEOUT) {
        return WL_CMDRES_INVALID_PARAM;
    }

    WSetStaState(STA_CLASS2);
    WClearAids();

    pWork->ListenInterval = pReq->listenInterval;
    pWork->CurrListenInterval = pReq->listenInterval;

    pMLME->pReq.Ass = (WlMlmeAssReq *)pReqt;
    pMLME->pCfm.Ass = (WlMlmeAssCfm *)pCfmt;

    pMLME->State = MLME_STATE_ASS_STARTUP;

    MLME_AssTask();

    return WL_CMDRES_OPERATING_MLME;
}

#ifndef SDK_SMALL_BUILD_WL
u16 MLME_ReAssReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMlmeReAssReq *pReq = (WlMlmeReAssReq *)pReqt;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPMLME_MAN pMLME = &wlMan->MLME;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeReAssCfm);

    if ((pConfig->Mode != WL_CMDLABEL_MODE_HOTSPOT) && (pConfig->Mode != WL_CMDLABEL_MODE_CHILD)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pWork->STA < STA_CLASS2) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (pReq->newApMacAdrs[0] & 0x0001) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->listenInterval < MIN_LISTEN_INTERVAL) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->listenInterval > MAX_LISTEN_INTERVAL) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->timeOut > MAX_REASS_TIMEOUT) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->timeOut < MIN_REASS_TIMEOUT) {
        return WL_CMDRES_INVALID_PARAM;
    }

    pWork->ListenInterval = pReq->listenInterval;
    pWork->CurrListenInterval = pReq->listenInterval;

    pMLME->pReq.ReAss = (WlMlmeReAssReq *)pReqt;
    pMLME->pCfm.ReAss = (WlMlmeReAssCfm *)pCfmt;

    pMLME->State = MLME_STATE_REASS_STARTUP;

    MLME_ReAssTask();

    return WL_CMDRES_OPERATING_MLME;
}
#endif

#ifndef SDK_SMALL_BUILD_WL
u16 MLME_DisAssReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMlmeDisAssReq *pReq = (WlMlmeDisAssReq *)pReqt;
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPTXFRM pFrm;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeDisAssCfm);

    if (wlMan->Config.Mode == WL_CMDLABEL_MODE_TEST) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if ((wlMan->Config.Mode != WL_CMDLABEL_MODE_PARENT) && (pReq->peerMacAdrs[0] & 0x0001)) {
        return WL_CMDRES_INVALID_PARAM;
    }

    if (wlMan->Work.STA != STA_CLASS3) {
        return WL_CMDRES_STATE_WRONG;
    }

    pFrm = (LPTXFRM)MakeDisAssFrame(pReq->peerMacAdrs, pReq->reasonCode);
    if ((u32)pFrm == HEAPBUF_NOT_ENOUGH_MEMORY) {
        return WL_CMDRES_NOT_ENOUGH_MEM;
    }

    pMLME->pReq.DisAss = (WlMlmeDisAssReq *)pReqt;
    pMLME->pCfm.DisAss = (WlMlmeDisAssCfm *)pCfmt;

    pMLME->Work.DisAss.pTxFrm = pFrm;
    pMLME->State = MLME_STATE_DISASS_ING;

    if (pReq->peerMacAdrs[0] & 0x0001) {
        pFrm->FirmHeader.FrameTime = wlMan->Work.IntervalCount;

        CAM_AddBcFrame(&wlMan->HeapMan.TmpBuf, CalcReqAdrsFromFrame(pFrm));

        if ((wlMan->CamMan.PowerMgtMode & ~wlMan->CamMan.NotClass3) == 0) {
            TxqPri(QID_BROADCAST);
        }
    } else {
        DeleteTxFrameByAdrs(pReq->peerMacAdrs);

        TxManCtrlFrame((LPTXFRM)pFrm);
    }

    return WL_CMDRES_OPERATING_MLME;
}
#endif

u16 MLME_StartReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMlmeStartReq *pReq = (WlMlmeStartReq *)pReqt;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeStartCfm);

    if ((pConfig->Mode != WL_CMDLABEL_MODE_PARENT) && (pConfig->Mode != WL_CMDLABEL_MODE_TEST)) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pWork->STA != STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (pReq->ssidLength > MAX_SSID_LENGTH) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->ssidLength == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->beaconPeriod < MIN_BEACON_PERIOD) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->beaconPeriod > MAX_BEACON_PERIOD) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->dtimPeriod == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->dtimPeriod > MAX_DTIM_PERIOD) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->channel & 0xFFF0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (CheckEnableChannel(pReq->channel) == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->basicRateSet == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->basicRateSet & ~MASK_BRS) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->supportRateSet == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->supportRateSet & ~MASK_SRS) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->gameInfoLength > MAX_GAMEINFO_LENGTH) {
        return WL_CMDRES_INVALID_PARAM;
    }

#ifndef SDK_NOCHK_ERR_WL
    if (FLASH_VerifyCheckSum(NULL) != 0) {
        return WL_CMDRES_FLASH_ERR;
    }
#endif

    if (pConfig->Mode == WL_CMDLABEL_MODE_TEST) {
        WSetBssid((u16 *)BC_ADRS);
    } else {
        WSetBssid(pConfig->MacAdrs);
    }
    WSetSsid(pReq->ssidLength, pReq->ssid);
    WSetBeaconPeriod(pReq->beaconPeriod);
    WSetDTIMPeriod(pReq->dtimPeriod);
    WSetChannel(pReq->channel, FALSE);
    WSetRateSet((LPRATE_SET)&pReq->basicRateSet);
    WInitGameInfo(pReq->gameInfoLength, pReq->gameInfo);

    pWork->bUpdateGameInfo = FALSE;

    WStart();

    return WL_CMDRES_SUCCESS;
}

u16 MLME_MeasChanReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    LPMLME_MAN pMLME = &wlMan->MLME;
    WlMlmeMeasChanReq *pReq = (WlMlmeMeasChanReq *)pReqt;
    u32 i, ch;

    DbgWlPutchar(B_WL_DBG_MLME, (WL_DBG_MLME_MSCH));

    pCfmt->header.length = CalcCfmMsgLength(WlMlmeMeasChanCfm);

    if (wlMan->Work.STA != STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (pReq->ccaMode > MAX_CCA_MODE) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->edThreshold > MAX_ED_THRESHOLD) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->measureTime == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->measureTime > MAX_MEASURE_TIME) {
        return WL_CMDRES_INVALID_PARAM;
    }
    for (i = 0; i < sizeof(pReq->channelList); i++) {
        ch = WL_ReadByte(&pReq->channelList[i]);

        if (ch == 0) {
            break;
        }

        if (CheckEnableChannel(ch) == 0) {
            return WL_CMDRES_INVALID_PARAM;
        }
    }
    if (i == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }

    pMLME->pReq.MeasChannel = (WlMlmeMeasChanReq *)pReqt;
    pMLME->pCfm.MeasChannel = (WlMlmeMeasChanCfm *)pCfmt;

    pMLME->State = MLME_STATE_MEAS_STARTUP;

    pCfmt->resultCode = WL_CMDRES_OPERATING_MLME;

    MLME_MeasChannelTask();

    return WL_CMDRES_OPERATING_MLME;
}

void MLME_ScanTask(void)
{
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPWORK_PARAM pWork = &wlMan->Work;
    void *pFrm;
    u32 bTask;
    u16 ch;

    bTask = FALSE;

    switch (pMLME->State) {
    case MLME_STATE_SCAN_STARTUP:
        WSetStaState(STA_CLASS1);

        pWork->Mode = WL_CMDLABEL_MODE_CHILD;

        pMLME->pCfm.Scan->bssDescCount = 0;
        pMLME->pCfm.Scan->foundMap = 0;
        pMLME->Work.Scan.ChannelCount = 0;
        pMLME->Work.Scan.bFound = 0;
        if (pMLME->pReq.Scan->scanType == WL_CMDLABEL_SCAN_ACTIVE) {
            pMLME->Work.Scan.TxPeriod = (pMLME->pReq.Scan->maxChannelTime + 3) / 4;
            if (pMLME->Work.Scan.TxPeriod < MIN_CHANNEL_TIME) {
                pMLME->Work.Scan.TxPeriod = MIN_CHANNEL_TIME;
            }
        } else {
            pMLME->Work.Scan.TxPeriod = pMLME->pReq.Scan->maxChannelTime;
        }
#if (EMU_MACREG)
        pMLME->Work.Scan.TxPeriod = pMLME->pReq.Scan->maxChannelTime;
#endif

        pMLME->pCfm.Scan->resultCode = WL_CMDRES_SUCCESS;

    case MLME_STATE_SCAN_SETCHANNEL:

        ch = WL_ReadByte(&pMLME->pReq.Scan->channelList[pMLME->Work.Scan.ChannelCount]);
        if (ch == 0) {
            pMLME->State = MLME_STATE_SCAN_FIN;
            bTask = TRUE;
            break;
        }
        pMLME->Work.Scan.ChannelCount++;

        pMLME->Work.Scan.ElapseTime = 0;

#ifndef SDK_NOCHK_ERR_WL
        if (FLASH_VerifyCheckSum(NULL) != 0) {
            pMLME->pCfm.Scan->resultCode = WL_CMDRES_FLASH_ERR;

            pMLME->State = MLME_STATE_SCAN_FIN;

            bTask = TRUE;
            break;
        }
#endif

        if (pMLME->State == MLME_STATE_SCAN_STARTUP) {
#if (BBP_RF_REGCHK)
            u32 CheckBbpRfRegs(u16 * p);

            WSetChannel(ch, FALSE);

            if (CheckBbpRfRegs(&pMLME->pCfm.Scan->foundMap) != 0) {
                pMLME->pCfm.Scan->resultCode = WL_CMDRES_FAILURE;

                pMLME->State = MLME_STATE_IDLE;

                pWork->Mode = wlMan->Config.Mode;

                IssueMlmeConfirm();
                break;
            }
#else
            WSetChannel(ch, FALSE);
#endif

            WStart();
        } else {
            WSetChannel(ch, FALSE);
        }

        pMLME->State = MLME_STATE_SCAN_START;

    case MLME_STATE_SCAN_START:
    case MLME_STATE_SCAN_ING:
        pMLME->State = MLME_STATE_SCAN_ING;

        if (pMLME->pReq.Scan->scanType == WL_CMDLABEL_SCAN_ACTIVE) {
            pFrm = MakeProbeReqFrame(pMLME->pReq.Scan->bssid);
            if ((u32)pFrm == HEAPBUF_NOT_ENOUGH_MEMORY) {
                pMLME->pCfm.Scan->resultCode = WL_CMDRES_NOT_ENOUGH_MEM;

                pMLME->State = MLME_STATE_SCAN_FIN;
                bTask = TRUE;
                break;
            }

            TxManCtrlFrame((LPTXFRM)pFrm);
        }

        SetupTimeOut(pMLME->Work.Scan.TxPeriod, MLME_ScanTimeOut);
        break;

    case MLME_STATE_SCAN_FIN:
        pMLME->State = MLME_STATE_IDLE;

        WStop();

        pWork->Mode = wlMan->Config.Mode;

        IssueMlmeConfirm();
        break;
    }

    if (bTask) {
        AddTask(TASK_NORMAL_PRIORITY, MLME_SCAN_TASK_ID);
    }
}

static void MLME_ScanTimeOut(void *arg)
{
    LPMLME_MAN pMLME = &wlMan->MLME;

    pMLME->Work.Scan.ElapseTime += pMLME->Work.Scan.TxPeriod;
    if (pMLME->Work.Scan.ElapseTime >= pMLME->pReq.Scan->maxChannelTime) {
        if (pMLME->Work.Scan.ChannelCount < 16) {
            pMLME->State = MLME_STATE_SCAN_SETCHANNEL;
        } else {
            pMLME->State = MLME_STATE_SCAN_FIN;
        }
    } else {
        // why is this here?
    }

    AddTask(TASK_NORMAL_PRIORITY, MLME_SCAN_TASK_ID);
}

void MLME_JoinTask(void)
{
    LPMLME_MAN pMLME = &wlMan->MLME;

    switch (pMLME->State) {
    case MLME_STATE_JOIN_STARTUP:
        WStart();

        pMLME->Work.Join.Result = FALSE;
        pMLME->Work.Join.Status = WL_CMDLABEL_STS_SUCCESSFUL;

        pMLME->State = MLME_STATE_JOIN_ING;

        SetupTimeOut(pMLME->pReq.Join->timeOut, MLME_JoinTimeOut);
        break;

    case MLME_STATE_JOIN_FIN:

        DbgWlPutchar(B_WL_DBG_MLME, (WL_DBG_MLME_JOIN_FIN));
        DbgClrDDO(DDO_WL_CONNECT, DDO_WL_CONNECT_JOIN);

        pMLME->pCfm.Join->resultCode = pMLME->Work.Join.Result;
        pMLME->pCfm.Join->statusCode = pMLME->Work.Join.Status;

        if (pMLME->Work.Join.Result != WL_CMDRES_SUCCESS) {
            WStop();
        }

        pMLME->State = MLME_STATE_IDLE;

        IssueMlmeConfirm();
        break;
    }
}

static void MLME_JoinTimeOut(void *arg)
{
    LPMLME_MAN pMLME = &wlMan->MLME;

    pMLME->Work.Join.Result = WL_CMDRES_TIMEOUT;

    pMLME->State = MLME_STATE_JOIN_FIN;

    AddTask(TASK_NORMAL_PRIORITY, MLME_JOIN_TASK_ID);
}

void MLME_AuthTask(void)
{
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPAUTH_FRAME pFrm;

    switch (pMLME->State) {
    case MLME_STATE_AUTH_STARTUP:
        pFrm = MakeAuthFrame(pMLME->pReq.Auth->peerMacAdrs, 0, FALSE);
        if ((u32)pFrm == HEAPBUF_NOT_ENOUGH_MEMORY) {
            pMLME->pCfm.Auth->resultCode = WL_CMDRES_NOT_ENOUGH_MEM;

            pMLME->State = MLME_STATE_AUTH_FIN;

            AddTask(TASK_NORMAL_PRIORITY, MLME_AUTH_TASK_ID);
        } else {
            pFrm->Body.AlgoType = pMLME->pReq.Auth->algorithm;
            pFrm->Body.SeqNum = 1;
            pFrm->Body.StatusCode = 0;

            pMLME->State = MLME_STATE_AUTH_ING;

            TxManCtrlFrame((LPTXFRM)pFrm);

            SetupTimeOut(pMLME->pReq.Auth->timeOut, MLME_AuthTimeOut);
        }
        break;

    case MLME_STATE_AUTH_FIN:
        ResetTxqPri(QID_MANCTRL);
        ClearQueuedPri(QID_MANCTRL);
        MessageDeleteTx(QID_MANCTRL, FALSE);

        pMLME->State = MLME_STATE_IDLE;

        IssueMlmeConfirm();
        break;
    }
}

static void MLME_AuthTimeOut(void *arg)
{
    LPMLME_MAN pMLME = &wlMan->MLME;

    pMLME->pCfm.Auth->resultCode = WL_CMDRES_TIMEOUT;

    pMLME->State = MLME_STATE_AUTH_FIN;

    AddTask(TASK_NORMAL_PRIORITY, MLME_AUTH_TASK_ID);
}

void MLME_AssTask(void)
{
    LPMLME_MAN pMLME = &wlMan->MLME;
    void *pFrm;

    switch (pMLME->State) {
    case MLME_STATE_ASS_STARTUP:
        pFrm = MakeAssReqFrame(pMLME->pReq.Ass->peerMacAdrs);
        if ((u32)pFrm == HEAPBUF_NOT_ENOUGH_MEMORY) {
            pMLME->pCfm.Ass->resultCode = WL_CMDRES_NOT_ENOUGH_MEM;

            pMLME->State = MLME_STATE_ASS_FIN;

            AddTask(TASK_NORMAL_PRIORITY, MLME_ASS_TASK_ID);
        } else {
            pMLME->State = MLME_STATE_ASS_ING;

            TxManCtrlFrame((LPTXFRM)pFrm);

            SetupTimeOut(pMLME->pReq.Ass->timeOut, MLME_AssTimeOut);
        }
        break;

    case MLME_STATE_ASS_FIN:
        ResetTxqPri(QID_MANCTRL);
        ClearQueuedPri(QID_MANCTRL);
        MessageDeleteTx(QID_MANCTRL, FALSE);

        pMLME->State = MLME_STATE_IDLE;

        IssueMlmeConfirm();
        break;
    }
}

static void MLME_AssTimeOut(void *arg)
{
    LPMLME_MAN pMLME = &wlMan->MLME;

    pMLME->pCfm.Ass->resultCode = WL_CMDRES_TIMEOUT;

    pMLME->State = MLME_STATE_ASS_FIN;

    AddTask(TASK_NORMAL_PRIORITY, MLME_ASS_TASK_ID);
}

void MLME_ReAssTask(void)
{
#ifndef SDK_SMALL_BUILD_WL
    LPMLME_MAN pMLME = &wlMan->MLME;
    void *pFrm;

    switch (pMLME->State) {
    case MLME_STATE_REASS_STARTUP:

        pFrm = MakeReAssReqFrame(pMLME->pReq.ReAss->newApMacAdrs);
        if ((u32)pFrm == HEAPBUF_NOT_ENOUGH_MEMORY) {
            pMLME->pCfm.ReAss->resultCode = WL_CMDRES_NOT_ENOUGH_MEM;

            pMLME->State = MLME_STATE_REASS_FIN;

            AddTask(TASK_NORMAL_PRIORITY, MLME_REASS_TASK_ID);
        } else {
            pMLME->State = MLME_STATE_REASS_ING;

            TxManCtrlFrame((LPTXFRM)pFrm);

            SetupTimeOut(pMLME->pReq.ReAss->timeOut, MLME_ReAssTimeOut);
        }
        break;

    case MLME_STATE_REASS_FIN:

        ClearQueuedPri(QID_MANCTRL);
        MessageDeleteTx(QID_MANCTRL, FALSE);

        pMLME->State = MLME_STATE_IDLE;

        IssueMlmeConfirm();
        break;
    }
#endif
}

#ifndef SDK_SMALL_BUILD_WL
static void MLME_ReAssTimeOut(void *arg)
{
    LPMLME_MAN pMLME = &wlMan->MLME;

    pMLME->pCfm.ReAss->resultCode = WL_CMDRES_TIMEOUT;

    pMLME->State = MLME_STATE_REASS_FIN;

    AddTask(TASK_NORMAL_PRIORITY, MLME_REASS_TASK_ID);
}
#endif

void MLME_MeasChannelTask(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPMLME_MAN pMLME = &wlMan->MLME;
    u32 ch, ratio;

    switch (pMLME->State) {
    case MLME_STATE_MEAS_STARTUP:
        pMLME->Work.Measure.Channel = 0;
        pMLME->Work.Measure.bkCCAMode = BBP_Read(BBPREG_CCA_MODE);
        pMLME->Work.Measure.bkEdTh = BBP_Read(BBPREG_ED_TH);
        WSetCCA_ED(pMLME->pReq.MeasChannel->ccaMode, pMLME->pReq.MeasChannel->edThreshold);

        pWork->Mode = WL_CMDLABEL_MODE_MEASCHAN;

        pMLME->Work.Measure.sts = WL_CMDRES_SUCCESS;

    case MLME_STATE_MEAS_SETCHANNEL:
        pMLME->Work.Measure.Counter = 0;
        pMLME->Work.Measure.CCA = 0;

        ch = WL_ReadByte(&pMLME->pReq.MeasChannel->channelList[pMLME->Work.Measure.Channel]);
        if ((ch == 0) || (pMLME->Work.Measure.Channel >= sizeof(pMLME->pReq.MeasChannel->channelList))) {
            pMLME->State = MLME_STATE_MEAS_FIN;
            break;
        }

#ifndef SDK_NOCHK_ERR_WL
        if (FLASH_VerifyCheckSum(NULL) != 0) {
            pMLME->Work.Measure.sts = WL_CMDRES_FLASH_ERR;

            pMLME->State = MLME_STATE_MEAS_FIN;
            break;
        }
#endif

        if (pMLME->State == MLME_STATE_MEAS_STARTUP) {
            WSetChannel(ch, FALSE);

            WStart();

            pMLME->Work.Measure.bkPowerMode = *(vu16 *)MREG_SET_FORCE_POWER;
            WSetForcePowerState(FPWR_STS_ACTIVE);
        } else {
            WSetChannel(ch, FALSE);
        }

        pMLME->State = MLME_STATE_MEAS_MEASURE;

        SetupTimeOut(pMLME->pReq.MeasChannel->measureTime, MLME_MeasChanTimeOut);

    case MLME_STATE_MEAS_MEASURE:

        pMLME->Work.Measure.Counter++;
        if (*(vu16 *)MREG_SIGNAL_STATE & SIGNAL_STATE_CCA) {
            pMLME->Work.Measure.CCA += 100;
        }
        break;

    case MLME_STATE_MEAS_END:

        ch = WL_ReadByte(&pMLME->pReq.MeasChannel->channelList[pMLME->Work.Measure.Channel]);
        ratio = 0;
        if (pMLME->Work.Measure.Counter != 0) {
            if (pMLME->Work.Measure.CCA) {
                ratio = (pMLME->Work.Measure.CCA / pMLME->Work.Measure.Counter) + 1;
                if (ratio > 100) {
                    ratio = 100;
                }
            }
        }
        pMLME->pCfm.MeasChannel->ccaBusyInfo[pMLME->Work.Measure.Channel] = ch | (ratio << 8);

        pMLME->Work.Measure.Channel++;
        pMLME->State = MLME_STATE_MEAS_SETCHANNEL;
        break;

    case MLME_STATE_MEAS_FIN:
        WStop();
        pWork->Mode = wlMan->Config.Mode;
        BBP_Write(BBPREG_CCA_MODE, pMLME->Work.Measure.bkCCAMode);
        BBP_Write(BBPREG_ED_TH, pMLME->Work.Measure.bkEdTh);
        WSetForcePowerState(pMLME->Work.Measure.bkPowerMode);

        pMLME->pCfm.MeasChannel->resultCode = pMLME->Work.Measure.sts;

        pMLME->State = MLME_STATE_IDLE;

        for (ch = pMLME->Work.Measure.Channel; ch < sizeof(pMLME->pReq.MeasChannel->channelList); ch++) {
            pMLME->pCfm.MeasChannel->ccaBusyInfo[ch] = 0;
        }

        IssueMlmeConfirm();
        break;
    }

    if (pMLME->State != MLME_STATE_IDLE) {
        AddTask(TASK_NORMAL_PRIORITY, MLME_MEASCHAN_TASK_ID);
    }
}

static void MLME_MeasChanTimeOut(void *arg)
{
    wlMan->MLME.State = MLME_STATE_MEAS_END;

    AddTask(TASK_NORMAL_PRIORITY, MLME_MEASCHAN_TASK_ID);
}

void MLME_BeaconLostTask(void)
{
    MLME_IssueBeaconLostIndication(wlMan->Work.LinkAdrs);
}

void IssueMlmeConfirm(void)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPCMDIF_MAN pCmdIf = &wlMan->CmdIf;

    pCmdIf->Busy &= ~CMDBUSY_MLME;

    SendMessageToWmDirect(&pHeapMan->RequestCmd, pCmdIf->pCmd);

    if (GetHeapBufCount(&pHeapMan->RequestCmd) != 0) {
        AddTask(TASK_NORMAL_PRIORITY, REQUEST_CMD_TASK_ID);
    }
}

u32 MLME_IssueAuthIndication(u16 *pMacAdrs, u16 algorithm)
{
    WlMlmeAuthInd *pInd;

    pInd = (WlMlmeAuthInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeAuthInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_AUTH_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeAuthInd);

    WSetMacAdrs1(pInd->peerMacAdrs, pMacAdrs);
    pInd->algorithm = algorithm;

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

u32 MLME_IssueDeAuthIndication(u16 *pMacAdrs, u16 reason)
{
    WlMlmeDeAuthInd *pInd;

    pInd = (WlMlmeDeAuthInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeDeAuthInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_DEAUTH_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeDeAuthInd);

    WSetMacAdrs1(pInd->peerMacAdrs, pMacAdrs);
    pInd->reasonCode = reason;

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

u32 MLME_IssueAssIndication(u16 *pMacAdrs, u16 aid, LPSSID_ELEMENT pSSID)
{
    WlMlmeAssInd *pInd;
    u32 i;

    pInd = (WlMlmeAssInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeAssInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_ASS_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeAssInd);

    WSetMacAdrs1(pInd->peerMacAdrs, pMacAdrs);
    pInd->aid = aid & 0x0FFF;
    pInd->ssidLength = WL_ReadByte(&pSSID->Length);
    for (i = 0; i < pInd->ssidLength; i++) {
        if (i >= MAX_SSID_LENGTH) {
            break;
        }
        WL_WriteByte(&pInd->ssid[i], WL_ReadByte(&pSSID->SSID[i]));
    }
    for (; i < MAX_SSID_LENGTH; i++) {
        WL_WriteByte(&pInd->ssid[i], 0);
    }

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

u32 MLME_IssueReAssIndication(u16 *pMacAdrs, u16 aid, LPSSID_ELEMENT pSSID)
{
    WlMlmeReAssInd *pInd;
    u32 i;

    pInd = (WlMlmeReAssInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeReAssInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_REASS_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeReAssInd);

    WSetMacAdrs1(pInd->peerMacAdrs, pMacAdrs);
    pInd->aid = aid;
    pInd->ssidLength = WL_ReadByte(&pSSID->Length);
    for (i = 0; i < pInd->ssidLength; i++) {
        if (i >= MAX_SSID_LENGTH) {
            break;
        }
        WL_WriteByte(&pInd->ssid[i], WL_ReadByte(&pSSID->SSID[i]));
    }
    for (; i < MAX_SSID_LENGTH; i++) {
        WL_WriteByte(&pInd->ssid[i], 0);
    }

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

u32 MLME_IssueDisAssIndication(u16 *pMacAdrs, u16 reason)
{
    WlMlmeDisAssInd *pInd;

    pInd = (WlMlmeDisAssInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeDisAssInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_DISASS_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeDisAssInd);

    WSetMacAdrs1(pInd->peerMacAdrs, pMacAdrs);
    pInd->reasonCode = reason;

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

u32 MLME_IssueBeaconLostIndication(u16 *pMacAdrs)
{
    WlMlmeBeaconLostInd *pInd;

    pInd = (WlMlmeBeaconLostInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeBeaconLostInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_BCLOST_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeBeaconLostInd);

    WSetMacAdrs1(pInd->apMacAdrs, pMacAdrs);

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

u32 MLME_IssueBeaconSendIndication(void)
{
    WlMlmeBeaconSendInd *pInd;

    pInd = (WlMlmeBeaconSendInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeBeaconSendInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_BCSEND_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeBeaconSendInd);

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

u32 MLME_IssueBeaconRecvIndication(void *pRxFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPRXFRM pFrm = (LPRXFRM)pRxFrm;
    WlMlmeBeaconRecvInd *pInd;
    u32 i;
    u8 *p1, *p2;

    pInd = (WlMlmeBeaconRecvInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMlmeBeaconRecvInd) + wlMan->Work.GameInfoLength);
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pInd->header.code = WL_CMDCODE_MLME_BCRECV_IND;
    pInd->header.length = CalcIndMsgLength(WlMlmeBeaconRecvInd) - 1 + (pWork->GameInfoLength + 1) / 2;
    WL_WriteByte(&pInd->rssi, pFrm->MacHeader.Rx.rsv_RSSI);
    WL_WriteByte(&pInd->rate, pFrm->MacHeader.Rx.Service_Rate);
    WSetMacAdrs1(pInd->srcMacAdrs, pFrm->Dot11Header.Adrs2);

    pInd->gameInfoLength = pWork->GameInfoLength;
    if (pInd->gameInfoLength != 0) {
        if (pWork->GameInfoAlign & 1) {
            p1 = (u8 *)((u32)pWork->GameInfoAdrs + 1);
            p2 = (u8 *)pInd->gameInfo;
            for (i = 0; i < pInd->gameInfoLength; i++) {
                WL_WriteByte(p2, WL_ReadByte(p1));
                p1++;
                p2++;
            }
        } else {
            WLLIB_DmaCopy16((u32)pWork->GameInfoAdrs, (u32)pInd->gameInfo, pInd->gameInfoLength + 1);
        }
    }

    IssueMlmeIndication(&wlMan->HeapMan.TmpBuf, pInd);

    return TRUE;
}

void InitializeMLME(void)
{
    WLLIB_DmaClear16((u32)&wlMan->MLME, sizeof(MLME_MAN)); // ìÆÌæ NA
}
