#define __CAM_C_
#define __INSYSROM__

#include "WlSys.h"
#include "WlLib.h"

#include "WlCmdLabel.h"
#include "Dot11Frm.h"
#include "TxCtrl.h"
#include "WlNic.h"
#include "MAC.h"
#include "Compati.h"

#include "CAM.h"

u32 CAM_Search(u16 *pMacAdrs)
{
    LPCAM_ELEMENT pCAM;
    u32 pos, i;

    if (pMacAdrs[0] & 0x0001) {
        return 0;
    }

    if (wlMan->CamMan.Count > 1) {
        pCAM = &wlMan->Config.pCAM[1];

        for (i = 0, pos = 1; pos < wlMan->Config.MaxStaNum; pos++, pCAM++) {
            if (pCAM->state != CAM_FREE) {
                if (MatchMacAdrs(pCAM->macAdrs, pMacAdrs)) {
                    return pos;
                }

                if (++i >= wlMan->CamMan.Count) {
                    break;
                }
            }
        }
    }

    return CAM_NOT_FOUND;
}

u32 CAM_SearchAdd(u16 *pMacAdrs)
{
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPCAM_ELEMENT pCAM;
    u32 pos, i, free, lifeTime;

    if (pMacAdrs[0] & 0x0001) {
        return 0;
    }

    if (wlMan->CamMan.Count > 1) {
        pCAM = &pConfig->pCAM[1];

        for (i = free = 0, pos = 1; pos < pConfig->MaxStaNum; pos++, pCAM++) {
            if (pCAM->state != CAM_FREE) {
                if (MatchMacAdrs(pCAM->macAdrs, pMacAdrs)) {
                    return pos;
                }

                if (++i >= wlMan->CamMan.Count) {
                    break;
                }
            } else if (free == 0) {
                free = pos;
            }
        }

        if (free != 0) {
            pos = free;
        }
    } else {
        pos = 1;
    }

    if (pos >= wlMan->Config.MaxStaNum) {
        pCAM = &pConfig->pCAM[0];
        lifeTime = 0x010000;
        for (i = 1, pos = 0; i < pConfig->MaxStaNum; i++) {
            if ((pCAM[i].state < STA_CLASS2) && (pCAM[i].frameCount == 0)) {
                if (lifeTime > pCAM[i].lifeTime) {
                    lifeTime = pCAM[i].lifeTime;
                    pos = i;
                }
            }
        }

        if (pos == 0) {
            return CAM_NOT_FOUND;
        }
    }

    CAM_InitElement(pos, pMacAdrs);

    return pos;
}

void CAM_AddBcFrame(LPHEAPBUF_MAN pBufMan, void *pBuf)
{
    LPHEAPBUF_MAN pBufManBc = &wlMan->HeapMan.TxPri[QID_BROADCAST];
    u32 x;

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (GetHeapBufCount(pBufManBc) == 0) {
        CAM_SetTIMElementBitmap(0);
    }

    MoveHeapBuf(pBufMan, pBufManBc, pBuf);

    OS_EnableIrqMask(x);
}

void CAM_IncFrameCount(LPTXFRM pFrm)
{
    LPCAM_ELEMENT pCAM;
    u32 x, cam_adrs;

    cam_adrs = pFrm->FirmHeader.CamAdrs;
    pCAM = CAM_GetTbl(cam_adrs);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (wlMan->Work.Mode == WL_CMDLABEL_MODE_PARENT) {
        if (pCAM->frameCount == 0) {
            CAM_SetTIMElementBitmap(cam_adrs);
        }
    }

    pCAM->frameCount++;

    OS_EnableIrqMask(x);

    if ((wlMan->CamMan.NotSetTIM & (1 << cam_adrs)) == 0) {
        pCAM->lifeTime = pCAM->maxLifeTime;
    }
}

void CAM_DecFrameCount(LPTXFRM pFrm)
{
    LPCAM_ELEMENT pCAM;
    u32 x, cam_adrs;

    cam_adrs = pFrm->FirmHeader.CamAdrs;
    pCAM = CAM_GetTbl(cam_adrs);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (wlMan->Work.Mode == WL_CMDLABEL_MODE_PARENT) {
        if (pCAM->frameCount == 1) {
            CAM_ClrTIMElementBitmap(cam_adrs);
        }
    }

    pCAM->frameCount--;

    OS_EnableIrqMask(x);
}

