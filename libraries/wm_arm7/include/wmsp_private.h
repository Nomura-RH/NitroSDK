#ifndef LIBRARIES_WM_ARM7_WMSP_PRIVATE_H__
#define LIBRARIES_WM_ARM7_WMSP_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nitro/types.h>
#include <nitro/os.h>
#include <nitro/wm.h>
#include "wm_private.h"

#include "wmsp_mac.h"

#define WM_THREAD_VARIABLE_ON_WRAM
#define WM_VALARM_VARIABLE_ON_WRAM
#define WM_ALARM_VARIABLE_ON_WRAM

#define WM_CLEAN_SEND_QUEUE

#define WMSP_UPDATE_LINK_LEVEL_PER_FRAME
#define WMSP_LINK_LEVEL_TREAT_ABSENCE_AS_ERROR

#define WMSP_MP_DETECT_FATAL_ERROR_STRICTLY



#define WMSP_TO_WL_MSGQ_COUNT    2
#define WMSP_FROM_WL_MSGQ_COUNT  4
#define WMSP_REQUEST_MSGQ_COUNT  32
#define WMSP_INDICATE_STACK_SIZE 1024
#define WMSP_REQUEST_STACK_SIZE  4096

#define WMSP_ENABLE_CHANNEL_MASK   0x1fff
#define WMSP_ENABLE_CHANNEL_PERIOD 5
#define WMSP_ALLOWED_CHANNEL_MASK  0x1041

#define WMSP_RSSI_HISTORY_MAX 32
#define WMSP_RSSI_COUNT       1
#define WMSP_RSSI_ERROR       4
#define WMSP_MP_PING_COUNT    60

#define WMSP_RSSI_NITRO_LINK_LEVEL_1 8
#define WMSP_RSSI_NITRO_LINK_LEVEL_2 14
#define WMSP_RSSI_NITRO_LINK_LEVEL_3 20

#define WMSP_RSSI_REVOLUTION_LINK_LEVEL_1 22
#define WMSP_RSSI_REVOLUTION_LINK_LEVEL_2 28
#define WMSP_RSSI_REVOLUTION_LINK_LEVEL_3 34

#define WMSP_LED_BLINK_ENABLE  PM_LED_PATTERN_WIRELESS
#define WMSP_LED_BLINK_DISABLE PM_LED_PATTERN_ON

#define WMSP_DEFAULT_CAM_LIFE_TIME   40
#define WMSP_DEFAULT_FRAME_LIFE_TIME 5
#define WMSP_DEFAULT_MP_LIFE_TIME    40

#define WMSP_DEAUTH_RETRY_MAX 2

#ifdef WM_DEBUG_INDICATES
#define WM_DPRINTF_INDICATES WMi_Printf
#else
#define WM_DPRINTF_INDICATES(...) ((void)0)
#endif
#ifdef WM_DEBUG_PORT_SENDRECV
#define WM_DPRINTF_PORT_SENDRECV WMi_Printf
#else
#define WM_DPRINTF_PORT_SENDRECV(...) ((void)0)
#endif
#ifdef WM_DEBUG_PORT_SEND_QUEUE
#define WM_DPRINTF_PORT_SEND_QUEUE WMi_Printf
#else
#define WM_DPRINTF_PORT_SEND_QUEUE(...) ((void)0)
#endif
#ifdef WM_DEBUG_LINKLEVEL
#define WM_DPRINTF_LINKLEVEL WMi_Printf
#else
#define WM_DPRINTF_LINKLEVEL(...) ((void)0)
#endif
#ifdef WM_DEBUG_VALARM
#define WM_DPRINTF_VALARM WMi_Printf
#else
#define WM_DPRINTF_VALARM(...) ((void)0)
#endif
#ifdef WM_DEBUG_HEAP
#define WM_DPRINTF_HEAP WMi_Printf
#else
#define WM_DPRINTF_HEAP(...) ((void)0)
#endif
#ifdef WM_DEBUG_REQUEST_QUEUE
#define WM_DPRINTF_REQUEST_QUEUE WMi_Printf
#else
#define WM_DPRINTF_REQUEST_QUEUE(...) ((void)0)
#endif
#ifdef WM_DEBUG_INDICATE_QUEUE
#define WM_DPRINTF_INDICATE_QUEUE WMi_Printf
#else
#define WM_DPRINTF_INDICATE_QUEUE(...) ((void)0)
#endif

