#ifndef __WMPARAM_H_
#define __WMPARAM_H_

#ifndef __INSYSROM__
#include "WmHotspot.h"
#include "WmParent.h"
#endif

#define WM_STATE_SUBMASK 0x00FF

#define WM_STATE_STOP 0x0000
#define WM_STATE_NONE 0x0000

#define WM_STATE_OPERATING   0xFF00
#define WM_STATE_CLASS1_WAIT 0xFFF1
#define WM_STATE_SINGLE_CMD  0xFFF2

#define WM_STATE_STARTUP      0x0100
#define WM_STATE_IDLE_CMD     0x0101
#define WM_STATE_CLASS1_CMD   0x0103
#define WM_STATE_POWERMGT     0x0200
#define WM_STATE_MEAS_CHAN    0x0300
#define WM_STATE_MEASING_CHAN 0x0301
#define WM_STATE_START        0x0400
#define WM_STATE_SCAN         0x0500
#define WM_STATE_SCAN_ING     0x0501
#define WM_STATE_JOIN         0x0600
#define WM_STATE_JOIN_ING     0x0601
#define WM_STATE_AUTH         0x0700
#define WM_STATE_AUTH_ING     0x0701
#define WM_STATE_DEAUTH       0x0800
#define WM_STATE_DEAUTH_ING   0x0801
#define WM_STATE_ASS          0x0900
#define WM_STATE_ASS_ING      0x0901
#define WM_STATE_REASS        0x0A00
#define WM_STATE_REASS_ING    0x0A01
#define WM_STATE_DISASS       0x0B00
#define WM_STATE_DISASS_ING   0x0B01
#define WM_STATE_RESET        0x0C00
#define WM_STATE_REBOOT       0x0D00
#define WM_STATE_SHUTDOWN     0x0E00
#define WM_STATE_CLRDATA      0x0F00
#define WM_STATE_CLRDATA_ING  0x0F01

#define WM_STATE_SET_PARAM_CMD   0x1000
#define WM_STATE_SET_MAXCONN_CMD 0x1001

#define WM_TST_STATE_IDLE 0x00

#define WM_TST_STATE_SCAN   0x01
#define WM_TST_STATE_JOIN   0x02
#define WM_TST_STATE_AUTH   0x03
#define WM_TST_STATE_DEAUTH 0x04
#define WM_TST_STATE_ASS    0x05
#define WM_TST_STATE_REASS  0x06
#define WM_TST_STATE_DISASS 0x07
#define WM_TST_STATE_MEASCH 0x08

#define WM_TST_STATE_STA_PAR 0x11
#define WM_TST_STATE_STA_CLD 0x12
#define WM_TST_STATE_CON     0x13
#define WM_TST_STATE_DISCON  0x14

#define WM_TST_STATE_RE_SER_CON 0x20

#define WM_TST_STATE_RESET       0xFF
#define WM_TST_INDSTS_IDLE       0x00
#define WM_TST_INDSTS_AUTH_ING   0x01
#define WM_TST_INDSTS_ASS_ING    0x02
#define WM_TST_INDSTS_DEAUTH_ING 0x10
#define WM_TST_INDSTS_DISASS_ING 0x20

#define DEFAULT_SSID       "IRIS_TEST"
#define DEFAULT_SSIDLENGTH (sizeof(DEFAULT_SSID) - 1)
#define DEFAULT_SSIDMASK \
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define DEFAULT_SCANTYPE   WL_CMDLABEL_SCAN_ACTIVE
#define DEFAULT_AUTOALGO   WL_CMDLABEL_AUTH_OPEN_SYSTEM
#define DEFAULT_MEASTYPE   0
#define DEFAULT_MIB        WL_CMDLABEL_RST_MIB_HOLD
#define DEFAULT_PWRMGTMODE WL_CMDLABEL_PMG_CONT_ACT
#define DEFAULT_WAKEUP     WL_CMDLABEL_WAKEUP_NORMAL
#define DEFAULT_RXDTIM     WL_CMDLABEL_OBEY_LSTN_INV
#define DEFAULT_CHLIST     { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0, 0 }
#define DEFAULT_CHANNEL    1
#define DEFAULT_MAXCHTIME  100
#define DEFAULT_SCANTIME   300
#define DEFAULT_JOINTIME   1000
#define DEFAULT_AUTHTIME   300
#define DEFAULT_ASSTIME    300
#define DEFAULT_LSNINTV    1
#define DEFAULT_REASONCODE WL_CMDLABEL_RSN_UNSPECIFIED
#define DEFAULT_MODE       WL_CMDLABEL_MODE_PARENT
#define DEFAULT_RETRYLIMIT 7
#define DEFAULT_ENCH       WL_CMDLABEL_ENCH_MKK
#define DEFAULT_RATE       WL_CMDLABEL_RATE_AUTO
#define DEFAULT_WEPMODE    WL_CMDLABEL_WEP_NO
// #define	//	wmParam.wepKey=;
#define DEFAULT_WEPKEYID         0
#define DEFAULT_BEACONTYPE       WL_CMDLABEL_INCLUDE_SSID
#define DEFAULT_RESPONSEBCPRBREQ WL_CMDLABEL_RESPONSE_BC_PRBREQ
#define DEFAULT_DITMPERIOD       3
#define DEFAULT_BEACONPERIOD     500
#define DEFAULT_ACTIVEZONE       100
#define DEFAULT_PREAMBLETYPE     WL_CMDLABEL_PREAMBLE_SHORT

