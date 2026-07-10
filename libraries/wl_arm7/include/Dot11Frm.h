#ifndef __DEFFRM_H_
#define __DEFFRM_H_

typedef struct {
    u16 FrameId;
    u16 CamAdrs;
    u16 FrameTime;
    u16 Length;
} FIRM_HEADER, *LPFIRM_HEADER;

typedef union {
    struct {
        u16 Status;
        u16 Status2;
        u16 rsv_RetryCount;
        u16 rsv_AppRate;
        u16 Service_Rate;
        u16 MPDU;
    } Tx;

    struct {
        u16 Status;
        u16 NextBnry;
        u16 TimeStamp;
        u16 Service_Rate;
        u16 MPDU;
        u16 rsv_RSSI;
    } Rx;
} MAC_HEADER, *LPMAC_HEADER;

typedef union {
    u16 Data;
    struct {
        u16 Version : 2;
        u16 Type : 2;
        u16 SubType : 4;
        u16 ToDS : 1;
        u16 FromDS : 1;
        u16 MoreFrag : 1;
        u16 Retry : 1;
        u16 PowerMan : 1;
        u16 MoreData : 1;
        u16 WEP : 1;
        u16 Order : 1;
    } Bit;
} FRAME_CTRL, *LPFRAME_CTRL;

typedef union {
    u16 Data;
    struct {
        u16 FragNum : 4;
        u16 SeqNum : 12;
    } Bit;
} SEQ_CTRL, *LPSEQ_CTRL;

typedef union {
    u16 Data;
    struct {
        u16 ESS : 1;
        u16 IBSS : 1;
        u16 CFPollable : 1;
        u16 CFPollRequest : 1;
        u16 Privacy : 1;
        u16 ShortPreamble : 1;
        u16 rsv6 : 1;
        u16 rsv7 : 1;
        u16 rsv8 : 1;
        u16 rsv9 : 1;
        u16 rsv10 : 1;
        u16 rsv11 : 1;
        u16 rsv12 : 1;
        u16 rsv13 : 1;
        u16 rsv14 : 1;
        u16 rsv15 : 1;
    } Bit;
} CAPA_INFO, *LPCAPA_INFO;

typedef struct {
    u8 ID;
    u8 Length;
    u8 SSID[32];
} SSID_ELEMENT, *LPSSID_ELEMENT;

typedef struct {
    u8 ID;
    u8 Length;
    u8 SupportedRate[2];
} SUP_RATE_ELEMENT, *LPSUP_RATE_ELEMENT;

typedef struct {
    u8 ID;
    u8 Length;
    u8 CurrChannel;
} DS_PARAM_ELEMENT, *LPDS_PARAM_ELEMENT;

typedef struct {
    u8 ID;
    u8 Length;
    u8 CFPCount;
    u8 CFPPeriod;
    union {
        u16 u16;
        u8 u8[2];
    } CFPMaxDuration;
    union {
        u16 u16;
        u8 u8[2];
    } CFPDurRemain;
} CF_PARAM_ELEMENT, *LPCF_PARAM_ELEMENT;

#define CF_PARAM_OFST_CFPCOUNT  (1 + 1)
#define CF_PARAM_OFST_CFPMAXDUR (1 + 1 + 1 + 1)

typedef struct {
    u8 ID;
    u8 Length;
    union {
        u16 u16;
        u8 u8[2];
    } ATIMWindow;
} IBSS_PARAM_ELEMENT, *LPIBSS_PARAM_ELEMENT;

#define IBSS_PARAM_OFST_ATIMWINDOW (1 + 1)

typedef struct {
    u8 ID;
    u8 Length;
    u8 DTIMCount;
    u8 DTIMPeriod;
    u8 BitmapCtrl;
    u8 VitrualBitmap[16 / 8 + 3];
} TIM_ELEMENT, *LPTIM_ELEMENT;