#define WMSPi_LOG(cat, msg)                                                                                                    \
    do {                                                                                                                       \
        s32 _SDK_WM_vcount = GX_GetVCount();                                                                                   \
        (void)WMi_Printf("%02x:%03d$ " cat msg "\n", ((OS_GetVBlankCount() + (_SDK_WM_vcount < 192)) & 0xff), _SDK_WM_vcount); \
    } while (FALSE)

#define WMSPi_LOGF(cat, msg, ...)                                                                                                           \
    do {                                                                                                                                    \
        s32 _SDK_WM_vcount = GX_GetVCount();                                                                                                \
        (void)WMi_Printf("%02x:%03d$ " cat msg "\n", ((OS_GetVBlankCount() + (_SDK_WM_vcount < 192)) & 0xff), _SDK_WM_vcount, __VA_ARGS__); \
    } while (FALSE)

#define WMSP_LOG(msg)       WMSPi_LOG("", msg)
#define WMSP_LOGF(msg, ...) WMSPi_LOGF("", msg, __VA_ARGS__)

#ifdef WM_DEBUG
#define WMSP_DLOG  WMSP_LOG
#define WMSP_DLOGF WMSP_LOGF
#else
#define WMSP_DLOG(...)  ((void)0)
#define WMSP_DLOGF(...) ((void)0)
#endif

#ifdef WM_DEBUG_INDICATES
#define WMSP_DLOG_INDICATES  WMSP_LOG
#define WMSP_DLOGF_INDICATES WMSP_LOGF
#else
#define WMSP_DLOG_INDICATES(...)  ((void)0)
#define WMSP_DLOGF_INDICATES(...) ((void)0)
#endif

#ifdef WM_DEBUG_PORT_SENDRECV
#define WMSP_DLOG_PORT_SENDRECV  WMSP_LOG
#define WMSP_DLOGF_PORT_SENDRECV WMSP_LOGF
#else
#define WMSP_DLOG_PORT_SENDRECV(...)  ((void)0)
#define WMSP_DLOGF_PORT_SENDRECV(...) ((void)0)
#endif

#ifdef WM_DEBUG_PORT_SEND_QUEUE
#define WMSP_DLOG_PORT_SEND_QUEUE  WMSP_LOG
#define WMSP_DLOGF_PORT_SEND_QUEUE WMSP_LOGF
#else
#define WMSP_DLOG_PORT_SEND_QUEUE(...)  ((void)0)
#define WMSP_DLOGF_PORT_SEND_QUEUE(...) ((void)0)
#endif

#ifdef WM_DEBUG_LINKLEVEL
#define WMSP_DLOG_LINKLEVEL  WMSP_LOG
#define WMSP_DLOGF_LINKLEVEL WMSP_LOGF
#else
#define WMSP_DLOG_LINKLEVEL(...)  ((void)0)
#define WMSP_DLOGF_LINKLEVEL(...) ((void)0)
#endif

#ifdef WM_DEBUG_VALARM
#define WMSP_DLOG_VALARM  WMSP_LOG
#define WMSP_DLOGF_VALARM WMSP_LOGF
#else
#define WMSP_DLOG_VALARM(...)  ((void)0)
#define WMSP_DLOGF_VALARM(...) ((void)0)
#endif

#ifdef WM_DEBUG_HEAP
#define WMSP_DLOG_HEAP  WMSP_LOG
#define WMSP_DLOGF_HEAP WMSP_LOGF
#else
#define WMSP_DLOG_HEAP(...)  ((void)0)
#define WMSP_DLOGF_HEAP(...) ((void)0)
#endif

#ifdef WM_DEBUG_REQUEST_QUEUE
#define WMSP_DLOG_REQUEST_QUEUE  WMSP_LOG
#define WMSP_DLOGF_REQUEST_QUEUE WMSP_LOGF
#else
#define WMSP_DLOG_REQUEST_QUEUE(...)  ((void)0)
#define WMSP_DLOGF_REQUEST_QUEUE(...) ((void)0)
#endif

#ifdef WM_DEBUG_INDICATE_QUEUE
#define WMSP_DLOG_INDICATE_QUEUE  WMSP_LOG
#define WMSP_DLOGF_INDICATE_QUEUE WMSP_LOGF
#else
#define WMSP_DLOG_INDICATE_QUEUE(...)  ((void)0)
#define WMSP_DLOGF_INDICATE_QUEUE(...) ((void)0)
#endif