#define DEFAULT_SRCMACADRS0 0xA000
#define DEFAULT_SRCMACADRS1 0x0096
#define DEFAULT_SRCMACADRS2 0x1000

#ifdef SDK_TEG
#define DEFAULT_DESTMACADRS0 0xA000
#define DEFAULT_DESTMACADRS1 0x0096
#define DEFAULT_DESTMACADRS2 0x1000
#else
#define DEFAULT_DESTMACADRS0 0x0900
#define DEFAULT_DESTMACADRS1 0x08BF
#define DEFAULT_DESTMACADRS2 0x0100
#endif

#define DEFAULT_BSSID0 0xFFFF
#define DEFAULT_BSSID1 0xFFFF
#define DEFAULT_BSSID2 0xFFFF

#define DEFAULT_BRS 0x03
#define DEFAULT_SRS 0x03

#define DEFAULT_CCAMODE     WL_CMDLABEL_CCA_MODE0
#define DEFAULT_EDTHRESHOLD 31

#define DEFAULT_MEASURETIME 100

#ifndef __INSYSROM__
typedef struct {
    u16 state;

    u16 tststate;

    u16 tstindsts;

    u16 forceState;

    APP_HEAPBUF_MAN mlmeIndBuf;
    APP_HEAPBUF_MAN maIndBuf;

    u16 ApMacAdrs[3];

    WmhFuncTbl wmhFuncTbl;
    WmpFuncTbl wmpFuncTbl;
    struct {
        void (*pCodeCfm)(u16 resultCode);
        void (*pCmdCfm)(WlCmdReq *pReq);
    } Func;

    WlCmdReq *reqMsgp;
    WlCmdCfm *cfmMsgp;

    WlCmdReq reqMsg;
    u8 reqMsgBuf[256];
} WmCtrl;
#endif //	__INSYSROM__

typedef struct {
    u16 irisMacAdrs[3];
    u16 bssid[3];
    u16 peerMacAdrs[3];

    u8 ssid[32];
    u8 ssidMask[32];

    u8 ssidLength;
    u8 scanType;
    u8 authAlgo;
    u8 rsv0;

    u8 mib;
    u8 pwrMgtMode;
    u8 wakeup;
    u8 rxDtim;

    u8 channelList[16];
    u16 maxChannelTime;
    u16 joinTimeOut;
    u16 authTimeOut;
    u16 assTimeOut;

    u16 roaming;
    u8 listenInterval;
    u8 reasonCode;

    u8 mode;
    u8 retryLimit;
    u8 rsvx;
    u8 rate;

    u8 wepKey[20 * 4];

    u8 wepMode;
    u8 wepKeyId;
    u8 beaconType;
    u8 responseBcPrbReq;

    u8 dtimPeriod;
    u8 preambleType;
    u8 channel;
    u8 agcLimit;

    u16 beaconPeriod;
    u16 activeZoneTime;

    u16 txop;
    u16 pollBitmap;
    u16 nextVBlank;

    u16 aid;
    u16 measTbd;

    u16 srcMacAdrs[3];
    u16 destMacAdrs[3];

    u16 basicRateSet;
    u16 supportRateSet;

    u8 ccaMode;
    u8 edThreshold;
    u16 measureTime;
} WmParam;

#ifndef __INSYSROM__
extern WmCtrl wm;
extern WmParam wmParam;
extern u16 enableChannel;

extern OSHeapHandle heapHandle;
extern OSMessageQueue sendMsgQueue;
extern OSMessageQueue recvMsgQueue;

extern OSMessage sendMsg[SEND_MSG_NUM];
extern OSMessage recvMsg[RECV_MSG_NUM];
#endif

#ifndef __INSYSROM__
void InitEeprom(void);
void WmInitParams(void);
u32 WmSearchStaList(u16 *pMacAdrs);
void PrintCmdResultCode(u16 code);
void PrintCmdResultStatusCode(u16 code, u16 status);
void PrintReasonCode(u32 code);
void PrintStatusCode(u32 code);
void PrintBssDescription(WlBssDesc *pDesc);
#endif //	__INSYSROM__

#endif // __WMPARAM_H_
