#ifndef __WLNIC_H_
#define __WLNIC_H_

#include "CAM.h"

#define MAX_SSID_LENGTH        32
#define MAX_RETRY_LIMIT        255
#define MASK_ENABLE_CHANNEL    0x7FFE
#define MAX_MODE_NUM           3
#define MAX_WEPMODE_NUM        3
#define MAX_WEP_KEYID_NUM      3
#define MAX_WEPKEY_LENGTH      20
#define MAX_BEACONTYPE_NUM     1
#define MAX_BCSSIDRESPONSE_NUM 1
#define MAX_BCNLOSTTH_NUM      255
#define MIN_BEACON_PERIOD      10
#define MAX_BEACON_PERIOD      1000
#define MAX_DTIM_PERIOD        255
#define MIN_DTIM_PERIOD        1
#define MAX_SCAN_TYPE          1
#define MAX_CHANNEL_TIME       1000
#define MIN_CHANNEL_TIME       10
#define MAX_AUTHTYPE           1
#define MAX_DEFAULT_MIB        1
#define MAX_PWRMGT_MODE        1
#define MAX_PWRMGT_WAVEUP      1
#define MAX_PWRMGT_RXDTIMS     1
#define MAX_LISTEN_INTERVAL    255
#define MIN_LISTEN_INTERVAL    1
#define MAX_PREAMBLE_TYPE      1
#define MAX_AUTHALGO           1
#define MAX_TXPOWER            1
#define MAX_MEASURE_MODE       1
#define MAX_CCA_MODE           3
#define MAX_ED_THRESHOLD       63
#define MAX_AGC_LIMIT          63
#define MAX_MEASURE_TIME       1000
#define MAX_FRM_LIFETIME       63
#define MAX_ANTENNA            1
#define MAX_DIVERSITY_MODE     1
#define MAX_BCN_TXRX_IND_MODE  1
#define MAX_NULLKEY_MODE       1
#define MIN_ACTIVE_ZONE        10
#define MAX_BSSIDMASK_NUM      16

#define MAX_DATA_LENGTH     1508
#define MAX_WMHEADER_LENGTH 2
#define MAX_KEYDATA_LENGTH  516

#define MAX_JOIN_TIMEOUT  2000
#define MIN_AUTH_TIMEOUT  10
#define MAX_AUTH_TIMEOUT  2000
#define MIN_ASS_TIMEOUT   10
#define MAX_ASS_TIMEOUT   2000
#define MIN_REASS_TIMEOUT 10
#define MAX_REASS_TIMEOUT 2000

#define MAX_APLIST_NUM       4
#define MAX_APLIST_LIFETIME  1024
#define CHALLENGE_TXT_LENGTH 128
#define MAX_GAMEINFO_LENGTH  128

#define WINTERVAL_TIMER 100
// #define	WINTERVAL_TIMER			50

#define MIN_BSSDESC_LENGTH (sizeof(WlBssDesc) - 4)

#define MASK_BRS 0x0FFF
#define MASK_SRS 0x0FFF

#define MAX_TEST_SIGNAL_CONTROL 1
#define MAX_TEST_SIGNAL_CHANNEL 14
#define MAX_TEST_SIGNAL         4

#define MBUF_HEADER     (8 + 12 + 24)
#define MBUF_WEP_HEADER (MBUF_HEADER + 8 + 12)

#define MBUF_BEACON    (MBUF_HEADER + 208 + 0)
#define MBUF_BROADCAST (MBUF_WEP_HEADER + 1508 + 0)
#define MBUF_MANCTRL   (MBUF_WEP_HEADER + 261 + 3)
#define MBUF_DATA      (MBUF_WEP_HEADER + 1508 + 0)
#define MBUF_PSPOLL    (MBUF_HEADER - 6 + 2)
#define MBUF_MP        (MBUF_HEADER + 2 + 2 + 2 + MAX_KEYDATA_LENGTH + 2)
#define MBUF_KEY       (MBUF_HEADER + 2 + MAX_KEYDATA_LENGTH + 2)

