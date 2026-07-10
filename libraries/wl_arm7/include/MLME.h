#ifndef __MLME_H_
#define __MLME_H_

#include "WlCmd.h"
#include "TxCtrl.h"

#define MLME_STATE_IDLE   0x00
#define MLME_STATE_SCAN   0x10
#define MLME_STATE_JOIN   0x20
#define MLME_STATE_AUTH   0x30
#define MLME_STATE_DEAUTH 0x40
#define MLME_STATE_ASS    0x50
#define MLME_STATE_REASS  0x60
#define MLME_STATE_DISASS 0x70
#define MLME_STATE_MEAS   0x80

#define MLME_STATE_MAIN_MASK 0xF0

#define MLME_STATE_SCAN_STARTUP    (0x00 | MLME_STATE_SCAN)
#define MLME_STATE_SCAN_SETCHANNEL (0x01 | MLME_STATE_SCAN)
#define MLME_STATE_SCAN_START      (0x02 | MLME_STATE_SCAN)
#define MLME_STATE_SCAN_ING        (0x03 | MLME_STATE_SCAN)
#define MLME_STATE_SCAN_END        (0x04 | MLME_STATE_SCAN)
#define MLME_STATE_SCAN_FIN        (0x05 | MLME_STATE_SCAN)

#define MLME_STATE_JOIN_STARTUP (0x00 | MLME_STATE_JOIN)
#define MLME_STATE_JOIN_ING     (0x01 | MLME_STATE_JOIN)
#define MLME_STATE_JOIN_TIMEOUT (0x02 | MLME_STATE_JOIN)
#define MLME_STATE_JOIN_OK      (0x03 | MLME_STATE_JOIN)
#define MLME_STATE_JOIN_NG      (0x04 | MLME_STATE_JOIN)
#define MLME_STATE_JOIN_FIN     (0x05 | MLME_STATE_JOIN)

#define MLME_STATE_AUTH_STARTUP (0x00 | MLME_STATE_AUTH)
#define MLME_STATE_AUTH_ING     (0x01 | MLME_STATE_AUTH)
#define MLME_STATE_AUTH_STEP2   (0x02 | MLME_STATE_AUTH)
#define MLME_STATE_AUTH_ING2    (0x03 | MLME_STATE_AUTH)
#define MLME_STATE_AUTH_TIMEOUT (0x04 | MLME_STATE_AUTH)
#define MLME_STATE_AUTH_FIN     (0x05 | MLME_STATE_AUTH)

// ASS STATE
#define MLME_STATE_ASS_STARTUP (0x00 | MLME_STATE_ASS)
#define MLME_STATE_ASS_ING     (0x01 | MLME_STATE_ASS)
#define MLME_STATE_ASS_TIMEOUT (0x02 | MLME_STATE_ASS)
#define MLME_STATE_ASS_FIN     (0x03 | MLME_STATE_ASS)

#define MLME_STATE_REASS_STARTUP (0x00 | MLME_STATE_REASS)
#define MLME_STATE_REASS_ING     (0x01 | MLME_STATE_REASS)
#define MLME_STATE_REASS_TIMEOUT (0x02 | MLME_STATE_REASS)
#define MLME_STATE_REASS_FIN     (0x03 | MLME_STATE_REASS)

#define MLME_STATE_DISASS_STARTUP (0x00 | MLME_STATE_DISASS)
#define MLME_STATE_DISASS_ING     (0x01 | MLME_STATE_DISASS)
#define MLME_STATE_DISASS_TIMEOUT (0x02 | MLME_STATE_DISASS)
#define MLME_STATE_DISASS_FIN     (0x03 | MLME_STATE_DISASS)

#define MLME_STATE_DEAUTH_STARTUP (0x00 | MLME_STATE_DEAUTH)
#define MLME_STATE_DEAUTH_ING     (0x01 | MLME_STATE_DEAUTH)
#define MLME_STATE_DEAUTH_TIMEOUT (0x02 | MLME_STATE_DEAUTH)
#define MLME_STATE_DEAUTH_FIN     (0x03 | MLME_STATE_DEAUTH)