typedef struct WMSPWork {
    OSMessageQueue toWLmsgQ;
    OSMessage toWLmsg[WMSP_TO_WL_MSGQ_COUNT];
    OSMessageQueue fromWLmsgQ;
    OSMessage fromWLmsg[WMSP_FROM_WL_MSGQ_COUNT];
    OSMessageQueue confirmQ;
    OSMessage confirm[WMSP_FROM_WL_MSGQ_COUNT];
    OSMessageQueue requestQ;
    OSMessage request[WMSP_REQUEST_MSGQ_COUNT];

#ifndef WM_THREAD_VARIABLE_ON_WRAM
    OSThread requestThread;
#endif
    u32 requestStack[WMSP_REQUEST_STACK_SIZE / sizeof(u32)];
#ifndef WM_THREAD_VARIABLE_ON_WRAM
    OSThread indicateThread;
#endif

    u32 indicateStack[WMSP_INDICATE_STACK_SIZE / sizeof(u32)];

    OSMutex fifoExclusive;

    u32 dmaNo;
    OSArenaId arenaId;
    OSHeapHandle heapHandle;

    WMArm7Buf *wm7buf;
    WMStatus *status;

    u8 rssiHistory[WMSP_RSSI_HISTORY_MAX];
    u32 rssiIndex;

    u32 indPrio_high;
    u32 wlPrio_high;
    u32 reqPrio_high;
    u32 indPrio_low;
    u32 wlPrio_low;
    u32 reqPrio_low;

#ifndef WM_VALARM_VARIABLE_ON_WRAM
    OSVAlarm VAlarm;
#endif

#ifndef WM_ALARM_VARIABLE_ON_WRAM
    OSAlarm mp_intervalAlarm;
    OSAlarm mp_ackAlarm;
#endif
} WMSPWork;

extern WMSPWork wmspW;

typedef void (*WMSPRequestFunc)(OSMessage msg);

typedef WlMlmePowerMgtReq WlMlmePowerManagementReq;
typedef WlMlmePowerMgtCfm WlMlmePowerManagementCfm;
typedef WlMlmeAuthReq WlMlmeAuthenticateReq;
typedef WlMlmeAuthCfm WlMlmeAuthenticateCfm;
typedef WlMlmeDeAuthReq WlMlmeDeAuthenticateReq;
typedef WlMlmeDeAuthCfm WlMlmeDeAuthenticateCfm;
typedef WlMlmeAssReq WlMlmeAssociateReq;
typedef WlMlmeAssCfm WlMlmeAssociateCfm;
typedef WlMlmeReAssReq WlMlmeReAssociateReq;
typedef WlMlmeReAssCfm WlMlmeReAssociateCfm;
typedef WlMlmeDisAssReq WlMlmeDisAssociateReq;
typedef WlMlmeDisAssCfm WlMlmeDisAssociateCfm;
typedef WlMlmeMeasChanReq WlMlmeMeasureChannelReq;
typedef WlMlmeMeasChanCfm WlMlmeMeasureChannelCfm;
typedef WlMlmeAuthInd WlMlmeAuthenticateInd;
typedef WlMlmeDeAuthInd WlMlmeDeAuthenticateInd;
typedef WlMlmeAssInd WlMlmeAssociateInd;
typedef WlMlmeReAssInd WlMlmeReAssociateInd;
typedef WlMlmeDisAssInd WlMlmeDisAssociateInd;
typedef WlMaClrDataReq WlMaClearDataReq;
typedef WlMaClrDataCfm WlMaClearDataCfm;
typedef WlParamSetMacAdrsReq WlParamSetMacAddressReq;
typedef WlParamSetProbeResReq WlParamSetProbeResponseReq;
typedef WlParamSetBeaconLostThReq WlParamSetBeaconLostThresholdReq;
typedef WlParamSetAuthAlgoReq WlParamSetAuthenticationAlgorithmReq;
typedef WlParamSetCCAModeEDThReq WlParamSetCCAModeEDThresholdReq;
typedef WlParamSetMaxConnReq WlParamSetMaxConnectableChildReq;
typedef WlParamGetMacAdrsCfm WlParamGetMacAddressCfm;
typedef WlParamGetProbeResCfm WlParamGetProbeResponseCfm;
typedef WlParamGetBeaconLostThCfm WlParamGetBeaconLostThresholdCfm;
typedef WlParamGetAuthAlgoCfm WlParamGetAuthenticationAlgorithmCfm;
typedef WlParamGetCCAModeEDThCfm WlParamGetCCAModeEDThresholdCfm;
typedef WlParamGetMaxConnCfm WlParamGetMaxConnectableChildCfm;
typedef WlDevRebootReq WlDevRestartReq;
typedef WlDevRebootCfm WlDevRestartCfm;
typedef WlDevClrInfoReq WlDevSetInitializeWirelessCounterReq;
typedef WlDevClrInfoCfm WlDevSetInitializeWirelessCounterCfm;
typedef WlDevGetVerInfoReq WlDevGetVersionReq;
typedef WlDevGetVerInfoCfm WlDevGetVersionCfm;
typedef WlDevGetInfoReq WlDevGetWirelessCounterReq;
typedef WlDevGetInfoCfm WlDevGetWirelessCounterCfm;
typedef WlDevGetStateReq WlDevGetStationStateReq;
typedef WlDevGetStateCfm WlDevGetStationStateCfm;