#define TIM_OFST_DTIMCOUNT (1 + 1)
#define TIM_ELEMENT_SIZE(_MaxStaNum_) \
    (1 + 1 + 1 + 1 + 1 + (_MaxStaNum_) / 8 + 3)

typedef struct {
    u8 ID;
    u8 Length;
    u8 Text[253];
} CHALLENGE_ELEMENT, *LPCHALLENGE_ELEMENT;

typedef struct {
    u8 ID;
    u8 Length;
    u8 OUI[3];
    u8 SubType;
    u8 ActZone[2];
    u8 VTSF[2];
    u8 GameInfo[3];
} GAME_INFO_ELEMENT, *LPGAME_INFO_ELEMENT;

#define CIP_ID_NONE    0
#define CIP_ID_RD4_40  1
#define CIP_ID_TKIP    2
#define CIP_ID_WRAP    3
#define CIP_ID_CCM     4
#define CIP_ID_RC4_104 5

#define GAME_INFO_OUI_0 0x00
#define GAME_INFO_OUI_1 0x09
#define GAME_INFO_OUI_2 0xBF

#define GAME_INFO_SUBTYPE 0x00

#define RSN_CAPA_PREAUTH   1
#define RSN_CAPA_PAIRWISE  2
#define RSN_CAPA_REPLAY_L  4
#define RSN_CAPA_REPLAY_H  8
#define RSN_CAPA_CACHE_PMK 1

#define ID_SSID_ELEMENT         0
#define ID_SUPRATE_ELEMENT      1
#define ID_FH_PARAM_ELEMENT     2
#define ID_DS_PARAM_ELEMENT     3
#define ID_CF_PARAM_ELEMENT     4
#define ID_TIM_ELEMENT          5
#define ID_IBSS_PARAM_ELEMENT   6
#define ID_COUNTRY_INFO_ELEMENT 7
#define ID_CHALLENGE_ELEMENT    16
#define ID_GAME_INFO_ELEMENT    221

#define MAN_STS_SUCCESSFUL                 0
#define MAN_STS_UNSPECIFIED                1
#define MAN_STS_NOT_SUPPORT_CAPABILITY     10
#define MAN_STS_REASS_INABILITY            11
#define MAN_STS_OUT_OF_STANDARD            12
#define MAN_STS_NOT_SUPPORT_AUTH_ALGORITHM 13
#define MAN_STS_OUT_OF_AUTH_SEQ_NUM        14
#define MAN_STS_CHALLENGE_FAILURE          15
#define MAN_STS_AUTH_TIMEOUT               16
#define MAN_STS_ASS_UNABLE_HANDLE          17
#define MAN_STS_INVALID_BASICRATESET       18
#define MAN_STS_NO_ENTRY                   19

#define MAN_RSN_RESERVED                   0
#define MAN_RSN_UNSPECIFIED                1
#define MAN_RSN_PREV_AUTH_INVALID          2
#define MAN_RSN_DEAUTH_LEAVING             3
#define MAN_RSN_INACTIVE                   4
#define MAN_RSN_UNABLE_HANDLE              5
#define MAN_RSN_RX_CLASS2_FROM_NONAUTH_STA 6
#define MAN_RSN_RX_CLASS3_FROM_NONASS_STA  7
#define MAN_RSN_DISASS_LEAVING             8
#define MAN_RSN_ASS_STA_NOTAUTHED          9
#define MAN_RSN_NO_ENTRY                   19

typedef struct {
    FRAME_CTRL FrameCtrl;
    u16 Duration;
    u16 DA[3];
    u16 SA[3];
    u16 BSSID[3];
    SEQ_CTRL SeqCtrl;
} MAN_HEADER, *LPMAN_HEADER;

#define MAN_ALL_HEADER_SIZE \
    (sizeof(FIRM_HEADER) + sizeof(MAC_HEADER) + sizeof(MAN_HEADER))