#define MLME_STATE_MEAS_STARTUP    (0x00 | MLME_STATE_MEAS)
#define MLME_STATE_MEAS_SETCHANNEL (0x01 | MLME_STATE_MEAS)
#define MLME_STATE_MEAS_MEASURE    (0x02 | MLME_STATE_MEAS)
#define MLME_STATE_MEAS_END        (0x03 | MLME_STATE_MEAS)
#define MLME_STATE_MEAS_FIN        (0x04 | MLME_STATE_MEAS)

typedef struct {
    u16 State;
    u16 pad;

    union {
        struct {
            u16 MaxConfirmLength;
            u16 ChannelCount;
            u16 bFound;
            u16 ElapseTime;
            u16 TxPeriod;
        } Scan;

        struct {
            u16 Result;
            u16 Status;
        } Join;

        struct {
            LPTXFRM pTxFrm;
        } DeAuth;

        struct {
            LPTXFRM pTxFrm;
        } DisAss;

        struct {
            u32 Counter;
            u32 CCA;
            u16 bkPowerMode;
            u16 bkCCAMode;
            u16 bkEdTh;
            u16 bkMode;
            u16 Channel;
            u16 sts;
        } Measure;
    } Work;

    union {
        WlMlmeResetReq *Reset;
        WlMlmePowerMgtReq *PwrMgt;
        WlMlmeScanReq *Scan;
        WlMlmeJoinReq *Join;
        WlMlmeAuthReq *Auth;
        WlMlmeDeAuthReq *DeAuth;
        WlMlmeAssReq *Ass;
        WlMlmeReAssReq *ReAss;
        WlMlmeDisAssReq *DisAss;
        WlMlmeStartReq *Start;
        WlMlmeMeasChanReq *MeasChannel;
    } pReq;

    union {
        WlCmdCfm *Cfm;
        WlMlmeResetCfm *Reset;
        WlMlmePowerMgtCfm *PwrMgt;
        WlMlmeScanCfm *Scan;
        WlMlmeJoinCfm *Join;
        WlMlmeAuthCfm *Auth;
        WlMlmeDeAuthCfm *DeAuth;
        WlMlmeAssCfm *Ass;
        WlMlmeReAssCfm *ReAss;
        WlMlmeDisAssCfm *DisAss;
        WlMlmeStartCfm *Start;
        WlMlmeMeasChanCfm *MeasChannel;
    } pCfm;
} MLME_MAN, *LPMLME_MAN;

u16 MLME_ResetReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_PwrMgtReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_ScanReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_JoinReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_AuthReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_DeAuthReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_AssReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_ReAssReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_DisAssReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_StartReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MLME_MeasChanReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

void MLME_ScanTask(void);
void MLME_JoinTask(void);
void MLME_AuthTask(void);
void MLME_DeAuthTask(void);
void MLME_AssTask(void);
void MLME_ReAssTask(void);
void MLME_DisAssTask(void);
void MLME_MeasChannelTask(void);

void MLME_BeaconLostTask(void);

void IssueMlmeConfirm(void);

u32 MLME_IssueAuthIndication(u16 *pMacAdrs, u16 algorithm);
u32 MLME_IssueDeAuthIndication(u16 *pMacAdrs, u16 reason);
u32 MLME_IssueAssIndication(u16 *pMacAdrs, u16 aid, LPSSID_ELEMENT pSSID);
u32 MLME_IssueReAssIndication(u16 *pMacAdrs, u16 aid, LPSSID_ELEMENT pSSID);
u32 MLME_IssueDisAssIndication(u16 *pMacAdrs, u16 reason);
u32 MLME_IssueBeaconLostIndication(u16 *pMacAdrs);
u32 MLME_IssueBeaconSendIndication(void);
u32 MLME_IssueBeaconRecvIndication(void *pRxFrm);

void InitializeMLME(void);

#define IssueMlmeIndication(_pBufMan_, _pBuf_) \
    SendMessageToWmDirect(_pBufMan_, _pBuf_)

#ifdef __MLME_C_

static void MLME_ScanTimeOut(void *arg);
static void MLME_JoinTimeOut(void *arg);
static void MLME_AuthTimeOut(void *arg);
static void MLME_AssTimeOut(void *arg);
static void MLME_ReAssTimeOut(void *arg);
static void MLME_MeasChanTimeOut(void *arg);

#endif // __MLME_C_
#endif // __MLME_H_
