#ifndef __CAM_H_
#define __CAM_H_

#include "TxCtrl.h"
#include "BufMan.h"
#include "WlStaList.h"

typedef WlStaElement CAM_ELEMENT;
typedef WlStaElement *LPCAM_ELEMENT;

typedef struct {
    u16 Count;

    u16 PowerMgtMode;
    u16 PowerState;
    u16 NotClass3;
    u16 NotSetTIM;
    u16 dmm;

    u16 ConnectSta;
    u16 UseAidMap;
    //	LPTXFRM		BcEnP;
    //	LPTXFRM		BcDeP;
    //	u16			BcCount;
} CAM_MAN, *LPCAM_MAN;

u32 CAM_Search(u16 *pMacAdrs);
u32 CAM_SearchAdd(u16 *pMacAdrs);

void CAM_AddBcFrame(LPHEAPBUF_MAN pBufMan, void *pBuf);
// void CAM_DelBcFrame(LPTXFRM pFrm);

void CAM_SetStaState(u16 camAdrs, u16 state);
void CAM_SetRSSI(u16 camAdrs, u16 rssi);
void CAM_SetPowerMgtMode(u16 camAdrs, u16 pmtMode);
void CAM_SetDoze(u32 camAdrs);
void CAM_SetAwake(u32 camAdrs);
void CAM_SetCapaInfo(u32 camAdrs, u32 capInfo);
void CAM_SetSupRate(u32 camAdrs, u32 SupRate);
void CAM_SetLastSeqCtrl(u32 camAdrs, u32 seqCtrl);
void CAM_SetAuthSeed(u32 camAdrs, u32 seed);
void CAM_UpdateLifeTime(u32 camAdrs);

u32 CAM_GetStaState(u32 camAdrs);
u32 CAM_IsActive(u32 camAdrs);
u32 CAM_GetPowerMgtMode(u32 camAdrs);
u16 *CAM_GetMacAdrs(u32 camAdrs);
u32 CAM_GetAuthSeed(u32 camAdrs);
u32 CAM_GetLastSeqCtrl(u32 camAdrs);
u32 CAM_GetTxRate(u32 camAdrs);
u32 CAM_GetAID(u32 camAdrs);
u32 CAM_GetFrameCount(u32 camAdrs);

void CAM_IncFrameCount(LPTXFRM pFrm);
void CAM_DecFrameCount(LPTXFRM pFrm);

void CAM_SetTIMElementBitmap(u32 camAdrs);
void CAM_ClrTIMElementBitmap(u32 camAdrs);

u32 CAM_AllocateAID(u16 camAdrs);
void CAM_ReleaseAID(u16 camAdrs);

void CAM_Delete(u16 camAdrs);
void InitializeCAM(void);
void InitCAM(void);
void CAM_TimerTask(void);
void CAM_Disp(void);

// #define	CAM_SetStaState(_cam_,_state_)		wlMan->pCAM[_cam_].State
// = _state_
#define CAM_CheckCAM(_cam_)
// #define	CAM_SetPowerMgtMode(_cam_,_mgt_)	{ CAM_CheckCAM(_cam_);
// wlMan->CamMan.PowerMgtMode = (wlMan->CamMan.PowerMgtMode & ~(1<<_cam_)) |
// (_mgt_<<_cam_); }

#define CAM_InitAllPowerState()                                                             \
    {                                                                                       \
        wlMan->CamMan.PowerState = (~wlMan->CamMan.PowerMgtMode) | wlMan->CamMan.NotClass3; \
    }

#define CAM_NOT_FOUND 255

#ifdef __CAM_C_

static void CAM_InitElement(u32 camAdrs, u16 *pMacAdrs);
WlStaElement *CAM_GetTbl(u32 camAdrs);

#define CAM_FREE         0x00
#define B_CAM_ACTIVE     0x01
#define MAX_RATE_COUNTER 0xFF
// #define	MAX_STA_LIFETIME	3600
#define MAX_STA_LIFETIME 0xFFFF // ³ÀÝè

#endif // __CAM_C_
#endif // __CAM_H_
