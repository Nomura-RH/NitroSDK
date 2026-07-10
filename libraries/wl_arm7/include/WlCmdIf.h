#ifndef __WLCMDIF_H_
#define __WLCMDIF_H_

void InitializeCmdIf(void);
void SendMessageToWmDirect(LPHEAPBUF_MAN pBufMan, void *pMsg);
void SendMessageToWmTask(void);

void RequestCmdTask(void);

#define PARAM_FLAG_DEV_IFC (0x01UL << 0)
#define PARAM_FLAG_CONFIG  (PARAM_FLAG_MACADRS | PARAM_FLAG_ENCH)
#define PARAM_FLAG_MACADRS (0x01UL << 1)
#define PARAM_FLAG_ENCH    (0x01UL << 2)
#define PARAM_FLAG_MODE    (0x01UL << 3)

#define CMDBUSY_MLME      0x01
#define CMDBUSY_MA        0x02
#define CMDBUSY_PARAMSET  0x04
#define CMDBUSY_PARAMSET2 0x08
#define CMDBUSY_PARAMGET  0x10
#define CMDBUSY_PARAMGET2 0x20
#define CMDBUSY_DEV       0x40

#define WL_CMDRES_OPERATING_MLME 0x80
#define WL_CMDRES_OPERATING_MA   0x81

#define CalcReqMsgLength(_x_) \
    ((sizeof(_x_) - WL_RSV * 2 - sizeof(WlCmdHeader) + 1) / 2)
#define CalcCfmMsgLength(_x_) ((sizeof(_x_) - sizeof(WlCmdHeader) + 1) / 2)
#define CalcIndMsgLength(_x_) \
    ((sizeof(_x_) - WL_RSV * 2 - sizeof(WlCmdHeader) + 1) / 2)
#define CalcIndMsgLength2(_x_, _y_) \
    ((sizeof(_x_) - WL_RSV * 2 - sizeof(WlCmdHeader) + _y_ + 1) / 2)

typedef struct {
    u8 *pCmd;
    u16 Busy;
    u16 pad;
} CMDIF_MAN, *LPCMDIF_MAN;

#ifdef __WLCMDIF_C_

typedef struct {
    u16 RequestMinLength;
    u16 ConfirmMinLength;
    u16 (*pCmdFunc)(WlCmdReq *pReq, WlCmdCfm *pCfm);
} WLLIB_CMD_TBL, *LPWLLIB_CMD_TBL;

#define MLME_REQ_NUM      (sizeof(WlibCmdTbl_MLME) / sizeof(WLLIB_CMD_TBL))
#define MA_REQ_NUM        (sizeof(WlibCmdTbl_MA) / sizeof(WLLIB_CMD_TBL))
#define PARAMSET_REQ_NUM  (sizeof(WlibCmdTbl_PARAMSET) / sizeof(WLLIB_CMD_TBL))
#define PARAMSET2_REQ_NUM (sizeof(WlibCmdTbl_PARAMSET2) / sizeof(WLLIB_CMD_TBL))
#define PARAMGET_REQ_NUM  (sizeof(WlibCmdTbl_PARAMGET) / sizeof(WLLIB_CMD_TBL))
#define PARAMGET2_REQ_NUM (sizeof(WlibCmdTbl_PARAMGET2) / sizeof(WLLIB_CMD_TBL))
#define DEV_REQ_NUM       (sizeof(WlibCmdTbl_DEV) / sizeof(WLLIB_CMD_TBL))
#define SYS_REQ_NUM       (sizeof(WlibCmdTbl_SYS) / sizeof(WLLIB_CMD_TBL))

#define PARAMSET_STR_OFST  (0x00)
#define PARAMSET2_STR_OFST (0x40)
#define PARAMGET_STR_OFST  (0x80)
#define PARAMGET2_STR_OFST (0xC0)