void CAM_SetStaState(u16 camAdrs, u16 state)
{
    u32 x;

    CAM_CheckCAM(camAdrs);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (state < STA_CLASS3) {
        CAM_SetAwake(camAdrs);
        wlMan->CamMan.NotClass3 |= (1 << camAdrs);

        if ((wlMan->Work.Mode == WL_CMDLABEL_MODE_PARENT) && (CAM_GetAID(camAdrs) != 0)) {
            CAM_ReleaseAID(camAdrs);
        }
    } else {
        wlMan->CamMan.NotClass3 &= ~(1 << camAdrs);

        if (CAM_GetPowerMgtMode(camAdrs)) {
            CAM_SetDoze(camAdrs);
        }
    }

    CAM_GetTbl(camAdrs)->state = state;

    OS_EnableIrqMask(x);
}

void CAM_SetRSSI(u16 camAdrs, u16 rssi)
{
    LPCAM_ELEMENT pCAM = CAM_GetTbl(camAdrs);

    CAM_CheckCAM(camAdrs);
    pCAM->rssi = rssi;
}

void CAM_SetPowerMgtMode(u16 camAdrs, u16 pmtMode)
{
    LPCAM_MAN pCamMan = &wlMan->CamMan;

    CAM_CheckCAM(camAdrs);

    pCamMan->PowerMgtMode = (pCamMan->PowerMgtMode & ~(1 << camAdrs)) | (pmtMode << camAdrs);

    if (pCamMan->PowerMgtMode & ~pCamMan->NotClass3) {
        *(vu16 *)MREG_TXQ_CLOSE = TXQ_CLOSE_TXQ2;
    } else {
        *(vu16 *)MREG_TXQ_OPEN = TXQ_OPEN_TXQ2;
    }
}

void CAM_SetDoze(u32 camAdrs)
{
    CAM_CheckCAM(camAdrs);

    if (CAM_GetStaState(camAdrs) == STA_CLASS3) {
        wlMan->CamMan.PowerState &= ~(1 << camAdrs);
    }
}

void CAM_SetAwake(u32 camAdrs)
{
    CAM_CheckCAM(camAdrs);

    wlMan->CamMan.PowerState |= (1 << camAdrs);
}

void CAM_SetCapaInfo(u32 camAdrs, u32 capInfo)
{
    CAM_CheckCAM(camAdrs);

    CAM_GetTbl(camAdrs)->capaInfo = capInfo;
}

void CAM_SetSupRate(u32 camAdrs, u32 SupRate)
{
    CAM_CheckCAM(camAdrs);

    CAM_GetTbl(camAdrs)->supRateSet = SupRate;
}

void CAM_SetLastSeqCtrl(u32 camAdrs, u32 seqCtrl)
{
    CAM_CheckCAM(camAdrs);

    CAM_GetTbl(camAdrs)->lastSeqCtrl = seqCtrl;
}

void CAM_SetAuthSeed(u32 camAdrs, u32 seed)
{
    CAM_CheckCAM(camAdrs);
    CAM_GetTbl(camAdrs)->authSeed = seed;
}

void CAM_UpdateLifeTime(u32 camAdrs)
{
    CAM_CheckCAM(camAdrs);

    wlMan->Config.pCAM[camAdrs].lifeTime = wlMan->Config.pCAM[camAdrs].maxLifeTime;
}

u32 CAM_AllocateAID(u16 camAdrs)
{
    LPCAM_MAN pCamMan = &wlMan->CamMan;
    u32 i, map, x;

    CAM_CheckCAM(camAdrs);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    for (i = 1, map = 2; i < 16; i++, map <<= 1) {
        if ((pCamMan->UseAidMap & map) == 0) {
            pCamMan->UseAidMap |= map;

            pCamMan->ConnectSta++;

            if (pCamMan->ConnectSta == 1) {
                WEnableTmpttPowerSave();
            }

            CAM_GetTbl(camAdrs)->aid = i;

            OS_EnableIrqMask(x);

            return i;
        }
    }

    OS_EnableIrqMask(x);

    return 0;
}

void CAM_ReleaseAID(u16 camAdrs)
{
    LPWL_MAN pWlMan = (LPWL_MAN)&wlMan->TaskMan;

    u32 aid, map;

    CAM_CheckCAM(camAdrs);

    CAM_ClrTIMElementBitmap(camAdrs);

    aid = CAM_GetAID(camAdrs);
    if (aid) {
        map = 0x0001 << aid;

        CAM_GetTbl(camAdrs)->aid = 0;

        pWlMan->CamMan.UseAidMap &= ~map;

        pWlMan->CamMan.ConnectSta--;

        if (pWlMan->CamMan.ConnectSta == 0) {
            WDisableTmpttPowerSave();
        }
    }
}

