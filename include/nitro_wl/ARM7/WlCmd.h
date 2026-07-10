#ifndef	__WLCMD_H_
#define	__WLCMD_H_


#include "WlFrame.h"
#include "WlBuf.h"

#define	WL_CalcHeaderLength(_len_)	((_len_+1)/2)
#define	WL_CalcConfirmPointer(_p_)	((u32)_p_ + ((WlCmdReq*)_p_)->header.length*2 + 12+4)

#define	WL_RSV	(12/2)

typedef	struct {
	u16		code;
	u16		length;
} WlCmdHeader;

typedef	struct {
	u16		length;
	u16		rssi;
	u16		bssid[3];
	u16		ssidLength;
	u8		ssid[32];
	u16		capaInfo;
	struct {
		u16	basic;
		u16	support;
	} rateSet;
	u16		beaconPeriod;
	u16		dtimPeriod;
	u16		channel;
	u16		cfpPeriod;
	u16		cfpMaxDuration;
	u16		gameInfoLength;
	u16		otherElementCount;
	union _element {
		u8		gameInfo[4];
		u8		otherElement[4];
	};
} WlBssDesc;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				mib;
} WlMlmeResetReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMlmeResetCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				pwrMgtMode;
	u16				wakeUp;
	u16				recieveDtims;
} WlMlmePowerMgtReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMlmePowerMgtCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				bssid[3];
	u16				ssidLength;
	u8				ssid[32];
	u16				scanType;
	u8				channelList[16];
	u16				maxChannelTime;
	u16				bssidMaskCount;
	u16				bssidMask[3];
} WlMlmeScanReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
	u16				foundMap;
	u16				bssDescCount;
	WlBssDesc		bssDescList[1];
} WlMlmeScanCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				timeOut;
	u16				rsv;
	WlBssDesc		bssDesc;
} WlMlmeJoinReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
	u16				statusCode;
	u16				peerMacAdrs[3];
} WlMlmeJoinCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				algorithm;
	u16				timeOut;
} WlMlmeAuthReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
	u16				statusCode;
	u16				peerMacAdrs[3];
	u16				algorithm;
} WlMlmeAuthCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				reasonCode;
} WlMlmeDeAuthReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				peerMacAdrs[3];
} WlMlmeDeAuthCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				listenInterval;
	u16				timeOut;
} WlMlmeAssReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				statusCode;
	u16				aid;
} WlMlmeAssCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				newApMacAdrs[3];
	u16				listenInterval;
	u16				timeOut;
} WlMlmeReAssReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				statusCode;
	u16				aid;
} WlMlmeReAssCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				reasonCode;
} WlMlmeDisAssReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMlmeDisAssCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				ssidLength;
	u8				ssid[32];
	u16				beaconPeriod;
	u16				dtimPeriod;
	u16				channel;
	u16				basicRateSet;
	u16				supportRateSet;
	u16				gameInfoLength;
	u8				gameInfo[2];
} WlMlmeStartReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMlmeStartCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				rsv;
	u16				ccaMode;
	u16				edThreshold;
	u16				measureTime;
	u8				channelList[16];
} WlMlmeMeasChanReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				reserved;
	u16				ccaBusyInfo[16];
} WlMlmeMeasChanCfm;




typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				algorithm;
} WlMlmeAuthInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				reasonCode;
} WlMlmeDeAuthInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				aid;
	u16				ssidLength;
	u8				ssid[32];
} WlMlmeAssInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				aid;
	u16				ssidLength;
	u8				ssid[32];
} WlMlmeReAssInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				peerMacAdrs[3];
	u16				reasonCode;
} WlMlmeDisAssInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				apMacAdrs[3];
} WlMlmeBeaconLostInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlMlmeBeaconSendInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				reserved1[3];
	u16				gameInfoLength;

	u16				reserved2[3];
	u8				rate;
	u8				rssi;

	u16				reserved3[4];
	u16				reserved4[3];

	u16				srcMacAdrs[3];
	u16				reserved5[4];

	u16				gameInfo[1];
} WlMlmeBeaconRecvInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	WlTxFrame		frame;
} WlMaDataReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
	u16				txStatus;
} WlMaDataCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				length;
	u16				wmHeader;
	u16*			keyDatap;
} WlMaKeyDataReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMaKeyDataCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				resume;
	u16				retryLimit;
	u16				txop;
	u16				pollBitmap;
	u16				tmptt;
	u16				currTsf;
	u16				dataLength;
	u16				wmHeader;
	u16*			datap;
} WlMaMpReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMaMpCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	WlTestFrame		frame;
} WlMaTestDataReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMaTestDataCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				flag;
} WlMaClrDataReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlMaClrDataCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	WlRxFrame		frame;
} WlMaDataInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	WlRxMpFrame		frame;
} WlMaMpInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	WlMpKey			mpKey;
} WlMaMpEndInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	WlRxMpAckFrame	ack;
} WlMaMpAckInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				errCode;
} WlMaFatalErrInd;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				staMacAdrs[3];
	u16				retryLimit;
	u16				enableChannel;
	u16				rsv;
	u16				mode;
	u16				rate;
	u16				wepMode;
	u16				wepKeyId;
	u16				wepKey[4][10];
	u16				beaconType;
	u16				probeRes;
	u16				beaconLostTh;
	u16				activeZoneTime;
	u8				ssidMask[32];
	u16				preambleType;
	u16				authAlgo;
} WlParamSetAllReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				staMacAdrs[3];
} WlParamSetMacAdrsReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				retryLimit;
} WlParamSetRetryLimitReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				enableChannel;
} WlParamSetEnableChannelReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				mode;
} WlParamSetModeReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				rate;
} WlParamSetRateReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				wepMode;
} WlParamSetWepModeReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
	
	u16				wepKeyId;
} WlParamSetWepKeyIdReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				wepKey[4][10];
} WlParamSetWepKeyReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				beaconType;
} WlParamSetBeaconTypeReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				probeRes;
} WlParamSetProbeResReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				beaconLostTh;
} WlParamSetBeaconLostThReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				activeZoneTime;
} WlParamSetActiveZoneReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u8				mask[32];
} WlParamSetSsidMaskReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				type;
} WlParamSetPreambleTypeReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				type;
} WlParamSetAuthAlgoReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				ccaMode;
	u16				edThreshold;
	u16				agcLimit;
} WlParamSetCCAModeEDThReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				tableNumber;
	u16				camLifeTime;
	u16				frameLifeTime;
} WlParamSetLifeTimeReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				count;
} WlParamSetMaxConnReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				mainAntenna;
} WlParamSetMainAntennaReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				diversity;
	u16				useAntenna;
} WlParamSetDiversityReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				enableMessage;
} WlParamSetBeaconSendRecvIndReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				mode;
} WlParamSetNullKeyModeReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				bssid[3];
} WlParamSetBssidReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				ssidLength;
	u8				ssid[32];
} WlParamSetSsidReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				beaconPeriod;
} WlParamSetBeaconPeriodReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				dtimPeriod;
} WlParamSetDtimPeriodReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				listenInterval;
} WlParamSetIntervalReq;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				gameInfoLength;
	u16				gameInfo[1];
} WlParamSetGameInfoReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlParamSetCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlParamGetReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				staMacAdrs[3];
	u16				retryLimit;
	u16				enableChannel;
	u16				channel;
	u16				mode;
	u16				rate;
	u16				wepMode;
	u16				wepKeyId;
	u16				beaconType;
	u16				probeRes;
	u16				beaconLostTh;
	u16				activeZoneTime;
	u8				ssidMask[32];
	u16				preambleType;
	u16				authAlgo;
} WlParamGetAllCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				staMacAdrs[3];
} WlParamGetMacAdrsCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				retryLimit;
} WlParamGetRetryLimitCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				enableChannel;
	u16				channel;
} WlParamGetEnableChannelCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				mode;
} WlParamGetModeCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				rate;
} WlParamGetRateCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				wepMode;
} WlParamGetWepModeCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
	
	u16				wepKeyId;
} WlParamGetWepKeyIdCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				beaconType;
} WlParamGetBeaconTypeCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				probe;
} WlParamGetProbeResCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				beaconLostTh;
} WlParamGetBeaconLostThCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				activeZoneTime;
} WlParamGetActiveZoneCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u8				mask[32];
} WlParamGetSsidMaskCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				type;
} WlParamGetPreambleTypeCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				type;
} WlParamGetAuthAlgoCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				ccaMode;
	u16				edThreshold;
	u16				agcLimit;
} WlParamGetCCAModeEDThCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				count;
} WlParamGetMaxConnCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				mainAntenna;
} WlParamGetMainAntennaCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				diversity;
	u16				useAntenna;
} WlParamGetDiversityCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				enableMessage;
} WlParamGetBeaconSendRecvIndCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				mode;
} WlParamGetNullKeyModeCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				bssid[3];
} WlParamGetBssidCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				ssidLength;
	u8				ssid[32];
} WlParamGetSsidCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				beaconPeriod;
} WlParamGetBeaconPeriodCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				dtimPeriond;
} WlParamGetDtimPeriodCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				listenInterval;
} WlParamGetIntervalCfm;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				gameInfoLength;
	u16				gameInfo[1];
} WlParamGetGameInfoCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevShutdownReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlDevShutdownCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevIdleReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlDevIdleCfm;


typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevClass1Req;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlDevClass1Cfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevRebootReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlDevRebootCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevClrInfoReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlDevClrInfoCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevGetVerInfoReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u8				wlVersion[8];
	u16				macVersion;
	u16				bbpVersion[2];
	u16				rfVersion;
} WlDevGetVerInfoCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevGetInfoReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				rsv1;
	WlCounter		counter;
} WlDevGetInfoCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
} WlDevGetStateReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;

	u16				state;
} WlDevGetStateCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				control;
	u16				signal;
	u16				rate;
	u16				channel;
} WlDevTestSignalReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlDevTestSignalCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;

	u16				control;
	u16				channel;
} WlDevTestRxReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
} WlDevTestRxCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
	u16				buf[2];
} WlCmdReq;

typedef	struct {
	WlCmdHeader		header;
	u16				resultCode;
	u16				buf[2];
} WlCmdCfm;

typedef	struct {
	u16				wlRsv[WL_RSV];
	WlCmdHeader		header;
	u16				buf[2];
} WlCmdInd;

#define	WL_CMDCODE_MLME_RESET				(WL_CMDGCODE_MLME  | 0x00)
#define	WL_CMDCODE_MLME_PWRMGT				(WL_CMDGCODE_MLME  | 0x01)
#define	WL_CMDCODE_MLME_SCAN				(WL_CMDGCODE_MLME  | 0x02)
#define	WL_CMDCODE_MLME_JOIN				(WL_CMDGCODE_MLME  | 0x03)
#define	WL_CMDCODE_MLME_AUTH				(WL_CMDGCODE_MLME  | 0x04)
#define	WL_CMDCODE_MLME_DEAUTH				(WL_CMDGCODE_MLME  | 0x05)
#define	WL_CMDCODE_MLME_ASS					(WL_CMDGCODE_MLME  | 0x06)
#define	WL_CMDCODE_MLME_REASS				(WL_CMDGCODE_MLME  | 0x07)
#define	WL_CMDCODE_MLME_DISASS				(WL_CMDGCODE_MLME  | 0x08)
#define	WL_CMDCODE_MLME_START				(WL_CMDGCODE_MLME  | 0x09)
#define	WL_CMDCODE_MLME_MEASCH				(WL_CMDGCODE_MLME  | 0x0A)
//#define	WL_CMDCODE_MLME_BCLOST			(WL_CMDGCODE_MLME  | 0x0B)