void WMSP_ReturnResult2Wm9(void *ptr);
void WMSP_Wait4Wm9(void);
void *WMSP_GetBuffer4Callback2Wm9(void);
u16 *WMSP_WlRequest(u16 *request);
BOOL WMSP_CheckMacAddress(const u8 *macAdr);
void WMSP_CopyParentParam(WMGameInfo *gameInfop, WMParentParam *pparamp);
BOOL WMSP_SetAllParams(u16 wmApiID, u16 *buf);
WMMpRecvData *WMSP_GetKeyDataAdr(WMMpRecvHeader *header, u16 aid);
void WMSP_SendMaMP(u16 pollBitmap);
void WMSP_ResumeMaMP(u16 pollBitmap);
void WMSP_SendMaKeyData(void);
void WMSP_InitSendQueue(void);
int WMSP_PutSendQueue(u32 childBitmap, u16 priority, u16 port, u32 destBitmap, const u16 *sendData, u16 sendDataSize, WMCallbackFunc callback, void *arg);
BOOL WMSP_FlushSendQueue(BOOL timeout, u16 pollBitmap);
void WMSP_CleanSendQueue(u16 aidBitmap);
void WMSP_ParsePortPacket(u16 aid, u16 wmHeader, u16 *data, u16 length, WMMpRecvBuf *recvBuf);
u16 WMSP_GetAllowedChannel(u16 bitField);
void WMSP_AddRssiToList(u8 rssi8);
void WMSP_FillRssiIntoList(u8 rssi8);
u16 WMSP_GetAverageLinkLevel(void);
u16 WMSP_GetLinkLevel(u32 rssi);
BOOL WMSP_DisconnectCore(u32 *args, BOOL indicateFlag, u16 *disconnected);
void WMSP_IndicateDisconnectionFromMyself(BOOL parent, u16 aid, void *mac);
void WMSP_SetThreadPriorityLow(void);
void WMSP_SetThreadPriorityHigh(void);
OSThread *WMSP_GetRequestThread(void);
OSThread *WMSP_GetIndicateThread(void);
WMErrCode WMSP_SetMPParameterCore(const WMMPParam *param, WMMPParam *old_param);
void WMSP_InitAlarm(void);
void WMSP_CancelAllAlarms(void);
void WMSP_ResetSizeVars(void);
void WMSP_SetParentMaxSize(u16 parentMaxSize);
void WMSP_SetChildMaxSize(u16 childMaxSize);
void WMSP_SetParentSize(u16 parentMaxSize);
void WMSP_SetChildSize(u16 childMaxSize);
void WMSP_RequestResumeMP(void);

void WMSP_CheckSendQueue(char *name);
void WMSP_CheckWLHeap(char *name);

void WMSP_IndicateThread(void *arg);
void WMSP_RequestThread(void *arg);

u32 *WMSP_GetInternalRequestBuf(void);