#define MLME_RESET_REQ_MINSIZE    CalcReqMsgLength(WlMlmeResetReq)
#define MLME_PWRMGT_REQ_MINSIZE   CalcReqMsgLength(WlMlmePowerMgtReq)
#define MLME_SCAN_REQ_MINSIZE     (CalcReqMsgLength(WlMlmeScanReq) - 3)
#define MLME_JOIN_REQ_MINSIZE     (CalcReqMsgLength(WlMlmeJoinReq) - 4 / 2)
#define MLME_AUTH_REQ_MINSIZE     CalcReqMsgLength(WlMlmeAuthReq)
#define MLME_DEAUTH_REQ_MINSIZE   CalcReqMsgLength(WlMlmeDeAuthReq)
#define MLME_ASS_REQ_MINSIZE      CalcReqMsgLength(WlMlmeAssReq)
#define MLME_REASS_REQ_MINSIZE    CalcReqMsgLength(WlMlmeReAssReq)
#define MLME_DISASS_REQ_MINSIZE   CalcReqMsgLength(WlMlmeDisAssReq)
#define MLME_START_REQ_MINSIZE    (CalcReqMsgLength(WlMlmeStartReq) - 1)
#define MLME_MEASCHAN_REQ_MINSIZE CalcReqMsgLength(WlMlmeMeasChanReq)

#define MLME_RESET_CFM_MINSIZE    CalcCfmMsgLength(WlMlmeResetCfm)
#define MLME_PWRMGT_CFM_MINSIZE   CalcCfmMsgLength(WlMlmePowerMgtCfm)
#define MLME_SCAN_CFM_MINSIZE     (CalcCfmMsgLength(WlMlmeScanCfm) - 4 / 2)
#define MLME_JOIN_CFM_MINSIZE     CalcCfmMsgLength(WlMlmeJoinCfm)
#define MLME_AUTH_CFM_MINSIZE     CalcCfmMsgLength(WlMlmeAuthCfm)
#define MLME_DEAUTH_CFM_MINSIZE   CalcCfmMsgLength(WlMlmeDeAuthCfm)
#define MLME_ASS_CFM_MINSIZE      CalcCfmMsgLength(WlMlmeAssCfm)
#define MLME_REASS_CFM_MINSIZE    CalcCfmMsgLength(WlMlmeReAssCfm)
#define MLME_DISASS_CFM_MINSIZE   CalcCfmMsgLength(WlMlmeDisAssCfm)
#define MLME_START_CFM_MINSIZE    CalcCfmMsgLength(WlMlmeStartCfm)
#define MLME_MEASCHAN_CFM_MINSIZE CalcCfmMsgLength(WlMlmeMeasChanCfm)

#define MA_DATA_REQ_MINSIZE     CalcReqMsgLength(WlMaDataReq)
#define MA_KEYDATA_REQ_MINSIZE  CalcReqMsgLength(WlMaKeyDataReq)
#define MA_MP_REQ_MINSIZE       CalcReqMsgLength(WlMaMpReq)
#define MA_TESTDATA_REQ_MINSIZE CalcReqMsgLength(WlMaTestDataReq)
#define MA_CLRDATA_REQ_MINSIZE  CalcReqMsgLength(WlMaClrDataReq)

#define MA_DATA_CFM_MINSIZE     CalcCfmMsgLength(WlMaDataCfm)
#define MA_KEYDATA_CFM_MINSIZE  CalcCfmMsgLength(WlMaKeyDataCfm)
#define MA_MP_CFM_MINSIZE       CalcCfmMsgLength(WlMaMpCfm)
#define MA_TESTDATA_CFM_MINSIZE CalcCfmMsgLength(WlMaTestDataCfm)
#define MA_CLRDATA_CFM_MINSIZE  CalcCfmMsgLength(WlMaClrDataCfm)

#define PARAMSET_ALL_REQ_MINSIZE         CalcReqMsgLength(WlParamSetAllReq)
#define PARAMSET_MACADRS_REQ_MINSIZE     CalcReqMsgLength(WlParamSetMacAdrsReq)
#define PARAMSET_RETRY_REQ_MINSIZE       CalcReqMsgLength(WlParamSetRetryLimitReq)
#define PARAMSET_ENCH_REQ_MINSIZE        CalcReqMsgLength(WlParamSetEnableChannelReq)
#define PARAMSET_MODE_REQ_MINSIZE        CalcReqMsgLength(WlParamSetModeReq)
#define PARAMSET_RATE_REQ_MINSIZE        CalcReqMsgLength(WlParamSetRateReq)
#define PARAMSET_WEPMODE_REQ_MINSIZE     CalcReqMsgLength(WlParamSetWepModeReq)
#define PARAMSET_WEPKEYID_REQ_MINSIZE    CalcReqMsgLength(WlParamSetWepKeyIdReq)
#define PARAMSET_WEPKEY_REQ_MINSIZE      CalcReqMsgLength(WlParamSetWepKeyReq)
#define PARAMSET_BCN_TYPE_REQ_MINSIZE    CalcReqMsgLength(WlParamSetBeaconTypeReq)
#define PARAMSET_RES_BC_SSID_REQ_MINSIZE CalcReqMsgLength(WlParamSetProbeResReq)
#define PARAMSET_BCN_LOST_REQ_MINSIZE \
    CalcReqMsgLength(WlParamSetBeaconLostThReq)