#define MBUF_TEST_PRI2 0x0000
#define MBUF_TEST_PRI1 (MBUF_TEST_PRI2 + MBUF_PSPOLL)
#define MBUF_TEST_PRI0 (MBUF_TEST_PRI1 + MBUF_MANCTRL)
#define MBUF_TEST_RX   (MBUF_TEST_PRI0 + MBUF_DATA)

#define MBUF_PARENT_MP     0x0000
#define MBUF_PARENT_BEACON (MBUF_PARENT_MP + MBUF_MP)
#define MBUF_PARENT_PRI2   (MBUF_PARENT_BEACON + MBUF_BEACON)
#define MBUF_PARENT_PRI1   (MBUF_PARENT_PRI2 + MBUF_BROADCAST)
#define MBUF_PARENT_PRI0   (MBUF_PARENT_PRI1 + MBUF_MANCTRL)
#define MBUF_PARENT_RX     (MBUF_PARENT_PRI0 + MBUF_DATA)

#define MBUF_CHILD_CDATA0 0x0000
#define MBUF_CHILD_CDATA1 (MBUF_CHILD_CDATA0 + MBUF_KEY)
#define MBUF_CHILD_PRI2   (MBUF_CHILD_CDATA1 + MBUF_KEY)
#define MBUF_CHILD_PRI1   (MBUF_CHILD_PRI2 + MBUF_PSPOLL)
#define MBUF_CHILD_PRI0   (MBUF_CHILD_PRI1 + MBUF_MANCTRL)
#define MBUF_CHILD_RX     (MBUF_CHILD_PRI0 + MBUF_DATA)

#define MBUF_HOTSPOT_PRI2 0x0000
#define MBUF_HOTSPOT_PRI1 (MBUF_HOTSPOT_PRI2 + MBUF_PSPOLL)
#define MBUF_HOTSPOT_PRI0 (MBUF_HOTSPOT_PRI1 + MBUF_MANCTRL)
#define MBUF_HOTSPOT_RX   (MBUF_HOTSPOT_PRI0 + MBUF_DATA)

#define MBUF_MAC_WORK   0x1F60
#define MBUF_WEPKEY_0   0x1F80
#define MBUF_WEPKEY_1   0x1FA0
#define MBUF_WEPKEY_2   0x1FC0
#define MBUF_WEPKEY_3   0x1FE0
#define MBUF_MAC_RX_END MBUF_MAC_WORK

#define STA_SHUTDOWN   0x00
#define STA_IDLE       0x10
#define STA_IDLE_TEST  0x11
#define STA_IDLE_TEST2 0x12
#define STA_CLASS1     0x20
#define STA_CLASS2     0x30
#define STA_CLASS3     0x40

#define STA_MAIN_MASK 0xF0

#define WL_CMDLABEL_MODE_MEASCHAN (MAX_MODE_NUM + 1)

#define B_RATE_1   0x01
#define B_RATE_2   0x02
#define RF_RATE_1M 0x0a
#define RF_RATE_2M 0x14

#define PWRMODE_PS  0x01
#define PWRMODE_ACT 0x00

#define PWRSTS_PS  PWR_STS_SLEEP
#define PWRSTS_ACT PWR_STS_ACTIVE

#define FC_MP       (FC_DATA_CFPOLL | B_FC_FROMDS)
#define FC_MPKEY    (FC_DATA_CFACK | B_FC_TODS)
#define FC_MPNULL   (FC_CFACK | B_FC_TODS)
#define FC_MPACK    (FC_DATA_CFACK | B_FC_FROMDS)
#define MP_ADRS0    0x0903
#define MP_ADRS1    0x00BF
#define MP_ADRS2    0x0000
#define MPKEY_ADRS0 0x0903
#define MPKEY_ADRS1 0x00BF
#define MPKEY_ADRS2 0x1000
#define MPACK_ADRS0 0x0903
#define MPACK_ADRS1 0x00BF
#define MPACK_ADRS2 0x0300