typedef struct {
    u32 TimeStamp[2];
    u16 BeaconInterval;
    CAPA_INFO CapaInfo;
    u8 Buf[sizeof(SSID_ELEMENT) + sizeof(SUP_RATE_ELEMENT) + sizeof(DS_PARAM_ELEMENT) + sizeof(GAME_INFO_ELEMENT) + 0];
    //						sizeof(TIM_ELEMENT)];
    u8 pad[2];
} BEACON_BODY, *LPBEACON_BODY;
#define SSID_ACTZONE 2
#define SSID_VTSF    4

typedef struct {
    u16 ReasonCode;
} DISASS_BODY, *LPDISASS_BODY;

typedef struct {
    CAPA_INFO CapaInfo;
    u16 ListenInterval;
    u8 Buf[sizeof(SSID_ELEMENT) + sizeof(SUP_RATE_ELEMENT)];
} ASSREQ_BODY, *LPASSREQ_BODY;

typedef struct {
    CAPA_INFO CapaInfo;
    u16 StatusCode;
    u16 AID;
    u8 Buf[sizeof(SUP_RATE_ELEMENT)];
} ASSRES_BODY, *LPASSRES_BODY;

typedef struct {
    CAPA_INFO CapaInfo;
    u16 ListenInterval;
    u16 CurrAPMacAdrs[3];
    u8 Buf[sizeof(SSID_ELEMENT) + sizeof(SUP_RATE_ELEMENT)];
} REASSREQ_BODY, *LPREASSREQ_BODY;

typedef struct {
    CAPA_INFO CapaInfo;
    u16 StatusCode;
    u16 AID;
    u8 Buf[sizeof(SUP_RATE_ELEMENT)];
} REASSRES_BODY, *LPREASSRES_BODY;

typedef struct {
    u8 Buf[sizeof(SSID_ELEMENT) + sizeof(SUP_RATE_ELEMENT)];
} PRBREQ_BODY, *LPPRBREQ_BODY;

typedef struct {
    u32 TimeStamp[2];
    u16 BeaconInterval;
    CAPA_INFO CapaInfo;
    u8 Buf[sizeof(SSID_ELEMENT) + sizeof(SUP_RATE_ELEMENT) + sizeof(DS_PARAM_ELEMENT) + sizeof(GAME_INFO_ELEMENT)];
    u8 pad[2];
} PRBRES_BODY, *LPPRBRES_BODY;

typedef struct {
    u16 AlgoType;
    u16 SeqNum;
    u16 StatusCode;
    CHALLENGE_ELEMENT ChallengeText;
    u8 pad[1];
} AUTH_BODY, *LPAUTH_BODY;

typedef struct {
    u16 ReasonCode;
} DEAUTH_BODY, *LPDEAUTH_BODY;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    BEACON_BODY Body;
} BEACON_FRAME, *LPBEACON_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
} ATIM_FRAME, *LPATIM_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    DISASS_BODY Body;
} DISASS_FRAME, *LPDISASS_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    ASSRES_BODY Body;
} ASSRES_FRAME, *LPASSRES_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    ASSREQ_BODY Body;
} ASSREQ_FRAME, *LPASSREQ_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    REASSREQ_BODY Body;
} REASSREQ_FRAME, *LPREASSREQ_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    REASSRES_BODY Body;
} REASSRES_FRAME, *LPREASSRES_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    PRBRES_BODY Body;
} PRBRES_FRAME, *LPPRBRES_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    PRBREQ_BODY Body;
} PRBREQ_FRAME, *LPPRBREQ_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    AUTH_BODY Body;
} AUTH_FRAME, *LPAUTH_FRAME;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    MAN_HEADER Dot11Header;
    DEAUTH_BODY Body;
} DEAUTH_FRAME, *LPDEAUTH_FRAME;

#define BEACON_FRAME_SIZE \
    (sizeof(BEACON_FRAME) + TIM_ELEMENT_SIZE(wlMan->Config.MaxStaNum))