void WMSP_Initialize(OSMessage msg);
void WMSP_Reset(OSMessage msg);
void WMSP_End(OSMessage msg);
void WMSP_Enable(OSMessage msg);
void WMSP_Disable(OSMessage msg);
void WMSP_PowerOn(OSMessage msg);
void WMSP_PowerOff(OSMessage msg);
void WMSP_SetParentParam(OSMessage msg);
void WMSP_StartParent(OSMessage msg);
void WMSP_EndParent(OSMessage msg);
void WMSP_StartScan(OSMessage msg);
void WMSP_EndScan(OSMessage msg);
void WMSP_StartConnectEx(OSMessage msg);
void WMSP_Disconnect(OSMessage msg);
void WMSP_StartMP(OSMessage msg);
void WMSP_SetMPData(OSMessage msg);
void WMSP_EndMP(OSMessage msg);
void WMSP_StartDCF(OSMessage msg);
void WMSP_SetDCFData(OSMessage msg);
void WMSP_EndDCF(OSMessage msg);
void WMSP_SetWEPKey(OSMessage msg);
void WMSP_SetGameInfo(OSMessage msg);
void WMSP_SetBeaconTxRxInd(OSMessage msg);
void WMSP_StartTestMode(OSMessage msg);
void WMSP_StopTestMode(OSMessage msg);
void WMSP_VAlarmSetMPData(OSMessage msg);
void WMSP_SetLifeTime(OSMessage msg);
void WMSP_MeasureChannel(OSMessage msg);
void WMSP_InitWirelessCounter(OSMessage msg);
void WMSP_GetWirelessCounter(OSMessage msg);
void WMSP_SetEntry(OSMessage msg);
void WMSP_AutoDeAuth(OSMessage msg);
void WMSP_SetMPParameter(OSMessage msg);
void WMSP_AutoDisconnect(OSMessage msg);
void WMSP_KickNextMP_Parent(OSMessage msg);
void WMSP_KickNextMP_Child(OSMessage msg);
void WMSP_KickNextMP_Resume(OSMessage msg);
void WMSP_SetBeaconPeriod(OSMessage msg);
void WMSP_StartScanEx(OSMessage msg);
void WMSP_SetWEPKeyEx(OSMessage msg);
void WMSP_SetPowerSaveMode(OSMessage msg);
void WMSP_StartTestRxMode(OSMessage msg);
void WMSP_StopTestRxMode(OSMessage msg);


void WMSP_InitVAlarm(void);
void WMSP_SetVAlarm(void);
void WMSP_CancelVAlarm(void);


