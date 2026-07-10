#ifndef __PARAMCMD_H_
#define __PARAMCMD_H_

u16 PARAMSET_AllReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_MacAdrsReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_RetryReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_EnableChannelReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_ModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_RateReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_WepModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_WepKeyIdReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_WepKeyReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_BeaconTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_ResBcSsidReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_BeaconLostThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_ActiveZoneReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_SSIDMaskReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_PreambleTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_AuthAlgoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_CCAModeEDThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_LifeTimeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_MaxConnReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_MainAntennaReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_DiversityReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_BcnSendRecvIndReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_NullKeyModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

u16 PARAMSET_BSSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_SSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_BeaconPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_DTIMPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_ListenIntervalReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMSET_GameInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

u16 PARAMGET_AllReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_MacAdrsReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_RetryReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_EnableChannelReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_ModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_RateReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_WepModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_WepKeyIdReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_WepKeyReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_BeaconTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_ResBcSsidReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_BeaconLostThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_ActiveZoneReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_SSIDMaskReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_PreambleTypeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_AuthAlgoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_CCAModeEDThReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_MaxConnReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_MainAntennaReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_DiversityReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_BcnSendRecvIndReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_NullKeyModeReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

u16 PARAMGET_BSSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_SSIDReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_BeaconPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_DTIMPeriodReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_ListenIntervalReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 PARAMGET_GameInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

#endif // __PARAMCMD_H_