#define ATIM_FRAME_SIZE     (sizeof(ATIM_FRAME) + 4)
#define DISASS_FRAME_SIZE   (sizeof(DISASS_FRAME) + 4)
#define ASSRES_FRAME_SIZE   (sizeof(ASSRES_FRAME) + 4)
#define ASSREQ_FRAME_SIZE   (sizeof(ASSREQ_FRAME) + 4)
#define REASSREQ_FRAME_SIZE (sizeof(REASSREQ_FRAME) + 4)
#define REASSRES_FRAME_SIZE (sizeof(REASSRES_FRAME) + 4)
#define PRBRES_FRAME_SIZE   (sizeof(PRBRES_FRAME) + 4)
#define PRBREQ_FRAME_SIZE   (sizeof(PRBREQ_FRAME) + 4)
#define AUTH_FRAME_SIZE     (sizeof(AUTH_FRAME) - sizeof(CHALLENGE_ELEMENT) + 4)
#define DEAUTH_FRAME_SIZE   (sizeof(DEAUTH_FRAME) + 4)

/*
typedef struct {
        FRAME_CTRL			FrameCtrl;
        u16				DurationID;
        u16				RA[3];
        u16				BSSID[3];
} CFENDACK_FRAME, *LPCFENDACK_FRAME;
*/

typedef struct {
    FRAME_CTRL FrameCtrl;
    u16 DurationID;
    u16 RA[3];
    u16 TA[3];
} RTS_FRAME, *LPRTS_FRAME;

typedef struct {
    FRAME_CTRL FrameCtrl;
    u16 DurationID;
    u16 RA[3];
} CTS_FRAME, *LPCTS_FRAME;

typedef struct {
    FRAME_CTRL FrameCtrl;
    u16 DurationID;
    u16 RA[3];
} ACK_FRAME, *LPACK_FRAME;

typedef struct {
    FRAME_CTRL FrameCtrl;
    u16 DurationID;
    u16 BSSID[3];
    u16 TA[3];
} PSPOLL_HEADER, *LPPSPOLL_HEADER;

// typedef struct {
// } PSPOLL_BODY, *LPPSPOLL_BODY;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    PSPOLL_HEADER Dot11Header;
    //	PSPOLL_HEADER		Header;
} PSPOLL_FRAME, *LPPSPOLL_FRAME;

typedef struct {
    FRAME_CTRL FrameCtrl;
    u16 DurationID;
    u16 RA[3];
    u16 BSSID[3];
} CFEND_HEADER, *LPCFEND_HEADER;

// typedef struct {
// } CFEND_BODY, *LPCFEND_BODY;

typedef struct {
    CFEND_HEADER Header;
} CFEND_FRAME, *LPCFEND_FRAME;

#define RTS_FRAME_SIZE    sizeof(RTS_FRAME)
#define CTS_FRAME_SIZE    sizeof(CTS_FRAME)
#define ACK_FRAME_SIZE    sizeof(ACK_FRAME)
#define PSPOLL_FRAME_SIZE sizeof(PSPOLL_FRAME)
#define CFEND_FRAME_SIZE  sizeof(CFEND_FRAME)

typedef struct {
    FRAME_CTRL FrameCtrl;
    u16 DurationID;
    u16 Adrs1[3];
    u16 Adrs2[3];
    u16 Adrs3[3];
    SEQ_CTRL SeqCtrl;
} DATA_HEADER, *LPDATA_HEADER;

typedef struct {
    //	BB_HEADER		BB;
    FRAME_CTRL FrameCtrl;
    u16 DurationID;
    u16 Adrs1[3];
    u16 Adrs2[3];
    u16 Adrs3[3];
    SEQ_CTRL SeqCtrl;
    u16 Adrs4[3];
    u16 Dummy;
    u8 FrameBody[1514];
} DOT11_FRAME, *LPDOT11_FRAME;