WlMlmeResetCfm *WMSP_WL_MlmeReset(u16 *buf, u16 mib);
WlMlmePowerManagementCfm *WMSP_WL_MlmePowerManagement(u16 *buf, u16 pwrMgtMode, u16 wakeUp, u16 recieveDtims);
WlMlmeScanCfm *WMSP_WL_MlmeScan(u16 *buf, u32 bufSize, u16 *bssid, u16 ssidLength, u8 *ssid, u16 scanType, u8 *channelList, u16 maxChannelTime);
WlMlmeJoinCfm *WMSP_WL_MlmeJoin(u16 *buf, u16 timeOut, WlBssDesc *bssDesc);
WlMlmeAuthenticateCfm *WMSP_WL_MlmeAuthenticate(u16 *buf, u16 *peerMacAdrs, u16 algorithm, u16 timeOut);
WlMlmeDeAuthenticateCfm *WMSP_WL_MlmeDeAuthenticate(u16 *buf, u16 *peerMacAdrs, u16 reasonCode);
WlMlmeAssociateCfm *WMSP_WL_MlmeAssociate(u16 *buf, u16 *peerMacAdrs, u16 listenInterval, u16 timeOut);
WlMlmeReAssociateCfm *WMSP_WL_MlmeReAssociate(u16 *buf, u16 *newApMacAdrs, u16 listenInterval, u16 timeOut);
WlMlmeDisAssociateCfm *WMSP_WL_MlmeDisAssociate(u16 *buf, u16 *peerMacAdrs, u16 reasonCode);
WlMlmeStartCfm *WMSP_WL_MlmeStart(u16 *buf, u16 ssidLength, u8 *ssid, u16 beaconPeriod, u16 dtimPeriod, u16 channel, u16 basicRateSet, u16 supportRateSet, u16 gameInfoLength, WMGameInfo *gameInfo);
WlMlmeMeasureChannelCfm *WMSP_WL_MlmeMeasureChannel(u16 *buf, u16 ccaMode, u16 edThreshold, u16 measureTime, u8 *channelList);
WlMaDataCfm *WMSP_WL_MaData(u16 *buf, WlTxFrame *frame);
WlMaKeyDataCfm *WMSP_WL_MaKeyData(u16 *buf, u16 length, u16 wmHeader, u16 *keyDatap);
WlMaMpCfm *WMSP_WL_MaMp(u16 *buf, u16 resume, u16 retryLimit, u16 txop, u16 pollBitmap, u16 tmptt, u16 currTsf, u16 dataLength, u16 wmHeader, u16 *datap);
WlMaTestDataCfm *WMSP_WL_MaTestData(u16 *buf, WlTestFrame *frame);
#if SDK_VERSION_WL >= 19600
WlMaClearDataCfm *WMSP_WL_MaClearData(u16 *buf, u16 flag);
#endif
WlParamSetCfm *WMSP_WL_ParamSetAll(WlParamSetAllReq *req);
WlParamSetCfm *WMSP_WL_ParamSetMacAddress(u16 *buf, u16 *staMacAdrs);
WlParamSetCfm *WMSP_WL_ParamSetRetryLimit(u16 *buf, u16 retryLimit);
#if SDK_VERSION_WL >= 15600
WlParamSetCfm *WMSP_WL_ParamSetEnableChannel(u16 *buf, u16 enableChannel);
#endif
WlParamSetCfm *WMSP_WL_ParamSetMode(u16 *buf, u16 mode);
WlParamSetCfm *WMSP_WL_ParamSetRate(u16 *buf, u16 rate);
WlParamSetCfm *WMSP_WL_ParamSetWepMode(u16 *buf, u16 wepMode);
WlParamSetCfm *WMSP_WL_ParamSetWepKeyId(u16 *buf, u16 wepKeyId);
WlParamSetCfm *WMSP_WL_ParamSetWepKey(u16 *buf, u8 *wepKey);
WlParamSetCfm *WMSP_WL_ParamSetBeaconType(u16 *buf, u16 beaconType);
WlParamSetCfm *WMSP_WL_ParamSetProbeResponse(u16 *buf, u16 probeRes);
WlParamSetCfm *WMSP_WL_ParamSetBeaconLostThreshold(u16 *buf, u16 beaconLostTh);
WlParamSetCfm *WMSP_WL_ParamSetActiveZone(u16 *buf, u16 activeZoneTime);
WlParamSetCfm *WMSP_WL_ParamSetSsidMask(u16 *buf, u8 *mask);
WlParamSetCfm *WMSP_WL_ParamSetPreambleType(u16 *buf, u16 type);
WlParamSetCfm *WMSP_WL_ParamSetAuthenticationAlgorithm(u16 *buf, u16 type);
#if SDK_VERSION_WL >= 24900
WlParamSetCfm *WMSP_WL_ParamSetCCAModeEDThreshold(u16 *buf, u16 ccaMode, u16 edThreshold, u16 agcLimit);
#else
WlParamSetCfm *WMSP_WL_ParamSetCCAModeEDThreshold(u16 *buf, u16 ccaMode, u16 edThreshold);
#endif
WlParamSetCfm *WMSP_WL_ParamSetLifeTime(u16 *buf, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime);
WlParamSetCfm *WMSP_WL_ParamSetMaxConnectableChild(u16 *buf, u16 count);
WlParamSetCfm *WMSP_WL_ParamSetMainAntenna(u16 *buf, u16 mainAntenna);
WlParamSetCfm *WMSP_WL_ParamSetDiversity(u16 *buf, u16 diversity, u16 useAntenna);
WlParamSetCfm *WMSP_WL_ParamSetBeaconSendRecvInd(u16 *buf, u16 enableMessage);
#if SDK_VERSION_WL >= 25700
WlParamSetCfm *WMSP_WL_ParamSetNullKeyResponseMode(u16 *buf, u16 mode);
#endif
WlParamSetCfm *WMSP_WL_ParamSetBssid(u16 *buf, u16 *bssid);
WlParamSetCfm *WMSP_WL_ParamSetSsid(u16 *buf, u16 ssidLength, u8 *ssid);
WlParamSetCfm *WMSP_WL_ParamSetBeaconPeriod(u16 *buf, u16 beaconPeriod);
WlParamSetCfm *WMSP_WL_ParamSetDtimPeriod(u16 *buf, u16 dtimPeriod);
WlParamSetCfm *WMSP_WL_ParamSetInterval(u16 *buf, u16 listenInterval);
WlParamSetCfm *WMSP_WL_ParamSetGameInfo(u16 *buf, u16 gameInfoLength, u16 *gameInfo);
WlParamGetAllCfm *WMSP_WL_ParamGetAll(u16 *buf);
WlParamGetMacAddressCfm *WMSP_WL_ParamGetMacAddress(u16 *buf);
WlParamGetRetryLimitCfm *WMSP_WL_ParamGetRetryLimit(u16 *buf);
#if SDK_VERSION_WL >= 15600
WlParamGetEnableChannelCfm *WMSP_WL_ParamGetEnableChannel(u16 *buf);
#endif
WlParamGetModeCfm *WMSP_WL_ParamGetMode(u16 *buf);
WlParamGetRateCfm *WMSP_WL_ParamGetRate(u16 *buf);
WlParamGetWepModeCfm *WMSP_WL_ParamGetWepMode(u16 *buf);
WlParamGetWepKeyIdCfm *WMSP_WL_ParamGetWepKeyId(u16 *buf);
WlParamGetBeaconTypeCfm *WMSP_WL_ParamGetBeaconType(u16 *buf);
WlParamGetProbeResponseCfm *WMSP_WL_ParamGetProbeResponse(u16 *buf);
WlParamGetBeaconLostThresholdCfm *WMSP_WL_ParamGetBeaconLostThreshold(u16 *buf);
WlParamGetActiveZoneCfm *WMSP_WL_ParamGetActiveZone(u16 *buf);
WlParamGetSsidMaskCfm *WMSP_WL_ParamGetSsidMask(u16 *buf);
WlParamGetPreambleTypeCfm *WMSP_WL_ParamGetPreambleType(u16 *buf);
WlParamGetAuthenticationAlgorithmCfm *WMSP_WL_ParamGetAuthenticationAlgorithm(u16 *buf);
#if SDK_VERSION_WL >= 24900
WlParamGetCCAModeEDThCfm *WMSP_WL_ParamGetCCAModeEDThreshold(u16 *buf);
#else
WlParamGetCCAModeEDThresholdCfm *WMSP_WL_ParamGetCCAModeEDThreshold(u16 *buf);
#endif
WlParamGetMaxConnectableChildCfm *WMSP_WL_ParamGetMaxConnectableChild(u16 *buf);
WlParamGetMainAntennaCfm *WMSP_WL_ParamGetMainAntenna(u16 *buf);
WlParamGetDiversityCfm *WMSP_WL_ParamGetDiversity(u16 *buf);
WlParamGetBeaconSendRecvIndCfm *WMSP_WL_ParamGetBeaconSendRecvInd(u16 *buf);
#if SDK_VERSION_WL >= 25700
WlParamGetNullKeyModeCfm *WMSP_WL_ParamGetNullKeyModeCfm(u16 *buf);
#endif
WlParamGetBssidCfm *WMSP_WL_ParamGetBssid(u16 *buf);
WlParamGetSsidCfm *WMSP_WL_ParamGetSsid(u16 *buf);
WlParamGetBeaconPeriodCfm *WMSP_WL_ParamGetBeaconPeriod(u16 *buf);
WlParamGetDtimPeriodCfm *WMSP_WL_ParamGetDtimPeriod(u16 *buf);
WlParamGetIntervalCfm *WMSP_WL_ParamGetInterval(u16 *buf);
WlParamGetGameInfoCfm *WMSP_WL_ParamGetGameInfo(u16 *buf);
WlDevShutdownCfm *WMSP_WL_DevShutdown(u16 *buf);
WlDevIdleCfm *WMSP_WL_DevIdle(u16 *buf);
WlDevClass1Cfm *WMSP_WL_DevClass1(u16 *buf);
#if SDK_VERSION_WL < 15600
WlDevIfcCfm *WMSP_WL_DevIfc(u16 *buf);
#else
WlDevRestartCfm *WMSP_WL_DevRestart(u16 *buf);
#endif
WlDevSetInitializeWirelessCounterCfm *WMSP_WL_DevSetInitializeWirelessCounter(u16 *buf);
WlDevGetVersionCfm *WMSP_WL_DevGetVersion(u16 *buf);
WlDevGetWirelessCounterCfm *WMSP_WL_DevGetWirelessCounter(u16 *buf);
WlDevGetStationStateCfm *WMSP_WL_DevGetStationState(u16 *buf);
WlDevTestSignalCfm *WMSP_WL_DevTestSignal(u16 *buf, u16 control, u16 signal, u16 rate, u16 channel);