u32 CAM_GetStaState(u32 camAdrs)
{
    return CAM_GetTbl(camAdrs)->state;
}

u32 CAM_IsActive(u32 camAdrs)
{
    return (wlMan->CamMan.PowerState >> camAdrs) & 1;
}

u32 CAM_GetPowerMgtMode(u32 camAdrs)
{
    return (wlMan->CamMan.PowerMgtMode >> camAdrs) & 1;
}

u16 *CAM_GetMacAdrs(u32 camAdrs)
{
    return CAM_GetTbl(camAdrs)->macAdrs;
}

u32 CAM_GetAuthSeed(u32 camAdrs)
{
    return CAM_GetTbl(camAdrs)->authSeed;
}

u32 CAM_GetLastSeqCtrl(u32 camAdrs)
{
    return CAM_GetTbl(camAdrs)->lastSeqCtrl;
}

u32 CAM_GetTxRate(u32 camAdrs)
{
    if (CAM_GetTbl(camAdrs)->supRateSet & 2) {
        return RF_RATE_2M;
    }

    return RF_RATE_1M;
}

u32 CAM_GetAID(u32 camAdrs)
{
    return CAM_GetTbl(camAdrs)->aid;
}

u32 CAM_GetFrameCount(u32 camAdrs)
{
    return CAM_GetTbl(camAdrs)->frameCount;
}

WlStaElement *CAM_GetTbl(u32 camAdrs)
{
    return &wlMan->Config.pCAM[camAdrs];
}

void CAM_SetTIMElementBitmap(u32 camAdrs)
{
    LPTIM_ELEMENT pTIM;
    u32 ofst, bit, tmp, x, aid;

    CAM_CheckCAM(camAdrs);

    if (CAM_GetStaState(camAdrs) != STA_CLASS3) {
        return;
    }

    if (wlMan->CamMan.NotSetTIM & (1 << camAdrs)) {
        return;
    }

    pTIM = (LPTIM_ELEMENT)(MAC_MEM_BASE + MBUF_PARENT_BEACON + 12 + 24 + (u32)wlMan->Work.Ofst.Beacon.TIM);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (camAdrs == 0) {
        tmp = WL_ReadByte(&pTIM->BitmapCtrl) | 0x01;
        WL_WriteByte(&pTIM->BitmapCtrl, tmp);
    } else {
        aid = CAM_GetAID(camAdrs);

        ofst = aid / 8;
        bit = aid & 7;

        tmp = WL_ReadByte(&pTIM->VitrualBitmap[ofst]) | (0x01 << bit);
        WL_WriteByte(&pTIM->VitrualBitmap[ofst], tmp);
    }

    OS_EnableIrqMask(x);
}

void CAM_ClrTIMElementBitmap(u32 camAdrs)
{
    LPTIM_ELEMENT pTIM;
    u32 ofst, bit, tmp, x, aid;

    CAM_CheckCAM(camAdrs);

    if (CAM_GetStaState(camAdrs) != STA_CLASS3) {
        return;
    }

    pTIM = (LPTIM_ELEMENT)(MAC_MEM_BASE + MBUF_PARENT_BEACON + 12 + 24 + (u32)wlMan->Work.Ofst.Beacon.TIM);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (camAdrs == 0) {
        tmp = WL_ReadByte(&pTIM->BitmapCtrl) & 0xFE;
        WL_WriteByte(&pTIM->BitmapCtrl, tmp);
    } else {
        aid = CAM_GetAID(camAdrs);

        ofst = aid / 8;
        bit = aid & 7;

        tmp = WL_ReadByte(&pTIM->VitrualBitmap[ofst]) & ~(0x01 << bit);
        WL_WriteByte(&pTIM->VitrualBitmap[ofst], tmp);
    }

    OS_EnableIrqMask(x);
}