#define DOT11_OFST_FC        (sizeof(BB_HEADER))
#define DOT11_OFST_DURID     (sizeof(FRAME_CTRL) + DOT11_OFST_FC)
#define DOT11_OFST_ADRS1     (sizeof(u16) + DOT11_OFST_DURID)
#define DOT11_OFST_ADRS2     (sizeof(u16) * 3 + DOT11_OFST_ADRS1)
#define DOT11_OFST_ADRS3     (sizeof(u16) * 3 + DOT11_OFST_ADRS2)
#define DOT11_OFST_SEQ_CTRL  (sizeof(u16) * 3 + DOT11_OFST_ADRS3)
#define DOT11_OFST_ADRS4     (sizeof(SEQ_CTRL) + DOT11_OFST_SEQ_CTRL)
#define DOT11_OFST_FRAMEBODY (sizeof(u16) * 3 + DOT11_OFST_ADRS4)

#define B_FC_PROTVER  0x0001
#define B_FC_TYPE     0x0004
#define B_FC_SUBTYPE  0x0010
#define B_FC_TODS     0x0100
#define B_FC_FROMDS   0x0200
#define B_FC_MOREFRAG 0x0400
#define B_FC_RETRY    0x0800
#define B_FC_PWRMAN   0x1000
#define B_FC_MOREDATA 0x2000
#define B_FC_WEP      0x4000
#define B_FC_ORDER    0x8000

#define B_FC_NULL    0x0040
#define B_FC_DATA    0x0008
#define B_FC_CONTROL 0x0004

#define BM_FC_PROTVER  0x0003
#define BM_FC_TYPE     0x000C
#define BM_FC_SUBTYPE  0x00F0
#define BM_FC_TODS     0x0100
#define BM_FC_FROMDS   0x0200
#define BM_FC_MOREFRAG 0x0400
#define BM_FC_RETRY    0x0800
#define BM_FC_PWRMAN   0x1000
#define BM_FC_MOREDATA 0x2000
#define BM_FC_WEP      0x4000
#define BM_FC_ORDER    0x8000

#define B_FC_TXED         0x0001
#define B_FC_TXQING       0x0002
#define B_FC_TXED_CHECKED 0x0004
#define B_FC_ICV_ERR      0x0001

#define B_FC_SUBTYPE_CFPOLL  0x0020
#define BM_FC_SUBTYPE_CFPOLL 0x0020

#define MAX_FC_NUM      0x30
#define TYPE_MANAGEMENT 0x00
#define TYPE_CONTROL    0x01
#define TYPE_DATA       0x02

#define FC_AP_DATA    (TYPE_DATA * B_FC_TYPE + B_FC_FROMDS)
#define FC_INFRA_DATA (TYPE_DATA * B_FC_TYPE + BM_FC_TODS)
#define FC_IBSS_DATA  (TYPE_DATA * B_FC_TYPE)
#define FC_ADHOC_DATA (TYPE_DATA * B_FC_TYPE)
#define FC_TEST_DATA  (TYPE_DATA * B_FC_TYPE)
#define FC_LAN_DATA   (TYPE_DATA * B_FC_TYPE + B_FC_FROMDS + BM_FC_TODS)

#define SUBTYPE_ASSREQ   0x00
#define SUBTYPE_ASSRES   0x01
#define SUBTYPE_REASSREQ 0x02
#define SUBTYPE_REASSRES 0x03
#define SUBTYPE_PRBREQ   0x04
#define SUBTYPE_PRBRES   0x05
#define SUBTYPE_BEACON   0x08
#define SUBTYPE_ATIM     0x09
#define SUBTYPE_DISASS   0x0A
#define SUBTYPE_AUTH     0x0B
#define SUBTYPE_DEAUTH   0x0C
#define SUBTYPE_ACTION   0x0D

#define SUBTYPE_PSPOLL      0x0A
#define SUBTYPE_RTS         0x0B
#define SUBTYPE_CTS         0x0C
#define SUBTYPE_ACK         0x0D
#define SUBTYPE_CFEND       0x0E
#define SUBTYPE_CFEND_CFACK 0x0F