#if SDK_VERSION_WL >= 26600
WlDevTestRxCfm *WMSP_WL_DevTestRx(u16 *buf, u16 control, u16 channel);
#endif

#if SDK_VERSION_WL >= 19600
#define WMSP_WL_MaClrData WMSP_WL_MaClearData
#endif
#define WMSP_WL_MlmePowerMgt         WMSP_WL_MlmePowerManagement
#define WMSP_WL_MlmeAuth             WMSP_WL_MlmeAuthenticate
#define WMSP_WL_MlmeDeAuth           WMSP_WL_MlmeDeAuthenticate
#define WMSP_WL_MlmeAss              WMSP_WL_MlmeAssociate
#define WMSP_WL_MlmeReAss            WMSP_WL_MlmeReAssociate
#define WMSP_WL_MlmeDisAss           WMSP_WL_MlmeDisAssociate
#define WMSP_WL_MlmeMeasChan         WMSP_WL_MlmeMeasureChannel
#define WMSP_WL_ParamSetMacAdrs      WMSP_WL_ParamSetMacAddress
#define WMSP_WL_ParamSetProbeRes     WMSP_WL_ParamSetProbeResponse
#define WMSP_WL_ParamSetBeaconLostTh WMSP_WL_ParamSetBeaconLostThreshold
#define WMSP_WL_ParamSetAuthAlgo     WMSP_WL_ParamSetAuthenticationAlgorithm
#define WMSP_WL_ParamSetCCAModeEDTh  WMSP_WL_ParamSetCCAModeEDThreshold
#define WMSP_WL_ParamSetMaxConn      WMSP_WL_ParamSetMaxConnectableChild
#define WMSP_WL_ParamGetMacAdrs      WMSP_WL_ParamGetMacAddress
#define WMSP_WL_ParamGetProbeRes     WMSP_WL_ParamGetProbeResponse
#define WMSP_WL_ParamGetBeaconLostTh WMSP_WL_ParamGetBeaconLostThreshold
#define WMSP_WL_ParamGetAuthAlgo     WMSP_WL_ParamGetAuthenticationAlgorithm
#define WMSP_WL_ParamGetCCAModeEDTh  WMSP_WL_ParamGetCCAModeEDThreshold
#define WMSP_WL_ParamGetMaxConn      WMSP_WL_ParamGetMaxConnectableChild
#if SDK_VERSION_WL >= 15600
#define WMSP_WL_DevReboot WMSP_WL_DevRestart
#endif
#define WMSP_WL_DevClrInfo    WMSP_WL_DevSetInitializeWirelessCounter
#define WMSP_WL_DevGetVerInfo WMSP_WL_DevGetVersion
#define WMSP_WL_DevGetInfo    WMSP_WL_DevGetWirelessCounter
#define WMSP_WL_DevGetState   WMSP_WL_DevGetStationState