#define PARAMSET_ACT_ZONE_REQ_MINSIZE  CalcReqMsgLength(WlParamSetActiveZoneReq)
#define PARAMSET_SSID_MASK_REQ_MINSIZE CalcReqMsgLength(WlParamSetSsidMaskReq)
#define PARAMSET_PREAMBLE_TYPE_REQ_MINSIZE \
    CalcReqMsgLength(WlParamSetPreambleTypeReq)
#define PARAMSET_AUTHALGO_REQ_MINSIZE  CalcReqMsgLength(WlParamSetAuthAlgoReq)
#define PARAMSET_CCA_EDTH_REQ_MINSIZE  CalcReqMsgLength(WlParamSetCCAModeEDThReq)
#define PARAMSET_LIFE_TIME_REQ_MINSIZE CalcReqMsgLength(WlParamSetLifeTimeReq)
#define PARAMSET_MAX_CONN_REQ_MINSIZE  CalcReqMsgLength(WlParamSetMaxConnReq)
#define PARAMSET_MAIN_ANTENNA_REQ_MINSIZE \
    CalcReqMsgLength(WlParamSetMainAntennaReq)
#define PARAMSET_DIVERSITY_REQ_MINSIZE CalcReqMsgLength(WlParamSetDiversityReq)
#define PARAMSET_BCNTXRXIND_REQ_MINSIZE \
    CalcReqMsgLength(WlParamSetBeaconSendRecvIndReq)
#define PARAMSET_NULLKEYMODE_REQ_MINSIZE \
    CalcReqMsgLength(WlParamSetNullKeyModeReq)

#define PARAMSET_BSSID_REQ_MINSIZE CalcReqMsgLength(WlParamSetBssidReq)
#define PARAMSET_SSID_REQ_MINSIZE  CalcReqMsgLength(WlParamSetSsidReq)
#define PARAMSET_BCN_PERIOD_REQ_MINSIZE \
    CalcReqMsgLength(WlParamSetBeaconPeriodReq)
#define PARAMSET_DTIM_PERIOD_REQ_MINSIZE \
    CalcReqMsgLength(WlParamSetDtimPeriodReq)
#define PARAMSET_LSN_INT_REQ_MINSIZE CalcReqMsgLength(WlParamSetIntervalReq)
#define PARAMSET_GAME_INFO_REQ_MINSIZE \
    (CalcReqMsgLength(WlParamSetGameInfoReq) - 2)

#define PARAMSET_ALL_CFM_MINSIZE           CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_MACADRS_CFM_MINSIZE       CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_RETRY_CFM_MINSIZE         CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_ENCH_CFM_MINSIZE          CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_MODE_CFM_MINSIZE          CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_RATE_CFM_MINSIZE          CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_WEPMODE_CFM_MINSIZE       CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_WEPKEYID_CFM_MINSIZE      CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_WEPKEY_CFM_MINSIZE        CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_BCN_TYPE_CFM_MINSIZE      CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_RES_BC_SSID_CFM_MINSIZE   CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_BCN_LOST_CFM_MINSIZE      CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_ACT_ZONE_CFM_MINSIZE      CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_SSID_MASK_CFM_MINSIZE     CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_PREAMBLE_TYPE_CFM_MINSIZE CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_AUTHALGO_CFM_MINSIZE      CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_CCA_EDTH_CFM_MINSIZE      CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_LIFE_TIME_CFM_MINSIZE     CalcCfmMsgLength(WlParamSetCfm)

#define PARAMSET_BSSID_CFM_MINSIZE        CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_SSID_CFM_MINSIZE         CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_BCN_PERIOD_CFM_MINSIZE   CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_DTIM_PERIOD_CFM_MINSIZE  CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_LSN_INT_CFM_MINSIZE      CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_GAME_INFO_CFM_MINSIZE    CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_MAX_CONN_CFM_MINSIZE     CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_MAIN_ANTENNA_CFM_MINSIZE CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_DIVERSITY_CFM_MINSIZE    CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_BCNTXRXIND_CFM_MINSIZE   CalcCfmMsgLength(WlParamSetCfm)
#define PARAMSET_NULLKEYMODE_CFM_MINSIZE  CalcCfmMsgLength(WlParamSetCfm)