#define SUBTYPE_DATA              0x00
#define SUBTYPE_DATA_CFACK        0x01
#define SUBTYPE_DATA_CFPOLL       0x02
#define SUBTYPE_DATA_CFACK_CFPOLL 0x03
#define SUBTYPE_NULL              0x04
#define SUBTYPE_CFACK             0x05
#define SUBTYPE_CFPOLL            0x06
#define SUBTYPE_CFACK_CFPOLL      0x07

#define FC_ASSREQ \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_ASSREQ * B_FC_SUBTYPE))
#define FC_ASSRES \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_ASSRES * B_FC_SUBTYPE))
#define FC_REASSREQ \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_REASSREQ * B_FC_SUBTYPE))
#define FC_REASSRES \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_REASSRES * B_FC_SUBTYPE))
#define FC_PRBREQ \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_PRBREQ * B_FC_SUBTYPE))
#define FC_PRBRES \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_PRBRES * B_FC_SUBTYPE))
#define FC_BEACON \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_BEACON * B_FC_SUBTYPE))
#define FC_ATIM ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_ATIM * B_FC_SUBTYPE))
#define FC_DISASS \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_DISASS * B_FC_SUBTYPE))
#define FC_AUTH ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_AUTH * B_FC_SUBTYPE))
#define FC_DEAUTH \
    ((TYPE_MANAGEMENT * B_FC_TYPE) | (SUBTYPE_DEAUTH * B_FC_SUBTYPE))

#define FC_PSPOLL ((TYPE_CONTROL * B_FC_TYPE) | (SUBTYPE_PSPOLL * B_FC_SUBTYPE))
#define FC_RTS    ((TYPE_CONTROL * B_FC_TYPE) | (SUBTYPE_RTS * B_FC_SUBTYPE))
#define FC_CTS    ((TYPE_CONTROL * B_FC_TYPE) | (SUBTYPE_CTS * B_FC_SUBTYPE))
#define FC_ACK    ((TYPE_CONTROL * B_FC_TYPE) | (SUBTYPE_ACK * B_FC_SUBTYPE))
#define FC_CFEND  ((TYPE_CONTROL * B_FC_TYPE) | (SUBTYPE_CFEND * B_FC_SUBTYPE))
#define FC_CFEND_CFACK \
    ((TYPE_CONTROL * B_FC_TYPE) | (SUBTYPE_CFEND_CFACK * B_FC_SUBTYPE))

#define FC_DATA ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_DATA * B_FC_SUBTYPE))
#define FC_DATA_CFACK \
    ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_DATA_CFACK * B_FC_SUBTYPE))
#define FC_DATA_CFPOLL \
    ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_DATA_CFPOLL * B_FC_SUBTYPE))
#define FC_DATA_CFACK_CFPOLL \
    ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_DATA_CFACK_CFPOLL * B_FC_SUBTYPE))
#define FC_NULL   ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_NULL * B_FC_SUBTYPE))
#define FC_CFACK  ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_CFACK * B_FC_SUBTYPE))
#define FC_CFPOLL ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_CFPOLL * B_FC_SUBTYPE))
#define FC_CFACK_CFPOLL \
    ((TYPE_DATA * B_FC_TYPE) | (SUBTYPE_CFACK_CFPOLL * B_FC_SUBTYPE))