#define MACBUG_MP_DELT_DUR       6
#define MACBUG_MP_DELT_TXOP_BYTE 1
#define MACBUG_MP_DELT_TXOP_US   2

#define QID_DATA      0
#define QID_MANCTRL   1
#define QID_PSPOLL    2
#define QID_BROADCAST 2

#define MREG_TXQ_DATA      MREG_TXQ0
#define MREG_TXQ_MANCTRL   MREG_TXQ1
#define MREG_TXQ_PSPOLL    MREG_TXQ2
#define MREG_TXQ_BROADCAST MREG_TXQ2

#define CONTINUOUS_DATA_MODE 0xFFFF
#define FID_DEAUTH_IND       0x0001
#define FID_DEAUTH_IND2      0x0002

// #define	MatchMacAdrs(_x_,_y_)		((_x_[2] == _y_[2]) && (_x_[1]
// == _y_[1]) && (_x_[0] == _y_[0]))
inline u32 LoadHigh(u16 *p)
{
    return *p >> 8;
}
inline u32 LoadLow(u16 *p)
{
    return *p & 0x00FF;
}
inline void StoreHigh(u16 *p, u16 data)
{
    *p = (*p & 0x00FF) | (data << 8);
}
inline void StoreLow(u16 *p, u16 data)
{
    *p = (*p & 0xFF00) | (data & 0x00FF);
}

typedef struct {
    u16 Basic;
    u16 Support;
} RATE_SET, *LPRATE_SET;

typedef struct {
    LPCAM_ELEMENT pCAM;
    u16 CamMaxStaNum;
    u16 MaxStaNum;

    u16 MacAdrs[3];
    u16 RetryLimit;
    u16 EnableChannel;
    u16 Mode;
    u16 Rate;
    u16 AuthAlgo;
    u16 WepMode;
    u16 WepKeyId;
    u16 FrameLifeTimePerBeacon;

    u16 BeaconType : 1;
    u16 BcSsidResponse : 1;
    u16 PreambleType : 1;
    u16 MainAntenna : 1;
    u16 Diversity : 1;
    u16 UseAntenna : 1;
    u16 BcnTxRxIndMsg : 1;
    u16 NullKeyRes : 1;
    u16 dmm : 8;

    u16 ActiveZone;
    u16 DiagResult;
    u32 ParamFlag;
} CONFIG_PARAM, *LPCONFIG_PARAM;

typedef struct {
    u64 NextTbttTsf;

    u16 STA;
    u16 RSSI;
    u16 Mode;

    u16 PowerMgtMode;
    u16 PowerState;
    u16 bSynchro;
    u16 RxDtims;
    u16 PN15Rate;
    u16 SigTest2;
    u16 bFirstTbtt;
    //	u16			xxx;

    u16 ManCtrlRetry;
    u16 SSIDLength;
    u8 SSID[MAX_SSID_LENGTH];
    u8 SSIDMask[MAX_SSID_LENGTH];

    RATE_SET RateSet;

    u16 BSSID[3];
    u16 AID;

    u16 SeqNum;

    u16 BeaconPeriod;
    u16 ListenInterval;
    u16 CurrListenInterval;
    u16 DTIMPeriod;
    u16 DTIMCount;

    u16 rsvcc;
    u16 CurrChannel;
    u16 CapaInfo;

    u16 BeaconLostTh;
    u16 BeaconLostCnt;

    u16 LinkAdrs[3];
    u16 APCamAdrs;

    u16 FrameCtrl;
    u16 FrameLifeTime;

    u16 bExistTIM;
    u16 TxPower;

    struct {
        struct {
            u16 SSID;
            u16 TIM;
            u16 GameInfo;
        } Beacon;

        struct {
            u16 SSID;
        } ProbeReq;

        struct {
            u16 Size;
        } RxBuf;
    } Ofst;

    void *GameInfoAdrs;
    u16 GameInfoLength;
    u16 GameInfoAlign;
    u16 bUpdateGameInfo;
    u16 TmpttPs;

    u32 IntervalCount;

    u16 Scrambler;
    u16 DbgSeqNum;

    u16 FatalErr;
    u16 rsv;

    u16 CurrErrCount;
    u16 TxBufErrCount;
    u16 TxBufResCount;
    u16 WepErrCount;
    u16 MpEndErrCount;
    u16 NotPollTxErrCount;

} WORK_PARAM, *LPWORK_PARAM;