void CAM_TimerTask(void)
{
    LPWL_MAN pWlMan = (LPWL_MAN)&wlMan->TaskMan;
    LPCAM_ELEMENT pCAM = &pWlMan->Config.pCAM[1];
    LPDEAUTH_FRAME pTxDeAuthFrm;
    u32 pos, i;
    u32 cnt, state;

    cnt = pWlMan->CamMan.Count;
    for (i = 0, pos = 1; pos < pWlMan->Config.MaxStaNum; pos++, pCAM++) {
        if (pCAM->state != CAM_FREE) {
            if ((pCAM->lifeTime != 0) && (pCAM->lifeTime != 0xFFFF) && (--pCAM->lifeTime == 0)) {
                if (pCAM->state >= STA_CLASS1) {
                    state = CAM_GetStaState(pos);

                    CAM_SetStaState(pos, STA_CLASS1);

                    DeleteTxFrames(pos);

                    if (pWlMan->Work.Mode == WL_CMDLABEL_MODE_PARENT) {
                        if (state > STA_CLASS1) {
                            wlMan->CamMan.NotSetTIM |= (1 << pos);
                            CAM_SetPowerMgtMode(pos, 0);
                            CAM_SetAwake(pos);
                            pTxDeAuthFrm = MakeDeAuthFrame(pCAM->macAdrs, MAN_STS_UNSPECIFIED, FALSE);
                            if ((u32)pTxDeAuthFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
                                pTxDeAuthFrm->FirmHeader.FrameId = FID_DEAUTH_IND;
                                TxManCtrlFrame((LPTXFRM)pTxDeAuthFrm);
                                i++;
                                continue;
                            } else {
                                MLME_IssueDeAuthIndication(pCAM->macAdrs, MAN_STS_UNSPECIFIED);
                            }
                        }
                    } else {
                        if (pos == pWlMan->Work.APCamAdrs) {
                            pTxDeAuthFrm = MakeDeAuthFrame(pCAM->macAdrs, MAN_STS_UNSPECIFIED, FALSE);
                            if ((u32)pTxDeAuthFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
                                pTxDeAuthFrm->FirmHeader.FrameId = FID_DEAUTH_IND;
                                TxManCtrlFrame((LPTXFRM)pTxDeAuthFrm);
                                i++;
                                continue;
                            } else {
                                WSetStaState(STA_CLASS1);

                                WClearAids();

                                MLME_IssueDeAuthIndication(pCAM->macAdrs, MAN_STS_UNSPECIFIED);
                            }
                        }
                    }
                }

                pCAM->state = CAM_FREE;
                pWlMan->CamMan.Count--;
            }

            i++;
        }
        if (i >= cnt) {
            break;
        }
    }
}

void CAM_Delete(u16 camAdrs)
{
    DeleteTxFrames(camAdrs);
    wlMan->Config.pCAM[camAdrs].state = CAM_FREE;
    wlMan->CamMan.Count--;
}

void InitializeCAM(void)
{
    LPCAM_ELEMENT pCAM = wlMan->Config.pCAM;
    u32 i, max;

    max = wlMan->Config.MaxStaNum;

    WLLIB_DmaClear16((u32)pCAM, sizeof(CAM_ELEMENT) * max); // ìÆÌæ NA

    WLLIB_DmaClear16((u32)&wlMan->CamMan, sizeof(CAM_MAN));

    pCAM[0].maxLifeTime = 0xFFFF;
    for (i = 1; i < max; i++) {
        pCAM[i].maxLifeTime = MAX_STA_LIFETIME;
    }

    CAM_InitElement(0, (u16 *)BC_ADRS);
    CAM_SetStaState(0, STA_CLASS3);
}

void InitCAM(void)
{
    LPCAM_ELEMENT pCAM = wlMan->Config.pCAM;
    LPCAM_MAN pCamMan = &wlMan->CamMan;
    u32 i, max;

    max = wlMan->Config.MaxStaNum;

    for (i = 1; i < max; i++) {
        WLLIB_DmaClear16((u32)&pCAM[i], sizeof(CAM_ELEMENT) - 2);
    }

    pCamMan->Count = 1;
    pCamMan->PowerMgtMode = 0;
    pCamMan->PowerState = 1;
    pCamMan->NotClass3 = 0xFFFE;
    pCamMan->ConnectSta = 0;
    pCamMan->NotSetTIM = 0;
    pCamMan->UseAidMap = 1;
}

static void CAM_InitElement(u32 camAdrs, u16 *pMacAdrs)
{
    LPCAM_ELEMENT pCAM = &wlMan->Config.pCAM[camAdrs];

    CAM_CheckCAM(camAdrs);

    if (pCAM->state == CAM_FREE) {
        wlMan->CamMan.Count++;
    }

    WLLIB_DmaClear16((u32)pCAM, sizeof(CAM_ELEMENT) - 2);

    wlMan->CamMan.NotSetTIM &= ~(1 << camAdrs);
    CAM_SetPowerMgtMode(camAdrs, 0);
    CAM_SetAwake(camAdrs);
    WSetMacAdrs1(pCAM->macAdrs, pMacAdrs);
    pCAM->lastSeqCtrl = 0xFFFF;
    pCAM->supRateSet = wlMan->Work.RateSet.Support;
    pCAM->lifeTime = pCAM->maxLifeTime;

    CAM_SetStaState(camAdrs, STA_CLASS1);
}