//#define	WL_CMDCODE_MLME_RESET_IND		(WL_CMDGCODE_MLME  | 0x80)
//#define	WL_CMDCODE_MLME_PWRMGT_IND		(WL_CMDGCODE_MLME  | 0x81)
//#define	WL_CMDCODE_MLME_SCAN_IND		(WL_CMDGCODE_MLME  | 0x82)
//#define	WL_CMDCODE_MLME_JOIN_IND		(WL_CMDGCODE_MLME  | 0x83)
#define	WL_CMDCODE_MLME_AUTH_IND			(WL_CMDGCODE_MLME  | 0x84)
#define	WL_CMDCODE_MLME_DEAUTH_IND			(WL_CMDGCODE_MLME  | 0x85)
#define	WL_CMDCODE_MLME_ASS_IND				(WL_CMDGCODE_MLME  | 0x86)
#define	WL_CMDCODE_MLME_REASS_IND			(WL_CMDGCODE_MLME  | 0x87)
#define	WL_CMDCODE_MLME_DISASS_IND			(WL_CMDGCODE_MLME  | 0x88)
//#define	WL_CMDCODE_MLME_MEASCH_IND		(WL_CMDGCODE_MLME  | 0x89)
#define	WL_CMDCODE_MLME_BCLOST_IND			(WL_CMDGCODE_MLME  | 0x8B)
#define	WL_CMDCODE_MLME_BCSEND_IND			(WL_CMDGCODE_MLME  | 0x8C)
#define	WL_CMDCODE_MLME_BCRECV_IND			(WL_CMDGCODE_MLME  | 0x8D)

#define	WL_CMDCODE_MA_DATA					(WL_CMDGCODE_MA    | 0x00)
#define	WL_CMDCODE_MA_KEY					(WL_CMDGCODE_MA    | 0x01)
#define	WL_CMDCODE_MA_MP					(WL_CMDGCODE_MA    | 0x02)
#define	WL_CMDCODE_MA_TESTDATA				(WL_CMDGCODE_MA    | 0x03)
#define	WL_CMDCODE_MA_CLRDATA				(WL_CMDGCODE_MA    | 0x04)

#define	WL_CMDCODE_MA_DATA_IND				(WL_CMDGCODE_MA    | 0x80)
//#define	WL_CMDCODE_MA_KEY_IND			(WL_CMDGCODE_MA    | 0x81)
#define	WL_CMDCODE_MA_MP_IND				(WL_CMDGCODE_MA    | 0x82)
//#define	WL_CMDCODE_MA_TESTDATA_IND		(WL_CMDGCODE_MA    | 0x83)
#define	WL_CMDCODE_MA_MPEND_IND				(WL_CMDGCODE_MA    | 0x84)
#define	WL_CMDCODE_MA_MPACK_IND				(WL_CMDGCODE_MA    | 0x85)
#define	WL_CMDCODE_MA_FATAL_ERR_IND			(WL_CMDGCODE_MA    | 0x86)

#define	WL_CMDCODE_PARAM_SET_ALL			(WL_CMDGCODE_PARAM | 0x00)
#define	WL_CMDCODE_PARAM_SET_MAC_ADRS		(WL_CMDGCODE_PARAM | 0x01)
#define	WL_CMDCODE_PARAM_SET_RETRY_LIMIT	(WL_CMDGCODE_PARAM | 0x02)
#define	WL_CMDCODE_PARAM_SET_ENABLECHANNEL	(WL_CMDGCODE_PARAM | 0x03)
#define	WL_CMDCODE_PARAM_SET_MODE			(WL_CMDGCODE_PARAM | 0x04)
#define	WL_CMDCODE_PARAM_SET_RATE			(WL_CMDGCODE_PARAM | 0x05)
#define	WL_CMDCODE_PARAM_SET_WEP_MODE		(WL_CMDGCODE_PARAM | 0x06)
#define	WL_CMDCODE_PARAM_SET_WEP_KEYID		(WL_CMDGCODE_PARAM | 0x07)
#define	WL_CMDCODE_PARAM_SET_WEP_KEY		(WL_CMDGCODE_PARAM | 0x08)
#define	WL_CMDCODE_PARAM_SET_BEACON_TYPE	(WL_CMDGCODE_PARAM | 0x09)
#define	WL_CMDCODE_PARAM_SET_PROBE_RES		(WL_CMDGCODE_PARAM | 0x0A)
#define	WL_CMDCODE_PARAM_SET_BEACON_LOST	(WL_CMDGCODE_PARAM | 0x0B)
#define	WL_CMDCODE_PARAM_SET_ACTIVE_ZONE	(WL_CMDGCODE_PARAM | 0x0C)
#define	WL_CMDCODE_PARAM_SET_SSID_MASK		(WL_CMDGCODE_PARAM | 0x0D)
#define	WL_CMDCODE_PARAM_SET_PREAMBLE_TYPE	(WL_CMDGCODE_PARAM | 0x0E)
#define	WL_CMDCODE_PARAM_SET_AUTHALGO		(WL_CMDGCODE_PARAM | 0x0F)
#define	WL_CMDCODE_PARAM_SET_CCAMODE		(WL_CMDGCODE_PARAM | 0x10)
#define	WL_CMDCODE_PARAM_SET_LIFETIME		(WL_CMDGCODE_PARAM | 0x11)
#define	WL_CMDCODE_PARAM_SET_MAXCONN		(WL_CMDGCODE_PARAM | 0x12)
#define	WL_CMDCODE_PARAM_SET_MAIN_ANTENNA	(WL_CMDGCODE_PARAM | 0x13)
#define	WL_CMDCODE_PARAM_SET_DIVERSITY		(WL_CMDGCODE_PARAM | 0x14)
#define	WL_CMDCODE_PARAM_SET_BCNTXRX_IND	(WL_CMDGCODE_PARAM | 0x15)
#define	WL_CMDCODE_PARAM_SET_NULLKEYMODE	(WL_CMDGCODE_PARAM | 0x16)