typedef struct {
    u16 a;
    u16 b;
    u16 seed;
    u16 rsv;
} RAND_CTRL, *LPRAND_CTRL;

typedef struct {
    u16 Id;
    u16 Bits;
    u16 InitNum;
    u16 ChanNum;
    u16 BbpCnt;
    u16 pad;
    u32 BkReg;
    //	u8		TxPwr[14];
} RF_CONFIG, *LPRF_CONFIG;

void InitializeParam(LPCAM_ELEMENT pCam, u32 staNum);

void WStart(void);
void WStop(void);
#define DbgTerminateMac(_bFlag_)
#define TERMINATE_MAC_MACSTOP  1
#define TERMINATE_MAC_SNAPSHOT 2

void InitializeAlarm(void);
void WWait(u16 ms);
void WWaitus(u32 us);
void SetupPeriodicTimeOut(u32 ms, void (*pFunc)(void *));
void ClearPeriodicTimeOut(void);
void SetupTimeOut(u32 ms, void (*pFunc)(void *));
void SetupUsTimeOut(u32 us, void (*pFunc)(void *));
void ClearTimeOut(void);

u16 WSetMacAdrs(u16 *pMacAdrs);
u16 WSetRetryLimit(u16 retry);
u16 WSetEnableChannel(u16 enableChannel);
u16 WSetMode(u16 mode);
u16 WSetRate(u16 rate);
u16 WSetWepMode(u16 mode);
u16 WSetWepKeyId(u16 keyId);
u16 WSetWepKey(u16 *pKey);
u16 WSetBeaconType(u16 type);
u16 WSetBcSsidResponse(u16 response);
u16 WSetBeaconLostThreshold(u16 threshold);
u16 WSetActiveZoneTime(u16 time, u32 update);
u16 WSetSsidMask(u16 *pMask);
u16 WSetPreambleType(u16 type);
u16 WSetAuthAlgo(u16 type);
u16 WSetCCA_ED(u32 ccaMode, u32 edThreshold);
u16 WSetLifeTime(u32 camAdrs, u32 lifeTime);
u16 WSetMainAntenna(u32 mainAntenna);
u16 WSetDiversity(u32 diversity, u32 useAntenna);
u16 WSetBeaconSendRecvIndicate(u32 mode);
u16 WSetNullKeyMode(u32 mode);

u16 WSetBssid(u16 *pBssid);
u16 WSetSsid(u16 length, u8 *pSsid);
u16 WSetBeaconPeriod(u16 period);
u16 WSetDTIMPeriod(u16 period);
u16 WSetListenInterval(u16 interval);

void WSetDefaultParameters(void);

u16 WSetChannel(u16 channel, u32 bDirect);
u16 WSetRateSet(LPRATE_SET pRateSet);
void WSetTxTimeStampOffset(void);
u16 WSetPowerMgtMode(u32 mode);
u16 WSetPowerState(u32 state);
void WShutdown(void);
void WWakeUp(void);
u16 WSetFrameLifeTime(u32 lifeTimePerBeacon);

void WDisableTmpttPowerSave(void);
void WEnableTmpttPowerSave(void);
u16 WInitGameInfo(u32 length, u8 *pGameInfo);
u16 WSetGameInfo(u32 length, u8 *pGameInfo);
void WAlignGameInfo(void);
void WSetStaState(u32 state);

void WSetAids(u16 aid);
void WClearAids(void);
void WSetKSID(void);
void WClearKSID(void *arg);

u16 WSetForcePowerState(u32 state);

void WSetMacAdrs1(u16 *dst, u16 *src1);
void WSetMacAdrs2(u16 *dst, u16 *src1, u16 *src2);
void WSetMacAdrs3(u16 *dst, u16 *src1, u16 *src2, u16 *src3);

