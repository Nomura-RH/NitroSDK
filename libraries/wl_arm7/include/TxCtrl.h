#ifndef __TXCTRL_H_
#define __TXCTRL_H_

#include "Dot11Frm.h"
#include "WlFrame.h"
#include "WlCmd.h"

typedef struct {
    FIRM_HEADER FirmHeader;
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    union {
        u8 Body[4];
        u8 *Pointer;
    } Data;
} TXFRM, *LPTXFRM;

typedef struct {
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    u8 Body[4];
} TXFRM_MAC, *LPTXFRM_MAC;

typedef struct {
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    u16 TXOP;
    u16 Bitmap;
    u8 Body[4];
} TXMPFRM_MAC, *LPTXMPFRM_MAC;

typedef struct {
    MAC_HEADER MacHeader;
    DATA_HEADER Dot11Header;
    u16 TMPTT;
    u16 Bitmap;
} TXMPACKFRM_MAC, *LPTXMPACKFRM_MAC;

typedef struct {
    u16 Busy;
    u16 InCount;
    u16 OutCount;
    u16 pad;
    LPTXFRM_MAC pMacFrm;
    LPTXFRM pFrm;
    void (*pEndFunc)(LPTXFRM pFrm, u32 flag);
} TXQ, *LPTXQ;

typedef struct {
    TXQ Txq[3];

    TXQ Mp;
    TXQ Key[2];
    TXQ Beacon;

    u32 Flag;

    WlMaMpEndInd *pMpEndInd;
    u16 TXOP;
    u16 TMPTT;
    u16 SetKeyMap;
    u16 GetKeyMap;
    u16 DataLength;
    u16 RetryLimit;
    u16 RestBitmap;

    //	u16				SaveMap;

    u16 BkKeyIn;
    u16 BkKeyOut;

    u16 BkSeqNum;
    u16 MpBlkCnt;
    u16 MpRstCnt;
    u16 MpLastOk;
    u16 TimeOutFrm;
} TX_CTRL, *LPTX_CTRL;

#define TXQ_FLAG_MP_RESUME    0x8000
#define TXQ_FLAG_SUSPEND_TBTT 0x0002

void TxqPri(u32 pri);
void TxqBroadCast(void);
void CopyTxFrmToMacBuf(LPTXFRM_MAC pMacTxFrm, WlMaDataReq *pTxReq);
u32 CheckFrameTimeout(LPTXFRM pTxFrm);

void TxqEndData(LPTXFRM pFrm, u32 flag);
void TxqEndManCtrl(LPTXFRM pFrm, u32 flag);
void TxqEndPsPoll(LPTXFRM pFrm, u32 flag);
void TxEndKeyData(LPTXQ pTxq);

void ResetTxqPri(u32 pri);

void DeleteTxFrames(u32 camAdrs);
void DeleteTxFrameByAdrs(u16 *pMacAdrs);
void DeleteAllTxFrames(void);
void MessageDeleteTx(u32 pri, u32 bMsg);

void ClearTxKeyData(void);
void ClearTxMp(void);
void ClearTxData(void);
void ClearQueuedPri(u32 pri);

void StartBeaconFrame(void);
void StopBeaconFrame(void);

void TxManCtrlFrame(LPTXFRM pFrm);
void SetManCtrlFrame(LPTXFRM pFrm);
void TxPsPollFrame(void);

void MakeBeaconFrame(void);
void UpdateGameInfoElement(void);
LPDISASS_FRAME MakeDisAssFrame(u16 *pDA, u16 reasonCode);
LPASSREQ_FRAME MakeAssReqFrame(u16 *pDA);
LPREASSREQ_FRAME MakeReAssReqFrame(u16 *pDA);
LPASSRES_FRAME MakeAssResFrame(u16 camAdrs, u16 statusCode, LPSSID_ELEMENT pSSID);
LPREASSRES_FRAME MakeReAssResFrame(u16 camAdrs, u16 statusCode, LPSSID_ELEMENT pSSID);
LPPRBREQ_FRAME MakeProbeReqFrame(u16 *pDA);
LPPRBRES_FRAME MakeProbeResFrame(u16 *pDA);
LPAUTH_FRAME MakeAuthFrame(u16 *pDA, u16 txtLen, u32 bCheck);
LPDEAUTH_FRAME MakeDeAuthFrame(u16 *pDA, u16 reasonCode, u32 bCheck);
void MakePsPollFrame(u16 aid);

u32 IsExistManFrame(u16 *pDA, u16 frameCtrl);

// void TxqOpen(u32 map);
#if (EMU_MACREG)
#define TxqOpen(_x_)  *(vu16 *)MREG_TXQ_MAP |= _x_
#define TxqClose(_x_) *(vu16 *)MREG_TXQ_MAP &= ~_x_
#else
#define TxqOpen(_x_)  *(vu16 *)MREG_TXQ_OPEN = _x_
#define TxqClose(_x_) *(vu16 *)MREG_TXQ_CLOSE = _x_
#endif

void InitTxCtrl(void);

#define DESTROY_TXBUF_ID0 0xB6B8
#define DESTROY_TXBUF_ID1 0x1D46

#ifdef __TXCTRL_C_

void TxqEndBroadCast(LPTXFRM pFrm, u32 flag);
void InitManHeader(LPTXFRM pFrm, u16 *pDA);

static u32 SetSSIDElement(u8 *pBuf);
static u32 SetSupRateSet(u8 *pBuf);
static u32 SetDSParamSet(u8 *pBuf);
static u32 SetGameInfoElement(u8 *pBuf);

#define MAX_MANAGEMENT_QUEUE 24

#define PACKET_HEADER        (sizeof(WlCmdHeader))
#define BEACON_PACKET_SIZE   (PACKET_HEADER + BEACON_FRAME_SIZE)
#define ATIM_PACKET_SIZE     (PACKET_HEADER + ATIM_FRAME_SIZE)
#define DISASS_PACKET_SIZE   (PACKET_HEADER + DISASS_FRAME_SIZE)
#define ASSRES_PACKET_SIZE   (PACKET_HEADER + ASSRES_FRAME_SIZE)
#define ASSREQ_PACKET_SIZE   (PACKET_HEADER + ASSREQ_FRAME_SIZE)
#define REASSREQ_PACKET_SIZE (PACKET_HEADER + REASSREQ_FRAME_SIZE)
#define REASSRES_PACKET_SIZE (PACKET_HEADER + REASSRES_FRAME_SIZE)
#define PRBRES_PACKET_SIZE   (PACKET_HEADER + PRBRES_FRAME_SIZE)
#define PRBREQ_PACKET_SIZE   (PACKET_HEADER + PRBREQ_FRAME_SIZE)
#define AUTH_PACKET_SIZE     (PACKET_HEADER + AUTH_FRAME_SIZE)
#define DEAUTH_PACKET_SIZE   (PACKET_HEADER + DEAUTH_FRAME_SIZE)

#endif // __TXCTRL_C_
#endif // __TXCTRL_H_