#define	WL_CMDCODE_PARAM_SET_BSSID			(WL_CMDGCODE_PARAM | 0x40)
#define	WL_CMDCODE_PARAM_SET_SSID			(WL_CMDGCODE_PARAM | 0x41)
#define	WL_CMDCODE_PARAM_SET_BEACON_PERIOD	(WL_CMDGCODE_PARAM | 0x42)
#define	WL_CMDCODE_PARAM_SET_DTIM_PERIOD	(WL_CMDGCODE_PARAM | 0x43)
#define	WL_CMDCODE_PARAM_SET_LISTEN_INT		(WL_CMDGCODE_PARAM | 0x44)
#define	WL_CMDCODE_PARAM_SET_GAME_INFO		(WL_CMDGCODE_PARAM | 0x45)

#define	WL_CMDCODE_PARAM_GET_ALL			(WL_CMDGCODE_PARAM | 0x80)
#define	WL_CMDCODE_PARAM_GET_MAC_ADRS		(WL_CMDGCODE_PARAM | 0x81)
#define	WL_CMDCODE_PARAM_GET_RETRY_LIMIT	(WL_CMDGCODE_PARAM | 0x82)
#define	WL_CMDCODE_PARAM_GET_ENABLECHANNEL	(WL_CMDGCODE_PARAM | 0x83)
#define	WL_CMDCODE_PARAM_GET_MODE			(WL_CMDGCODE_PARAM | 0x84)
#define	WL_CMDCODE_PARAM_GET_RATE			(WL_CMDGCODE_PARAM | 0x85)
#define	WL_CMDCODE_PARAM_GET_WEP_MODE		(WL_CMDGCODE_PARAM | 0x86)
#define	WL_CMDCODE_PARAM_GET_WEP_KEYID		(WL_CMDGCODE_PARAM | 0x87)
#define	WL_CMDCODE_PARAM_GET_WEP_KEY		(WL_CMDGCODE_PARAM | 0x88)
#define	WL_CMDCODE_PARAM_GET_BEACON_TYPE	(WL_CMDGCODE_PARAM | 0x89)
#define	WL_CMDCODE_PARAM_GET_PROBE_RES		(WL_CMDGCODE_PARAM | 0x8A)
#define	WL_CMDCODE_PARAM_GET_BEACON_LOST	(WL_CMDGCODE_PARAM | 0x8B)
#define	WL_CMDCODE_PARAM_GET_ACTIVE_ZONE	(WL_CMDGCODE_PARAM | 0x8C)
#define	WL_CMDCODE_PARAM_GET_SSID_MASK		(WL_CMDGCODE_PARAM | 0x8D)
#define	WL_CMDCODE_PARAM_GET_PREAMBLE_TYPE	(WL_CMDGCODE_PARAM | 0x8E)
#define	WL_CMDCODE_PARAM_GET_AUTHALGO		(WL_CMDGCODE_PARAM | 0x8F)
#define	WL_CMDCODE_PARAM_GET_CCAMODE		(WL_CMDGCODE_PARAM | 0x90)
//#define	WL_CMDCODE_PARAM_GET_LIFETIME	(WL_CMDGCODE_PARAM | 0x91)
#define	WL_CMDCODE_PARAM_GET_MAXCONN		(WL_CMDGCODE_PARAM | 0x92)
#define	WL_CMDCODE_PARAM_GET_MAIN_ANTENNA	(WL_CMDGCODE_PARAM | 0x93)
#define	WL_CMDCODE_PARAM_GET_DIVERSITY		(WL_CMDGCODE_PARAM | 0x94)
#define	WL_CMDCODE_PARAM_GET_BCNTXRX_IND	(WL_CMDGCODE_PARAM | 0x95)
#define	WL_CMDCODE_PARAM_GET_NULLKEYMODE	(WL_CMDGCODE_PARAM | 0x96)