#define FUNC_ASSREQ   ((TYPE_MANAGEMENT << 4) | SUBTYPE_ASSREQ)
#define FUNC_ASSRES   ((TYPE_MANAGEMENT << 4) | SUBTYPE_ASSRES)
#define FUNC_REASSREQ ((TYPE_MANAGEMENT << 4) | SUBTYPE_REASSREQ)
#define FUNC_REASSRES ((TYPE_MANAGEMENT << 4) | SUBTYPE_REASSRES)
#define FUNC_PRBREQ   ((TYPE_MANAGEMENT << 4) | SUBTYPE_PRBREQ)
#define FUNC_PBRRES   ((TYPE_MANAGEMENT << 4) | SUBTYPE_PRBRES)
#define FUNC_BEACON   ((TYPE_MANAGEMENT << 4) | SUBTYPE_BEACON)
#define FUNC_ATIM     ((TYPE_MANAGEMENT << 4) | SUBTYPE_ATIM)
#define FUNC_DISASS   ((TYPE_MANAGEMENT << 4) | SUBTYPE_DISASS)
#define FUNC_AUTH     ((TYPE_MANAGEMENT << 4) | SUBTYPE_AUTH)
#define FUNC_DEAUTH   ((TYPE_MANAGEMENT << 4) | SUBTYPE_DEAUTH)

#define FUNC_PSPOLL      ((TYPE_CONTROL << 4) | SUBTYPE_PSPOLL)
#define FUNC_RTS         ((TYPE_CONTROL << 4) | SUBTYPE_RTS)
#define FUNC_CTS         ((TYPE_CONTROL << 4) | SUBTYPE_CTS)
#define FUNC_ACK         ((TYPE_CONTROL << 4) | SUBTYPE_ACK)
#define FUNC_CFEND       ((TYPE_CONTROL << 4) | SUBTYPE_CFEND)
#define FUNC_CFEND_CFACK ((TYPE_CONTROL << 4) | SUBTYPE_CFEND_CFACK)

#define FUNC_DATA              ((TYPE_DATA << 4) | SUBTYPE_DATA)
#define FUNC_DATA_CFACK        ((TYPE_DATA << 4) | SUBTYPE_DATA_CFACK)
#define FUNC_DATA_CFPOLL       ((TYPE_DATA << 4) | SUBTYPE_DATA_CFPOLL)
#define FUNC_DATA_CFACK_CFPOLL ((TYPE_DATA << 4) | SUBTYPE_DATA_CFACK_CFPOLL)
#define FUNC_NULL              ((TYPE_DATA << 4) | SUBTYPE_NULL)
#define FUNC_CFACK             ((TYPE_DATA << 4) | SUBTYPE_CFACK)
#define FUNC_CFPOLL            ((TYPE_DATA << 4) | SUBTYPE_CFPOLL)
#define FUNC_CFACK_CFPOLL      ((TYPE_DATA << 4) | SUBTYPE_CFACK_CFPOLL)

#define B_CAPA_ESS             (0x01 << 0)
#define B_CAPA_IBSS            (0x01 << 1)
#define B_CAPA_CF_POLLABLE     (0x01 << 2)
#define B_CAPA_CF_POLLREQ      (0x01 << 3)
#define B_CAPA_PRIVACY         (0x01 << 4)
#define B_CAPA_SHORTPREAMBLE   (0x01 << 5)
#define B_CAPA_PBCC            (0x01 << 6)
#define B_CAPA_CHANNEL_AGILITY (0x01 << 7)
#define B_CAPA_BRIDGE          (0x01 << 10)

#define BM_CAPA_NG_MASK_AP   0xFFC2
#define BM_CAPA_NG_MASK_IBSS 0xFFD1
#define BM_CAPA_CHECK_MASK   (B_CAPA_ESS | B_CAPA_IBSS | B_CAPA_PRIVACY)

#define B_IV_KEYID (0x01 << 14)

#define MAX_FRAME_LENGTH (24 + 4 + 2312 + 4 + 4)

#define TIME_SLOT 20
#define TIME_SIFS 10
#define TIME_PIFS (TIME_SIFS + TIME_SLOT)
#define TIME_DIFS (TIME_PIFS + TIME_SLOT)

#define TIME_PREAMBLE_SHORT 96
#define TIME_PREAMBLE_LONG  192

#define TIME_BYTE_1M 8
#define TIME_BYTE_2M 4

#endif // __DEFFRM_H_