#define WL_RATE_1MBPS 0xa
#define WL_RATE_2MBPS 0x14

static inline WMSPWork *WMSP_GetSystemWork(void)
{
    return &wmspW;
}

static inline WMStatus *WMSP_GetStatusStructure(void)
{
    return wmspW.status;
}

static inline u8 WMSP_GetRssi8(u8 rssi)
{
    if (rssi & 0x0002) {
        return (u8)(rssi >> 2);
    }
    return (u8)((rssi >> 2) + 25);
}

static inline void WMSP_AddRssiToRandomPool(u8 rssi8)
{
    u32 rssi_pool = (u32)(OS_GetSystemWork()->wm_rssi_pool << 1);
    rssi_pool ^= rssi8;
    rssi_pool ^= rssi_pool >> 16;
    OS_GetSystemWork()->wm_rssi_pool = (u16)rssi_pool;
}

static inline int WMSP_GetMPSendBufferSize(int sendSize)
{
    return (sendSize + 31) & ~0x1f;
}

static inline int WMSP_GetMPOneReceiveBufferSizeParent(int recvSize, int nChildlen)
{
    return (int)((sizeof(WMmpRecvHeader) - sizeof(WMmpRecvData) + ((sizeof(WMmpRecvData) + recvSize - 2 + 2) * nChildlen) + 31) & ~0x1f);
}

static inline int WMSP_GetMPOneReceiveBufferSizeChild(int recvSize)
{
    return (int)((sizeof(WMMpRecvBuf) + recvSize - 4 + 31) & ~0x1f);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBRARIES_WM_ARM7_WMSP_PRIVATE_H__ */

