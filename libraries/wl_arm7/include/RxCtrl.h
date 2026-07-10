#ifndef __RXCTRL_H_
#define __RXCTRL_H_

#include "Dot11Frm.h"

typedef struct {
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    u8 Body[4];
} RXFRM_MAC, *LPRXFRM_MAC;

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    u8 Body[4];
} RXFRM, *LPRXFRM;

typedef struct {
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    u16 TXOP;
    u16 Bitmap;
    u8 Body[4];
} RXMPFRM_MAC, *LPRXMPFRM_MAC;

typedef struct {
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    u16 TMPTT;
    u16 Bitmap;
} TRMPACKFRM_MAC, *LPRXMPACKFRM_MAC;

typedef struct {
    u16 wlRsv[WL_RSV];
    WlCmdHeader header;

    RXFRM frame;
} RXPACKET, *LPRXPACKET;

#define MAX_DEFRAG_NUM       3
#define MAX_DEFRAG_LIFETIME  5
#define MAX_DEFRAG_DATASIZE  1508
#define MAX_DEFRAG_FRAMESIZE (8 + 12 + 24 + MAX_DEFRAG_DATASIZE)
#define MAX_DEFRAG_PACKETSIZE \
    (sizeof(WlCmdHeader) + MAX_DEFRAG_FRAMESIZE + 4 + sizeof(WlCmdCfm))

typedef struct {
    u16 DA[3];
    u16 SA[3];
    SEQ_CTRL SeqCtrl;
    u16 pad;
} DEFRAG_TBL, *LPDEFRAG_TBL;

typedef struct {
    u16 RestTime;
    u16 UnitLength;
    DEFRAG_TBL DefragTbl;
    LPRXPACKET pPacket;
} DEFRAG_LIST, *LPDEFRAG_LIST;

typedef struct {
    u16 LastMpSeq;
    u16 IcvOkCntFlag;

    u16 wlCurr;
    u16 TxKeyReg;

    DEFRAG_LIST DefragList[MAX_DEFRAG_NUM];

} RX_CTRL, *LPRX_CTRL;

void RxDataFrameTask(void);
u32 RxMpFrame(LPRXFRM pFrm);
void RxKeyDataFrame(LPRXFRM pFrm);
u32 RxMpAckFrame(LPRXFRM pFrm);

void RxBeaconFrame(LPBEACON_FRAME pFrm);
void RxManCtrlTask(void);
void InitRxCtrl(void);

void DefragTask(void);
void DefragTimerTask(void);

#define GetFragCount(_x_) ((_x_ & RXSTS_FRAGCNT_MASK) / RXSTS_FRAGCNT)

#define TIM_FREE 0x00
#define TIM_BC   0x01
#define TIM_UC   0x02

#ifdef __RXCTRL_C_

typedef struct {
    u8 *pElement;

    u16 rxStatus;
    u16 capability;

    u16 bodyLength;
    u16 matchFlag;
    u16 foundFlag;

    u16 activeZone;
    u16 vtsf;
    u16 channel;
    RATE_SET rateSet;

    u16 otherElementCount;
    u16 otherElementLength;

    LPSSID_ELEMENT pSSID;
    LPCF_PARAM_ELEMENT pCFP;
    LPTIM_ELEMENT pTIM;
    LPGAME_INFO_ELEMENT pGMIF;
} ELEMENT_CHECKER, *LPELEMENT_CHECKER;

#define B_MATCH_SSID     0x0001
#define B_MATCH_CHANNEL  0x0002
#define B_MATCH_RATESET  0x0004
#define B_MATCH_BSSID    0x0008
#define B_MATCH_CAPA_ESS 0x0010
#define B_MATCH_CAPA_WEP 0x0020

#define B_FOUND_SSID     0x0001
#define B_FOUND_CHANNEL  0x0002
#define B_FOUND_RATESET  0x0004
#define B_FOUND_BSSID    0x0008
#define B_FOUND_CAPA     0x0030
#define B_FOUND_ACTZONE  0x0040
#define B_FOUND_VTSF     0x0080
#define B_FOUND_TIM      0x0100
#define B_FOUND_CFP      0x0200
#define B_FOUND_GAMEINFO 0x0400
#define B_FOUND_BCSSID   0x0800

static void RxDisAssFrame(LPDISASS_FRAME pFrm);
static void RxAssReqFrame(LPASSREQ_FRAME pFrm);
static void RxAssResFrame(LPASSRES_FRAME pFrm);
static void RxReAssReqFrame(LPREASSREQ_FRAME pFrm);
static void RxReAssResFrame(LPREASSRES_FRAME pFrm);
static void RxProbeReqFrame(LPPRBREQ_FRAME pFrm);
static void RxProbeResFrame(LPPRBRES_FRAME pFrm, LPELEMENT_CHECKER pChk);
static void RxAuthFrame(LPAUTH_FRAME pFrm);
static void RxDeAuthFrame(LPDEAUTH_FRAME pFrm);
static void RxPsPollFrame(LPPSPOLL_FRAME pFrm);
static void RxCfEndFrame(LPCFEND_FRAME pFrm);

static void ElementChecker(LPELEMENT_CHECKER p);

static void SetChallengeText(u32 camAdrs, LPAUTH_FRAME pFrm);
static u32 CheckChallengeText(LPAUTH_FRAME pFrm);

static void NewDefragment(LPRXFRM_MAC pMFrm, LPDEFRAG_TBL pDefragTbl);
static void MoreDefragment(LPRXFRM_MAC pMFrm, LPDEFRAG_TBL pDefragTbl);

#endif // __RXCTRL_C_
#endif // __RXCTRL_H_