void WInitCounter(void);
void WUpdateCounter(void);
void WUpdateMpCounter(u16 map);

u32 WCheckSSID(u16 len, u8 *pSSID);
u32 MatchMacAdrs(u16 *pAdrs1, u16 *pAdrs2);
u32 CheckEnableChannel(u32 ch);

void WElement2RateSet(LPSUP_RATE_ELEMENT pSup, LPRATE_SET pRateSet);
u32 WCalcManRate(void);

void WConfigDevice(void);
void InitMac(void);
void InitBaseBand(void);
void InitRF(void);

u32 BBP_Write(u32 adrs, u32 data);
u32 BBP_Read(u32 adrs);
u32 RF_Read(u32 data);
void RF_Write(u32 data);
u32 EEPROM_Read(u32 adrs);
u8 EEPROM_ReadU8(u32 adrs);
u16 CalcBbpCRC(void);
u32 CheckPllLock(void);

void DMA_Read(void *destAdrs, void *srcAdrs, u32 length);
void DMA_WriteCore(void *destAdrs, void *srcAdrs, u32 count);
void DMA_Write(void *destAdrs, void *srcAdrs, u32 length);

void DMA_WriteHeaderData(LPTXFRM_MAC destAdrs, LPMAC_HEADER header, u8 *data, u32 length);
void DMA_WepWriteHeaderData(LPTXFRM_MAC destAdrs, LPMAC_HEADER header, u8 *data, u32 length);

#ifdef SDK_SMALL_BUILD_WL
#define WLLIB_DmaCopy32(srcAdrs, destAdrs, length) \
    MI_CpuCopy32((void *)srcAdrs, (void *)destAdrs, length)
#define WLLIB_DmaCopy16(srcAdrs, destAdrs, length) \
    MI_CpuCopy16((void *)srcAdrs, (void *)destAdrs, length)
#define WLLIB_DmaClear32(destAdrs, length) \
    MI_CpuClear32((void *)destAdrs, length)
#define WLLIB_DmaClear16(destAdrs, length) \
    MI_CpuClear16((void *)destAdrs, length)
#else
#ifdef SDK_CPU_COPY_WL
#define WLLIB_DmaCopy32(srcAdrs, destAdrs, length) \
    MI_CpuCopy32((void *)srcAdrs, (void *)destAdrs, length)
#define WLLIB_DmaCopy16(srcAdrs, destAdrs, length) \
    MI_CpuCopy16((void *)srcAdrs, (void *)destAdrs, length)
#define WLLIB_DmaClear32(destAdrs, length) \
    MI_CpuClear32((void *)destAdrs, length)
#define WLLIB_DmaClear16(destAdrs, length) \
    MI_CpuClear16((void *)destAdrs, length)
#else
void WLLIB_DmaCopy32(u32 src, u32 dst, u32 length);
void WLLIB_DmaCopy16(u32 src, u32 dst, u32 length);
void WLLIB_DmaClear32(u32 dst, u32 length);
void WLLIB_DmaClear16(u32 dst, u32 length);
#endif
#endif

void WL_WriteByte(void *p, u8 data);
u8 WL_ReadByte(void *p);

void RND_init(u32 a, u32 b);
void RND_seed(u32 seed);
u16 RND_rand(void);

u16 calc_CRC(u8 *data, u16 len);
u16 calc_NextCRC(u8 data, u16 total);

void WCheckTxBuf(void);

void SetFatalErr(u32 errCode);
void SendFatalErrMsgTask(void);

void TerminateWlTask(void);
void ReleaseWlTask(void);

#define SetMacTxAdrs(_x_) (((u32)(_x_) & 0x3FFF) / 2)
#define GetMacTxAdrs(_x_) (MAC_MEM_BASE + (((u32)(_x_) & 0x0FFF) * 2))
#define GetMacRxAdrs(_x_) (MAC_MEM_BASE + ((u32)(_x_) * 2))

#define WSetAntenna(_x_) \
    *(vu16 *)MREG_ANTENNA = (u16)(wlMan->Config.UseAntenna ^ wlMan->Config.MainAntenna)