#define	WL_CMDCODE_PARAM_GET_BSSID			(WL_CMDGCODE_PARAM | 0xC0)
#define	WL_CMDCODE_PARAM_GET_SSID			(WL_CMDGCODE_PARAM | 0xC1)
#define	WL_CMDCODE_PARAM_GET_BEACON_PERIOD	(WL_CMDGCODE_PARAM | 0xC2)
#define	WL_CMDCODE_PARAM_GET_DTIM_PERIOD	(WL_CMDGCODE_PARAM | 0xC3)
#define	WL_CMDCODE_PARAM_GET_LISTEN_INT		(WL_CMDGCODE_PARAM | 0xC4)
#define	WL_CMDCODE_PARAM_GET_GAME_INFO		(WL_CMDGCODE_PARAM | 0xC5)


#define	WL_CMDCODE_DEV_SHUTDOWN				(WL_CMDGCODE_DEV   | 0x01)
#define	WL_CMDCODE_DEV_IDLE					(WL_CMDGCODE_DEV   | 0x02)
#define	WL_CMDCODE_DEV_CLASS1				(WL_CMDGCODE_DEV   | 0x03)
#define	WL_CMDCODE_DEV_REBOOT				(WL_CMDGCODE_DEV   | 0x04)
#define	WL_CMDCODE_DEV_INIT_INFO			(WL_CMDGCODE_DEV   | 0x05)
#define	WL_CMDCODE_DEV_GET_VERINFO			(WL_CMDGCODE_DEV   | 0x06)
#define	WL_CMDCODE_DEV_GET_INFO				(WL_CMDGCODE_DEV   | 0x07)
#define	WL_CMDCODE_DEV_GET_STATE			(WL_CMDGCODE_DEV   | 0x08)
#define	WL_CMDCODE_DEV_TEST_SIGNAL			(WL_CMDGCODE_DEV   | 0x09)
#define	WL_CMDCODE_DEV_TEST_RX				(WL_CMDGCODE_DEV   | 0x0A)


#define	WL_CMDGCODE_MLME					0x0000
#define	WL_CMDGCODE_MA						0x0100
#define	WL_CMDGCODE_PARAM					0x0200
#define	WL_CMDGCODE_DEV						0x0300


#define	WL_CMDSCODE_MASK					0x00FF
#define	WL_CMDGCODE_MASK					0xFF00

#define	WL_CMDRES_SUCCESS					0x00
#define	WL_CMDRES_STATE_WRONG				0x01
#define	WL_CMDRES_REQUEST_BUSY				0x02
#define	WL_CMDRES_NOT_SUPPORT				0x03
#define	WL_CMDRES_LENGTH_ERR				0x04
#define	WL_CMDRES_INVALID_PARAM				0x05
#define	WL_CMDRES_REFUSE					0x06
#define	WL_CMDRES_TIMEOUT					0x07
#define	WL_CMDRES_NOT_ENOUGH_MEM			0x08
#define	WL_CMDRES_NOT_ENOUGH_PARAM			0x09
#define	WL_CMDRES_NOT_CLASS3_STA_FRAME		0x0A
#define	WL_CMDRES_ILLEGAL_MODE				0x0B
#define	WL_CMDRES_FAILURE					0x0C
#define	WL_CMDRES_CONFIRM_CODE_ERR			0x0D
#define	WL_CMDRES_FLASH_ERR					0x0E

#define	WL_CMDRES_ERR_MASK			0x7F


#endif	// __WLCMD_H_