#define PARAMGET_ALL_REQ_MINSIZE           CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_MACADRS_REQ_MINSIZE       CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_RETRY_REQ_MINSIZE         CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_ENCH_REQ_MINSIZE          CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_MODE_REQ_MINSIZE          CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_RATE_REQ_MINSIZE          CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_WEPMODE_REQ_MINSIZE       CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_WEPKEYID_REQ_MINSIZE      CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_BCN_TYPE_REQ_MINSIZE      CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_RES_BC_SSID_REQ_MINSIZE   CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_BCN_LOST_REQ_MINSIZE      CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_ACT_ZONE_REQ_MINSIZE      CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_SSID_MASK_REQ_MINSIZE     CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_PREAMBLE_TYPE_REQ_MINSIZE CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_AUTHALGO_REQ_MINSIZE      CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_CCA_EDTH_REQ_MINSIZE      CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_MAX_CONN_REQ_MINSIZE      CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_MAIN_ANTENNA_REQ_MINSIZE  CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_DIVERSITY_REQ_MINSIZE     CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_BCNTXRXIND_REQ_MINSIZE    CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_NULLKEYMODE_REQ_MINSIZE   CalcReqMsgLength(WlParamGetReq)

#define PARAMGET_BSSID_REQ_MINSIZE       CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_SSID_REQ_MINSIZE        CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_BCN_PERIOD_REQ_MINSIZE  CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_DTIM_PERIOD_REQ_MINSIZE CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_LSN_INT_REQ_MINSIZE     CalcReqMsgLength(WlParamGetReq)
#define PARAMGET_GAME_INFO_REQ_MINSIZE   CalcReqMsgLength(WlParamGetReq)

#define PARAMGET_ALL_CFM_MINSIZE         CalcCfmMsgLength(WlParamGetAllCfm)
#define PARAMGET_MACADRS_CFM_MINSIZE     CalcCfmMsgLength(WlParamGetMacAdrsCfm)
#define PARAMGET_RETRY_CFM_MINSIZE       CalcCfmMsgLength(WlParamGetRetryLimitCfm)
#define PARAMGET_ENCH_CFM_MINSIZE        CalcCfmMsgLength(WlParamGetEnableChannelCfm)
#define PARAMGET_MODE_CFM_MINSIZE        CalcCfmMsgLength(WlParamGetModeCfm)
#define PARAMGET_RATE_CFM_MINSIZE        CalcCfmMsgLength(WlParamGetRateCfm)
#define PARAMGET_WEPMODE_CFM_MINSIZE     CalcCfmMsgLength(WlParamGetWepModeCfm)
#define PARAMGET_WEPKEYID_CFM_MINSIZE    CalcCfmMsgLength(WlParamGetWepKeyIdCfm)
#define PARAMGET_BCN_TYPE_CFM_MINSIZE    CalcCfmMsgLength(WlParamGetBeaconTypeCfm)
#define PARAMGET_RES_BC_SSID_CFM_MINSIZE CalcCfmMsgLength(WlParamGetProbeResCfm)
#define PARAMGET_BCN_LOST_CFM_MINSIZE \
    CalcCfmMsgLength(WlParamGetBeaconLostThCfm)
#define PARAMGET_ACT_ZONE_CFM_MINSIZE  CalcCfmMsgLength(WlParamGetActiveZoneCfm)
#define PARAMGET_SSID_MASK_CFM_MINSIZE CalcCfmMsgLength(WlParamGetSsidMaskCfm)
#define PARAMGET_PREAMBLE_TYPE_CFM_MINSIZE \
    CalcCfmMsgLength(WlParamGetPreambleTypeCfm)
#define PARAMGET_AUTHALGO_CFM_MINSIZE CalcCfmMsgLength(WlParamGetAuthAlgoCfm)
#define PARAMGET_CCA_EDTH_CFM_MINSIZE CalcCfmMsgLength(WlParamGetCCAModeEDThCfm)
#define PARAMGET_MAX_CONN_CFM_MINSIZE CalcCfmMsgLength(WlParamGetMaxConnCfm)
#define PARAMGET_MAIN_ANTENNA_CFM_MINSIZE \
    CalcCfmMsgLength(WlParamGetMainAntennaCfm)