#define WChangeAntenna(_x_) *(vu16 *)MREG_ANTENNA ^= 1

#define BitSet(_adrs_, _mask_, _value_) \
    _adrs_ = (u16)((_adrs_ & (u16)~_mask_) | ((u16)_value_))

#define CalcReqAdrsFromFrame(_x_) \
    (void *)((u32)_x_ - (sizeof(WlMaDataReq) - sizeof(WlTxFrame)))

extern const u16 RateBit2Element[];
extern const u16 RateBit2RFRate[];
extern const u16 NULL_ADRS[3];
extern const u16 BC_ADRS[3];
extern const u16 MPKEY_ADRS[3];
extern const u16 MP_ADRS[3];

#ifdef __WLNIC_C_

static u32 WCheckTxBufIdBeforeFrame(LPTXQ pTxq);
static void WaitMacStop(void);
static void RestoreTxFrame(LPTXQ pTxq);

#if (USE_FLASH == 0)
u32 WLi_EEPROM_Read(u32 adrs);
#endif

void WIntervalTimer(void *arg);

#define WLDEF_MAC_ADRS0   0xa000
#define WLDEF_MAC_ADRS1   0x0096
#define WLDEF_MAC_ADRS2   0x1000
#define WLDEF_RETRY_LIMIT 7
#define WLDEF_ENCH        WL_CMDLABEL_ENCH_MKK
#define WLDEF_MODE        WL_CMDLABEL_MODE_CHILD
#define WLDEF_RATE        WL_CMDLABEL_RATE_AUTO
#define WLDEF_WEP_MODE    WL_CMDLABEL_WEP_NO
#define WLDEF_WEP_KEYID   0
#define WLDEF_WEP_KEY \
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }
#define WLDEF_BEACON_PERIOD   500
#define WLDEF_BCN_TYPE        WL_CMDLABEL_INCLUDE_SSID
#define WLDEF_BCSSID_RESPONSE WL_CMDLABEL_RESPONSE_BC_PRBREQ
#define WLDEF_BCN_LOST_TH     16
#define WLDEF_ACTIVE_ZONE     0xFFFF
#define WLDEF_SSID_MASK \
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }
#define WLDEF_PREAMBLE_TYPE    WL_CMDLABEL_PREAMBLE_SHORT
#define WLDEF_AUTHALGO         WL_CMDLABEL_AUTH_OPEN_SYSTEM
#define WLDEF_CHANNEL          10
#define WLDEF_TXPOWER          WL_CMDLABEL_TXPOWER_10DBM
#define WLDEF_RATESET_BASIC    0x03
#define WLDEF_RATESET_SUPPORT  0x03
#define WLDEF_MANCTRL_RETRY    2
#define WLDEF_CCA_MODE         0
#define WLDEF_ED_THRESHOLD     0x1F
#define WLDEF_FRAME_LIFETIME   5
#define WLDEF_MAIN_ANTENNA     WL_CMDLABEL_MAIN_ANTENNA_1
#define WLDEF_DIVERSITY        WL_CMDLABEL_DIVERSITY_OFF
#define WLDEF_USEANTENNA       WL_CMDLABEL_USE_MAIN_ANTENNA
#define WLDEF_BCN_TXRX_IND     WL_CMDLABEL_BCNIND_OFF
#define WLDEF_NULLKEY_RESPONSE WL_CMDLABEL_NULLKEY_DISABLE

#define AccConfigBBP(_adrsBitCnt_, _dataBitCnt_) \
    *(vu16 *)MREG_BBP_CFG = (u16)(_adrsBitCnt_ | _dataBitCnt_)
#define AccConfigRF(_adrsBitCnt_, _dataBitCnt_) \
    *(vu16 *)MREG_RF_CFG = (u16)(_adrsBitCnt_ | _dataBitCnt_)

typedef struct {
    u16 adrs;
    u16 value;
} MAC_INIT_REGS;

#endif // __WLNIC_C_
#endif // __WLNIC_H_