#define PARAMGET_DIVERSITY_CFM_MINSIZE CalcCfmMsgLength(WlParamGetDiversityCfm)
#define PARAMGET_BCNTXRXIND_CFM_MINSIZE \
    CalcCfmMsgLength(WlParamGetBeaconSendRecvIndCfm)
#define PARAMGET_NULLKEYMODE_CFM_MINSIZE \
    CalcCfmMsgLength(WlParamGetNullKeyModeCfm)

#define PARAMGET_BSSID_CFM_MINSIZE CalcCfmMsgLength(WlParamGetBssidCfm)
#define PARAMGET_SSID_CFM_MINSIZE  CalcCfmMsgLength(WlParamGetSsidCfm)
#define PARAMGET_BCN_PERIOD_CFM_MINSIZE \
    CalcCfmMsgLength(WlParamGetBeaconPeriodCfm)
#define PARAMGET_DTIM_PERIOD_CFM_MINSIZE \
    CalcCfmMsgLength(WlParamGetDtimPeriodCfm)
#define PARAMGET_LSN_INT_CFM_MINSIZE CalcCfmMsgLength(WlParamGetIntervalCfm)
#define PARAMGET_GAME_INFO_CFM_MINSIZE \
    (CalcCfmMsgLength(WlParamGetGameInfoCfm) - 2)

#define DEV_SHUTDOWN_REQ_MINSIZE    CalcReqMsgLength(WlDevShutdownReq)
#define DEV_IDLE_REQ_MINSIZE        CalcReqMsgLength(WlDevIdleReq)
#define DEV_CLASS1_REQ_MINSIZE      CalcReqMsgLength(WlDevClass1Req)
#define DEV_REBOOT_REQ_MINSIZE      CalcReqMsgLength(WlDevRebootReq)
#define DEV_CLR_WLINFO_REQ_MINSIZE  CalcReqMsgLength(WlDevClrInfoReq)
#define DEV_GET_VERINFO_REQ_MINSIZE CalcReqMsgLength(WlDevGetVerInfoReq)
#define DEV_GET_WLINFO_REQ_MINSIZE  CalcReqMsgLength(WlDevGetInfoReq)
#define DEV_GET_STATE_REQ_MINSIZE   CalcReqMsgLength(WlDevGetStateReq)
#define DEV_TEST_SIGNAL_REQ_MINSIZE CalcReqMsgLength(WlDevTestSignalReq)
#define DEV_TEST_RX_REQ_MINSIZE     CalcReqMsgLength(WlDevTestRxReq)

#define DEV_IFC_CFM_MINSIZE         CalcCfmMsgLength(WlDevIfcCfm)
#define DEV_SHUTDOWN_CFM_MINSIZE    CalcCfmMsgLength(WlDevShutdownCfm)
#define DEV_IDLE_CFM_MINSIZE        CalcCfmMsgLength(WlDevIdleCfm)
#define DEV_CLASS1_CFM_MINSIZE      CalcCfmMsgLength(WlDevClass1Cfm)
#define DEV_REBOOT_CFM_MINSIZE      CalcCfmMsgLength(WlDevRebootCfm)
#define DEV_CLR_WLINFO_CFM_MINSIZE  CalcCfmMsgLength(WlDevClrInfoCfm)
#define DEV_GET_VERINFO_CFM_MINSIZE CalcCfmMsgLength(WlDevGetVerInfoCfm)
#define DEV_GET_WLINFO_CFM_MINSIZE  CalcCfmMsgLength(WlDevGetInfoCfm)
#define DEV_GET_STATE_CFM_MINSIZE   CalcCfmMsgLength(WlDevGetStateCfm)
#define DEV_TEST_SIGNAL_CFM_MINSIZE CalcCfmMsgLength(WlDevTestSignalCfm)
#define DEV_TEST_RX_CFM_MINSIZE     CalcCfmMsgLength(WlDevTestRxCfm)

#define CMD_RESERVED_REQ_MINSIZE 0
#define CMD_RESERVED_CFM_MINSIZE 1

u16 CMD_ReservedReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

#endif // __WLCMDIF_C_
#endif // __WLCMDIF_H_
