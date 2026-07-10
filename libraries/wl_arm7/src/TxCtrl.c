#define __TXCTRL_C_
#define __INSYSROM__

#include "WlSys.h"
#include "TaskMan.h"

#include "WlLib.h"
#include "TxCtrl.h"
#include "MAC.h"
#include "CAM.h"
#include "MA.h"
#include "Compati.h"
#include "WlOpe.h"
#include "WlIntrTask.h"

void TxqPri(u32 pri)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    LPTXQ pTxq = &pTxCtrl->Txq[pri];
    LPHEAPBUF_MAN pBufMan = &wlMan->HeapMan.TxPri[pri];
    WlMaDataReq *pTxReq, *pNextTxReq;
    LPTXFRM pTxFrm;
    LPTXFRM_MAC pMacTxFrm;
    u16 *pMREG;
    u32 x, fc, cam_adrs, adrs;

    if (GetHeapBufCount(pBufMan) == 0) {
        DbgPrint("TxQ[%u] count is 0.\r\n", pri);
        return;
    }

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (pTxq->Busy) {
        OS_EnableIrqMask(x);

        return;
    }

    pNextTxReq = (WlMaDataReq *)GetHeapBufHeadAdrs(pBufMan);
next_TxqFrm:
    if ((u32)pNextTxReq == HEAPBUF_HEAD_NONE) {
        OS_EnableIrqMask(x);
        return;
    }

    pTxReq = pNextTxReq;
    pNextTxReq = (WlMaDataReq *)GetHeapBufNextAdrs(pTxReq);
    pTxFrm = (LPTXFRM)&pTxReq->frame;
    cam_adrs = pTxFrm->FirmHeader.CamAdrs;

    if (CheckFrameTimeout(pTxFrm)) {
        pTxCtrl->TimeOutFrm++;

        pTxFrm->MacHeader.Tx.Status = 2;

        pTxq->OutCount++;
        (*pTxq->pEndFunc)(pTxFrm, FALSE);

        goto next_TxqFrm;
    }

    if ((pri == 0) || ((pri == 1) && (CAM_GetStaState(cam_adrs) == STA_CLASS3))) {
        if (!CAM_IsActive(cam_adrs)) {
            goto next_TxqFrm;
        }

        if (CAM_GetStaState(cam_adrs) != STA_CLASS3) {
            DbgPuts("[NotCl3]\r");
            pTxFrm->MacHeader.Tx.Status = 2;
            IssueMaDataConfirm(pBufMan, CalcReqAdrsFromFrame(pTxFrm));
            CAM_DecFrameCount(pTxFrm);

            goto next_TxqFrm;
        }
    }

    pTxq->Busy = TRUE;
    pTxq->InCount++;

    pTxq->pFrm = pTxFrm;
    pMacTxFrm = pTxq->pMacFrm;

    if (pWork->PowerState == (PWRSTS_PS >> 1)) {
        WSetPowerState(PWRSTS_ACT);
    }

    CopyTxFrmToMacBuf(pMacTxFrm, pTxReq);

    if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
        if (CAM_GetFrameCount(cam_adrs) > 1) {
            pMacTxFrm->Dot11Header.FrameCtrl.Data |= B_FC_MOREDATA;
        }
    }

    DbgSetDDO(DDO_WL_PS2, DDO_WL_PS2_TXQ0 << pri);
    DbgSetDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_TXQ0 << pri);
    fc = pTxFrm->Dot11Header.FrameCtrl.Data;
    pMREG = (u16 *)(MREG_TXQ + pri * 4);
    adrs = (u32)SetMacTxAdrs(pMacTxFrm);
    if ((fc & BM_FC_TYPE) == (TYPE_CONTROL * B_FC_TYPE)) {
        *pMREG = TXQ_ENABLE | TXQ_CTRL | (u16)adrs;
    } else if ((fc & (BM_FC_TYPE | BM_FC_SUBTYPE)) == FC_PRBRES) {
        *pMREG = TXQ_ENABLE | TXQ_PRBRES | (u16)adrs;
    } else {
        *pMREG = TXQ_ENABLE | (u16)adrs;
    }

    OS_EnableIrqMask(x);
}

void CopyTxFrmToMacBuf(LPTXFRM_MAC pMacTxFrm, WlMaDataReq *pTxReq)
{
    LPTXFRM pTxFrm = (LPTXFRM)&pTxReq->frame;

    if (pTxFrm->Dot11Header.FrameCtrl.Data & B_FC_WEP) {
        if (wlMan->Work.Mode == WL_CMDLABEL_MODE_HOTSPOT) {
            WUpdateCounter();
        }

        if (pTxReq->header.code == CONTINUOUS_DATA_MODE) {
            DMA_WepWriteHeaderData(pMacTxFrm, &pTxFrm->MacHeader, pTxFrm->Data.Body, pTxFrm->FirmHeader.Length);
        } else {
            DMA_WepWriteHeaderData(pMacTxFrm, &pTxFrm->MacHeader, pTxFrm->Data.Pointer, pTxFrm->FirmHeader.Length);
        }
        *(vu16 *)&pMacTxFrm->Body[0] = *(vu16 *)MREG_MSEQ16 + (*(vu16 *)MREG_MSEQ16 << 8);
        *(vu16 *)&pMacTxFrm->Body[2] = (*(vu16 *)MREG_MSEQ16 & 0x00FF) | ((u32)wlMan->Config.WepKeyId * B_IV_KEYID);

        if (wlMan->WlOperation & MAC_BUG_WEP) {
            u16 *pIcv = (u16 *)(((u32)&pMacTxFrm->Dot11Header + pTxFrm->MacHeader.Tx.MPDU - 8 + 1) & 0xFFFFFFFE);

            *pIcv++ = 0;
            *pIcv = 0;
        }
    } else {
        if (pTxReq->header.code == CONTINUOUS_DATA_MODE) {
            DMA_Write(pMacTxFrm, &pTxFrm->MacHeader, pTxFrm->FirmHeader.Length + sizeof(MAC_HEADER) + sizeof(DATA_HEADER));
        } else {
            DMA_WriteHeaderData(pMacTxFrm, &pTxFrm->MacHeader, pTxFrm->Data.Pointer, pTxFrm->FirmHeader.Length);
        }
    }

    if (wlMan->WlOperation & MAC_BUG_DESTROY_TXBUF) {
        u16 *pId = (u16 *)(((u32)&pMacTxFrm->Dot11Header + pTxFrm->MacHeader.Tx.MPDU - 1) & 0xFFFFFFFC);
        *pId++ = DESTROY_TXBUF_ID0;
        *pId = DESTROY_TXBUF_ID1;
    }
}

u32 CheckFrameTimeout(LPTXFRM pTxFrm)
{
    u16 delt, timeout;
    LPWORK_PARAM pWork = &wlMan->Work;

    timeout = pWork->FrameLifeTime * 8;
    switch (pTxFrm->Dot11Header.FrameCtrl.Bit.Type) {
    case TYPE_MANAGEMENT:
        if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
            switch (pTxFrm->Dot11Header.FrameCtrl.Bit.SubType) {
            case SUBTYPE_AUTH:
            case SUBTYPE_ASSRES:
            case SUBTYPE_REASSRES:
                timeout /= 8;
                break;
            }
        }
        break;

    default:
        timeout /= 8;
        break;
    }

    delt = pWork->IntervalCount - pTxFrm->FirmHeader.FrameTime;

    return delt > timeout;
}

void TxqEndData(LPTXFRM pFrm, u32 flag)
{
    LPHEAPBUF_MAN pBufMan = &wlMan->HeapMan.TxPri[QID_DATA];
    LPWORK_PARAM pWork = &wlMan->Work;
    WlCounter *pCounter = &wlMan->Counter;
    void *pReq = (void *)CalcReqAdrsFromFrame(pFrm);

    CAM_DecFrameCount(pFrm);

    if ((pFrm->MacHeader.Tx.Status & TXSTS_RETRY_ERR) == 0) {
        pCounter->tx.success++;
        if (pFrm->Dot11Header.FrameCtrl.Bit.ToDS) {
            if (pFrm->Dot11Header.Adrs3[0] & 0x0001) {
                pCounter->tx.multicast++;
            } else {
                pCounter->tx.unicast++;
            }
        } else {
            if (pFrm->Dot11Header.Adrs1[0] & 0x0001) {
                pCounter->tx.multicast++;
            } else {
                pCounter->tx.unicast++;
            }
        }
    } else {
        pCounter->tx.failed++;
    }
    if (pFrm->Dot11Header.FrameCtrl.Bit.WEP) {
        pCounter->tx.wep++;
    }
    pCounter += LoadHigh(&pFrm->MacHeader.Tx.rsv_RetryCount);

    IssueMaDataConfirm(pBufMan, pReq);

    wlMan->TxCtrl.Txq[QID_DATA].Busy = FALSE;

    if (CAM_GetPowerMgtMode(pFrm->FirmHeader.CamAdrs)) {
        if ((pFrm->Dot11Header.FrameCtrl.Data & B_FC_MOREDATA) == 0) {
            CAM_SetDoze(pFrm->FirmHeader.CamAdrs);
        }
    }

    if (flag) {
        if (GetHeapBufCount(pBufMan)) {
            TxqPri(QID_DATA);
        } else if ((pWork->Mode == WL_CMDLABEL_MODE_CHILD) || (pWork->Mode == WL_CMDLABEL_MODE_HOTSPOT)) {
            if ((pWork->STA == STA_CLASS3) && (pWork->PowerMgtMode) && (CAM_GetFrameCount(pWork->APCamAdrs) == 0) && (pWork->bExistTIM == TIM_FREE)) {
                WSetPowerState(PWRSTS_PS);
            }
        }
    }
}

void TxqEndManCtrl(LPTXFRM pFrm, u32 flag)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPMLME_MAN pMLME = &wlMan->MLME;
    WlCounter *pCounter = &wlMan->Counter;
    LPHEAPBUF_MAN pBufMan = &wlMan->HeapMan.TxPri[QID_MANCTRL];
    LPAUTH_FRAME pAuth;
    LPASSRES_FRAME pAssRes;
    LPDISASS_FRAME pDisAss;
    LPDEAUTH_FRAME pDeAuth;
    u32 bRelease = TRUE;
    u32 i, type, cam_adrs;

    cam_adrs = pFrm->FirmHeader.CamAdrs;

    if ((pFrm->MacHeader.Tx.Status & TXSTS_RETRY_ERR) == 0) {
        pCounter->tx.success++;
        if (pFrm->Dot11Header.Adrs1[0] & 0x0001) {
            pCounter->tx.multicast++;
        } else {
            pCounter->tx.unicast++;
        }

        if (CAM_GetPowerMgtMode(cam_adrs)) {
            if ((pFrm->Dot11Header.FrameCtrl.Data & B_FC_MOREDATA) == 0) {
                CAM_SetDoze(cam_adrs);
            }
        }
    } else {
        pCounter->tx.failed++;
    }
    if (pFrm->Dot11Header.FrameCtrl.Bit.WEP) {
        pCounter->tx.wep++;
    }
    pCounter->tx.retry += LoadLow(&pFrm->MacHeader.Tx.rsv_RetryCount);

    type = pFrm->Dot11Header.FrameCtrl.Data & (BM_FC_SUBTYPE | BM_FC_TYPE);
    switch (type) {
    case FC_AUTH:

        if (cam_adrs == 0) {
            break;
        }

        if ((pFrm->MacHeader.Tx.Status & TXSTS_RETRY_ERR) == 0) {
            pAuth = (LPAUTH_FRAME)pFrm;

            if ((pAuth->Body.AlgoType == WL_CMDLABEL_AUTH_OPEN_SYSTEM) && (pAuth->Body.SeqNum == 2) && (pAuth->Body.StatusCode == WL_CMDLABEL_STS_SUCCESSFUL)) {
                CAM_SetStaState(cam_adrs, STA_CLASS2);

                MLME_IssueAuthIndication(pAuth->Dot11Header.DA, pAuth->Body.AlgoType);
            } else if ((pAuth->Body.AlgoType == WL_CMDLABEL_AUTH_SHARED_KEY) && (pAuth->Body.SeqNum == 4) && (pAuth->Body.StatusCode == WL_CMDLABEL_STS_SUCCESSFUL)) {
                CAM_SetStaState(cam_adrs, STA_CLASS2);

                MLME_IssueAuthIndication(pAuth->Dot11Header.DA, pAuth->Body.AlgoType);
            }
        }

        else {
        }
        break;

    case FC_ASSRES:
    case FC_REASSRES:

        if (cam_adrs == 0) {
            break;
        }

        pAssRes = (LPASSRES_FRAME)pFrm;

        if ((pFrm->MacHeader.Tx.Status & TXSTS_RETRY_ERR) == 0) {
            if ((pAssRes->Body.StatusCode == WL_CMDLABEL_STS_SUCCESSFUL) && (CAM_GetStaState(cam_adrs) == STA_CLASS2)) {
                CAM_SetStaState(cam_adrs, STA_CLASS3);

                if (type == FC_ASSRES) {
                    MLME_IssueAssIndication(pAssRes->Dot11Header.DA, pAssRes->Body.AID, (LPSSID_ELEMENT)((u32)&pFrm->Dot11Header + pFrm->MacHeader.Tx.MPDU));
                } else {
                    MLME_IssueReAssIndication(pAssRes->Dot11Header.DA, pAssRes->Body.AID, (LPSSID_ELEMENT)((u32)&pFrm->Dot11Header + pFrm->MacHeader.Tx.MPDU));
                }
            }
        }

        else {
            CAM_ReleaseAID(cam_adrs);

            pDeAuth = MakeDeAuthFrame(pAssRes->Dot11Header.DA, MAN_RSN_UNSPECIFIED, FALSE);
            if ((u32)pDeAuth != HEAPBUF_NOT_ENOUGH_MEMORY) {
                pDeAuth->FirmHeader.FrameId = FID_DEAUTH_IND2;
                if (flag) {
                    TxManCtrlFrame((LPTXFRM)pDeAuth);
                } else {
                    SetManCtrlFrame((LPTXFRM)pDeAuth);
                }
            }
        }
        break;

    case FC_DISASS:

        if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
            if (cam_adrs != 0) {
                if (CAM_GetStaState(cam_adrs) > STA_CLASS2) {
                    CAM_SetStaState(cam_adrs, STA_CLASS2);
                }
            } else if (pFrm->Dot11Header.Adrs1[0] & 0x0001) {
                for (i = 1; i < wlMan->Config.MaxStaNum; i++) {
                    if (CAM_GetStaState(i) > STA_CLASS2) {
                        CAM_SetStaState(i, STA_CLASS2);
                    }
                }
            }
        } else {
            if (pWork->STA > STA_CLASS2) {
                WSetStaState(STA_CLASS2);
                WClearAids();
            }
        }

        if ((pMLME->State == MLME_STATE_DISASS_ING) && (pFrm == pMLME->Work.DisAss.pTxFrm)) {
            pDisAss = (LPDISASS_FRAME)pFrm;

            if ((pFrm->MacHeader.Tx.Status & TXSTS_RETRY_ERR) == 0) {
                pMLME->pCfm.DisAss->resultCode = WL_CMDRES_SUCCESS;
            } else {
                pMLME->pCfm.DisAss->resultCode = WL_CMDRES_FAILURE;
            }

            pMLME->State = MLME_STATE_IDLE;

            IssueMlmeConfirm();
        }
        break;

    case FC_DEAUTH:

        if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
            if (cam_adrs != 0) {
                if (CAM_GetStaState(cam_adrs) > STA_CLASS1) {
                    CAM_SetStaState(cam_adrs, STA_CLASS1);
                }
            } else if (pFrm->Dot11Header.Adrs1[0] & 0x0001) {
                for (i = 1; i < wlMan->Config.MaxStaNum; i++) {
                    if (CAM_GetStaState(i) > STA_CLASS1) {
                        CAM_SetStaState(i, STA_CLASS1);
                    }
                }
            }
        } else {
            if (pWork->STA > STA_CLASS1) {
                WSetStaState(STA_CLASS1);
                WClearAids();
            }
        }

        if ((pMLME->State == MLME_STATE_DEAUTH_ING) && (pFrm == pMLME->Work.DeAuth.pTxFrm)) {
            pDeAuth = (LPDEAUTH_FRAME)pFrm;

            if ((pFrm->MacHeader.Tx.Status & TXSTS_RETRY_ERR) == 0) {
                pMLME->pCfm.DeAuth->resultCode = WL_CMDRES_SUCCESS;
            } else {
                pMLME->pCfm.DeAuth->resultCode = WL_CMDRES_FAILURE;
            }

            pMLME->State = MLME_STATE_IDLE;

            IssueMlmeConfirm();
        }

        if (pFrm->FirmHeader.FrameId == FID_DEAUTH_IND) {
            DbgPrint("In FID_DEAUTH_IND[%u]\r\n", cam_adrs);

            if (cam_adrs != 0) {
                wlMan->CamMan.NotSetTIM &= ~(1 << cam_adrs);

                CAM_Delete(cam_adrs);
            }

            MLME_IssueDeAuthIndication(pFrm->Dot11Header.Adrs1, MAN_STS_UNSPECIFIED);
        } else if (pFrm->FirmHeader.FrameId == FID_DEAUTH_IND2) {
            DbgPrint("In FID_DEAUTH_IND2\r\n");

            pDeAuth = (LPDEAUTH_FRAME)pFrm;

            MLME_IssueDeAuthIndication(pFrm->Dot11Header.Adrs1, pDeAuth->Body.ReasonCode);
        }
        break;

    default:
        break;
    }

    if (bRelease) {
        CAM_DecFrameCount(pFrm);
        ReleaseHeapBuf(pBufMan, CalcReqAdrsFromFrame(pFrm));
    }

    wlMan->TxCtrl.Txq[QID_MANCTRL].Busy = FALSE;

    if (flag && GetHeapBufCount(pBufMan)) {
        TxqPri(QID_MANCTRL);
    }
}

void TxqEndPsPoll(LPTXFRM pFrm, u32 flag)
{
#pragma unused(flag)
    WlCounter *pCounter = &wlMan->Counter;
    LPTXFRM_MAC pMFrm = (LPTXFRM_MAC)pFrm;

    pCounter->tx.retry += LoadLow(&pMFrm->MacHeader.Tx.rsv_RetryCount);

    if ((pMFrm->MacHeader.Tx.Status & TXSTS_RETRY_ERR) == 0) {
        pCounter->tx.success++;
        pCounter->tx.unicast++;
    } else {
        pCounter->tx.failed++;
    }

    wlMan->TxCtrl.Txq[QID_PSPOLL].Busy = FALSE;

    DbgClrDDO(DDO_WL_PSPOLL, DDO_WL_PSPOLL_BUSY);
}

void TxqEndBroadCast(LPTXFRM pFrm, u32 flag)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPHEAPBUF_MAN pBufMan = &pHeapMan->TxPri[QID_BROADCAST];
    LPTXFRM_MAC pMacFrm;

    wlMan->Counter.tx.multicast++;

    if (pFrm->Dot11Header.FrameCtrl.Bit.Type == 0) {
        CAM_IncFrameCount(pFrm);
        MoveHeapBuf(pBufMan, &pHeapMan->TxPri[QID_MANCTRL], CalcReqAdrsFromFrame(pFrm));
        TxqEndManCtrl(pFrm, FALSE);
    } else {
        IssueMaDataConfirm(pBufMan, CalcReqAdrsFromFrame(pFrm));
    }

    wlMan->TxCtrl.Txq[QID_BROADCAST].Busy = FALSE;

    pMacFrm = wlMan->TxCtrl.Txq[QID_BROADCAST].pMacFrm;
    if (pMacFrm->Dot11Header.FrameCtrl.Bit.MoreData == 0) {
        TxqClose(TXQ_CLOSE_TXQ2);
        TxqOpen(TXQ_OPEN_TXQ1 | TXQ_OPEN_TXQ0);

        if (flag) {
            if (GetHeapBufCount(&pHeapMan->TxPri[QID_MANCTRL]) != 0) {
                TxqPri(QID_MANCTRL);
            }
            if (GetHeapBufCount(&pHeapMan->TxPri[QID_DATA]) != 0) {
                TxqPri(QID_DATA);
            }
        }
    }

    if (GetHeapBufCount(pBufMan)) {
        if (flag) {
            TxqPri(QID_BROADCAST);
        }
    } else {
        CAM_ClrTIMElementBitmap(0);
    }
}

void TxEndKeyData(LPTXQ pTxq)
{
    u32 retrycnt = LoadLow(&pTxq->pMacFrm->MacHeader.Tx.rsv_RetryCount);
    WlCounter *pCounter = &wlMan->Counter;

    if (retrycnt == 0) {
        DbgWlPutchar(B_WL_DBG_MP, (WL_DBG_TX_MP_NULL));
        pCounter->multiPoll.txNull++;
    } else {
        DbgWlPutchar(B_WL_DBG_MP, (WL_DBG_TX_MP_KEY));
        pCounter->multiPoll.txKey += retrycnt;
    }

    pTxq->OutCount++;

    pTxq->Busy = FALSE;
}

void ClearTxKeyData(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 x;

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (wlMan->Config.NullKeyRes == 0) {
        WClearKSID(NULL);
    }

    *(vu16 *)MREG_TXQ_RESET = TXQ_RST_KEYIN | TXQ_RST_KEYOUT;

    pTxCtrl->Key[0].Busy = FALSE;
    pTxCtrl->Key[1].Busy = FALSE;

    OS_EnableIrqMask(x);
}

void ClearTxMp(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 x;

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    *(vu16 *)MREG_TXQ_RESET = TXQ_RST_MP;

    if (pTxCtrl->Mp.Busy) {
        WlIntrMpEndTask();
    }

    OS_EnableIrqMask(x);
}

void ClearTxData(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 x;

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    switch (wlMan->Work.Mode) {
    case WL_CMDLABEL_MODE_PARENT:
        *(vu16 *)MREG_TXQ_RESET = TXQ_RST_TXQ0 | TXQ_RST_TXQ2;

        if (pTxCtrl->Txq[QID_BROADCAST].Busy) {
            ClearQueuedPri(QID_BROADCAST);
        }

        MessageDeleteTx(QID_BROADCAST, TRUE);
        break;

    default:
        *(vu16 *)MREG_TXQ_RESET = TXQ_RST_TXQ0;
        break;
    }

    if (pTxCtrl->Txq[QID_DATA].Busy) {
        ClearQueuedPri(QID_DATA);
    }

    MessageDeleteTx(QID_DATA, TRUE);

    OS_EnableIrqMask(x);
}

void ClearQueuedPri(u32 pri)
{
    LPTXQ pTxq = &wlMan->TxCtrl.Txq[pri];

    if (!pTxq->Busy) {
        return;
    }

    if (pTxq->pMacFrm->MacHeader.Tx.Status == 0) {
        pTxq->pFrm->MacHeader.Tx.Status = 2;
    } else {
        pTxq->pFrm->MacHeader.Tx.Status = pTxq->pMacFrm->MacHeader.Tx.Status;
    }

    (*pTxq->pEndFunc)(pTxq->pFrm, FALSE);
}

static const u16 Pri2QBit[] = { 0x01, 0x04, 0x08 };
void ResetTxqPri(u32 pri)
{
    u32 x;
    LPTXQ pTxq = &wlMan->TxCtrl.Txq[pri];

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    *(vu16 *)MREG_TXQ_RESET = Pri2QBit[pri];

    if (pTxq->Busy) {
        if ((pTxq->pFrm->Dot11Header.FrameCtrl.Data & B_FC_WEP) == 0) {
            pTxq->pFrm->MacHeader.Tx.rsv_RetryCount = pTxq->pMacFrm->MacHeader.Tx.rsv_RetryCount;
        }

        pTxq->pFrm->Dot11Header.SeqCtrl.Data = pTxq->pMacFrm->Dot11Header.SeqCtrl.Data;

        DbgClrDDO(DDO_WL_PS2, DDO_WL_PS2_TXQ0 << pri);
        DbgClrDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_TXQ0 << pri);
    }

    OS_EnableIrqMask(x);
}

void DeleteTxFrames(u32 camAdrs)
{
    WlMaDataReq *pReq, *pNextReq;
    LPTXFRM pFrm;
    u32 bTask = FALSE;
    u32 i;

    if (CAM_GetFrameCount(camAdrs) == 0) {
        return;
    }

    for (i = 0; i < 3; i++) {
        pReq = (WlMaDataReq *)GetHeapBufHeadAdrs(&wlMan->HeapMan.TxPri[i]);
        if ((u32)pReq == HEAPBUF_HEAD_NONE) {
            continue;
        }

        do {
            pNextReq = (WlMaDataReq *)GetHeapBufNextAdrs(pReq);
            pFrm = (LPTXFRM)&pReq->frame;

            if (pFrm->FirmHeader.CamAdrs == camAdrs) {
                if ((i == 1) || (pFrm == wlMan->TxCtrl.Txq[i].pFrm)) {
                    CAM_DecFrameCount(pFrm);
                    pFrm->FirmHeader.CamAdrs = 0;
                    CAM_IncFrameCount(pFrm);
                } else {
                    pFrm->MacHeader.Tx.Status = 2;
                    CAM_DecFrameCount(pFrm);
                    IssueMaDataConfirm(&wlMan->HeapMan.TxPri[i], pReq);
                    if (bTask == FALSE) {
                        bTask = TRUE;
                    }
                }
            }

            pReq = pNextReq;

        } while ((u32)pReq != HEAPBUF_HEAD_NONE);
    }
}

void DeleteTxFrameByAdrs(u16 *pMacAdrs)
{
    LPCAM_ELEMENT pCAM = (LPCAM_ELEMENT)wlMan->Config.pCAM;
    u32 i;

    if (pMacAdrs[0] & 0x0001) {
        for (i = 1, pCAM++; i < wlMan->Config.MaxStaNum; i++, pCAM++) {
            DeleteTxFrames(i);
        }
    }

    else {
        i = CAM_Search(pMacAdrs);
        if (i != CAM_NOT_FOUND) {
            DeleteTxFrames(i);
        }

        if ((wlMan->Config.Mode == WL_CMDLABEL_MODE_PARENT) && (CAM_GetStaState(i) == STA_CLASS3)) {
            CAM_SetStaState(i, STA_CLASS1);
            ClearTxKeyData();
        }
    }
}

void DeleteAllTxFrames(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;

    switch (wlMan->Work.Mode) {
    case WL_CMDLABEL_MODE_PARENT:
        MessageDeleteTx(0, TRUE);
        MessageDeleteTx(1, FALSE);
        MessageDeleteTx(2, TRUE);
        if (pTxCtrl->Mp.Busy) {
            DbgPrint("Release MPEndInd\r\n");
            pTxCtrl->Mp.Busy = 0;
            pTxCtrl->Mp.InCount--;

            ReleaseHeapBuf(&wlMan->HeapMan.TmpBuf, pTxCtrl->pMpEndInd);
        }
        break;

    case WL_CMDLABEL_MODE_CHILD:
    case WL_CMDLABEL_MODE_HOTSPOT:
        MessageDeleteTx(0, TRUE);
        MessageDeleteTx(1, FALSE);
        MessageDeleteTx(2, FALSE);
        break;

    default:
        MessageDeleteTx(0, FALSE);
        MessageDeleteTx(1, FALSE);
        MessageDeleteTx(2, FALSE);
        break;
    }
}

void MessageDeleteTx(u32 pri, u32 bMsg)
{
    WlMaDataReq *pReq, *pNextReq;
    LPTXFRM pFrm;

    pReq = (WlMaDataReq *)GetHeapBufHeadAdrs(&wlMan->HeapMan.TxPri[pri]);
    if ((u32)pReq == HEAPBUF_HEAD_NONE) {
        return;
    }

    do {
        pNextReq = (WlMaDataReq *)GetHeapBufNextAdrs(pReq);
        pFrm = (LPTXFRM)&pReq->frame;

        if (pri != QID_BROADCAST) {
            CAM_DecFrameCount(pFrm);
        }

        pFrm->MacHeader.Tx.Status = 2;

        if (bMsg) {
            IssueMaDataConfirm(&wlMan->HeapMan.TxPri[pri], pReq);
        }

        pReq = pNextReq;

    } while ((u32)pReq != HEAPBUF_HEAD_NONE);
}

void TxManCtrlFrame(LPTXFRM pFrm)
{
    SetManCtrlFrame(pFrm);
    TxqPri(QID_MANCTRL);
}

void SetManCtrlFrame(LPTXFRM pFrm)
{
    pFrm->FirmHeader.CamAdrs = CAM_Search(pFrm->Dot11Header.Adrs1);
    if (pFrm->FirmHeader.CamAdrs == CAM_NOT_FOUND) {
        pFrm->FirmHeader.CamAdrs = 0;
    }

    pFrm->FirmHeader.FrameTime = wlMan->Work.IntervalCount;

    if (pFrm->Dot11Header.FrameCtrl.Data & B_FC_WEP) {
        pFrm->MacHeader.Tx.MPDU += 8;
    }

    CAM_IncFrameCount(pFrm);
    MoveHeapBuf(&wlMan->HeapMan.TmpBuf, &wlMan->HeapMan.TxPri[QID_MANCTRL], CalcReqAdrsFromFrame(pFrm));
}

void TxPsPollFrame(void)
{
    LPTXQ pTxq = &wlMan->TxCtrl.Txq[QID_PSPOLL];

    pTxq->InCount++;
    DbgSetDDO(DDO_WL_PSPOLL, DDO_WL_PSPOLL_BUSY);

    if (pTxq->Busy) {
        pTxq->pMacFrm->MacHeader.Tx.rsv_RetryCount = 0;
    } else {
        pTxq->Busy = TRUE;

        pTxq->pMacFrm->MacHeader.Tx.Status = 0;
        pTxq->pMacFrm->MacHeader.Tx.rsv_RetryCount = 0;
        pTxq->pMacFrm->MacHeader.Tx.Service_Rate = WCalcManRate();

        *(vu16 *)(MREG_TXQ + QID_PSPOLL * 4) = TXQ_ENABLE | (u16)SetMacTxAdrs(pTxq->pMacFrm);
    }
}

void StartBeaconFrame(void)
{
    LPTXQ pTxq = &wlMan->TxCtrl.Beacon;

    pTxq->Busy = TRUE;

    *(vu16 *)MREG_BCN_ADRS = TXQ_ENABLE | SetMacTxAdrs(pTxq->pMacFrm);
}

void StopBeaconFrame(void)
{
    LPTXQ pTxq = &wlMan->TxCtrl.Beacon;

    *(vu16 *)MREG_BCN_ADRS = 0;

    pTxq->Busy = FALSE;
}

void MakeBeaconFrame(void)
{
    LPTXFRM_MAC pFrm = wlMan->TxCtrl.Beacon.pMacFrm;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPBEACON_BODY pBody;
    u8 *pBuf;
    u8 *p;
    u32 i, vtsf;
    u16 *p16;

    p16 = (u16 *)&pFrm->MacHeader.Tx;
    *p16++ = 0;
    *p16++ = 0;
    *p16++ = 0;
    *p16++ = 0;
    *p16++ = WCalcManRate();

    pFrm->Dot11Header.FrameCtrl.Data = FC_BEACON;
    pFrm->Dot11Header.DurationID = 0;
    WSetMacAdrs3(pFrm->Dot11Header.Adrs1, (u16 *)BC_ADRS, pConfig->MacAdrs, pConfig->MacAdrs);
    pFrm->Dot11Header.SeqCtrl.Data = 0;

    pBody = (LPBEACON_BODY)pFrm->Body;

    pBody->TimeStamp[0] = 0;
    pBody->TimeStamp[1] = 0;
    pBody->BeaconInterval = pWork->BeaconPeriod;
    pBody->CapaInfo.Data = pWork->CapaInfo;

    pBuf = pBody->Buf;
    if (pConfig->BeaconType == WL_CMDLABEL_INCLUDE_SSID) {
        pWork->Ofst.Beacon.SSID = (u16)((u32)pBuf - (u32)pFrm->Body);
        WL_WriteByte(pBuf++, ID_SSID_ELEMENT);
        WL_WriteByte(pBuf++, pWork->SSIDLength);
        for (i = 0; i < pWork->SSIDLength; i++) {
            WL_WriteByte(pBuf++, WL_ReadByte(&pWork->SSID[i]));
        }
        WL_WriteByte(&pBuf[-i - 1], i);
    } else {
        pWork->Ofst.Beacon.SSID = 0;
    }

    pBuf += SetSupRateSet(pBuf);

    WL_WriteByte(pBuf++, ID_DS_PARAM_ELEMENT);
    WL_WriteByte(pBuf++, 1);
    WL_WriteByte(pBuf++, pWork->CurrChannel);

    pWork->Ofst.Beacon.TIM = (u16)((u32)pBuf - (u32)pBody);
    *(u16 *)MREG_BCN_DTIM_OFST = pWork->Ofst.Beacon.TIM + 2;
    WL_WriteByte(pBuf++, ID_TIM_ELEMENT);
    WL_WriteByte(pBuf++, 3 + 2);
    WL_WriteByte(pBuf++, 0);
    WL_WriteByte(pBuf++, pWork->DTIMPeriod);
    WL_WriteByte(pBuf++, 0);
    WL_WriteByte(pBuf++, 0);
    WL_WriteByte(pBuf++, 0);

    pWork->Ofst.Beacon.GameInfo = (u16)((u32)pBuf - (u32)pBody);
    pWork->GameInfoAlign = pWork->Ofst.Beacon.GameInfo & 1;
    WL_WriteByte(pBuf++, ID_GAME_INFO_ELEMENT);
    WL_WriteByte(pBuf++, 3 + 1 + 4 + pWork->GameInfoLength);
    WL_WriteByte(pBuf++, GAME_INFO_OUI_0);
    WL_WriteByte(pBuf++, GAME_INFO_OUI_1);
    WL_WriteByte(pBuf++, GAME_INFO_OUI_2);
    WL_WriteByte(pBuf++, GAME_INFO_SUBTYPE);
    if (pWork->PowerMgtMode == WL_CMDLABEL_PMG_PS) {
        WL_WriteByte(pBuf++, pConfig->ActiveZone);
        WL_WriteByte(pBuf++, (pConfig->ActiveZone >> 8));
    } else {
        WL_WriteByte(pBuf++, 0xFF);
        WL_WriteByte(pBuf++, 0xFF);
    }
    vtsf = *(u16 *)V_TSF;
    WL_WriteByte(pBuf++, vtsf & 0x00FF);
    WL_WriteByte(pBuf++, vtsf >> 8);
    p = (u8 *)pWork->GameInfoAdrs;
    for (i = 0; i < pWork->GameInfoLength; i++) {
        WL_WriteByte(pBuf++, WL_ReadByte(p));
        p++;
    }
    if (pWork->GameInfoAlign) {
        p = (u8 *)((u32)pWork->GameInfoAdrs + pWork->GameInfoLength - 1);
        for (i = 0; i < pWork->GameInfoLength; i++, p--) {
            WL_WriteByte(p + 1, WL_ReadByte(p));
        }
    }

    if (wlMan->WlOperation & MAC_BUG_DESTROY_TXBUF) {
        u16 *pId = (u16 *)(((u32)pBuf + 3) & 0xFFFFFFFC);
        pId[0] = DESTROY_TXBUF_ID0;
        pId[1] = DESTROY_TXBUF_ID1;
    }

    pWork->bUpdateGameInfo = FALSE;

    pFrm->MacHeader.Tx.MPDU = 24 + (u32)pBuf - (u32)pBody + 4;
}

void UpdateGameInfoElement(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPGAME_INFO_ELEMENT pGMIF = (LPGAME_INFO_ELEMENT)((u32)wlMan->TxCtrl.Beacon.pMacFrm->Body + pWork->Ofst.Beacon.GameInfo);

    if (pWork->GameInfoLength != 0) {
        if ((u32)pWork->GameInfoAlign & 1) {
            DMA_Write((void *)((u32)pGMIF->GameInfo - 1), pWork->GameInfoAdrs, pWork->GameInfoLength + 2);
            WL_WriteByte(&pGMIF->VTSF[1], *(vu16 *)V_TSF >> 8);
        } else {
            DMA_Write(pGMIF->GameInfo, pWork->GameInfoAdrs, pWork->GameInfoLength + 1);
        }
    }

    wlMan->TxCtrl.Beacon.pMacFrm->MacHeader.Tx.MPDU = 24 + pWork->Ofst.Beacon.GameInfo + 2 + 3 + 1 + 4 + pWork->GameInfoLength + 4;

    WL_WriteByte(&pGMIF->Length, 3 + 1 + 4 + pWork->GameInfoLength);

    if (wlMan->WlOperation & MAC_BUG_DESTROY_TXBUF) {
        u16 *pId = (u16 *)(((u32)pGMIF + 1 + 1 + 3 + 1 + 4 + pWork->GameInfoLength + 3) & 0xFFFFFFFC);
        pId[0] = DESTROY_TXBUF_ID0;
        pId[1] = DESTROY_TXBUF_ID1;
    }

    pWork->bUpdateGameInfo = FALSE;
}

static u32 IsEnableManagement(void)
{
    if (GetHeapBufCount(&wlMan->HeapMan.TxPri[QID_MANCTRL]) < (MAX_MANAGEMENT_QUEUE - wlMan->CamMan.ConnectSta)) {
        return TRUE;
    }
    DbgPrint("Ovrflow:");

    return FALSE;
}

LPDISASS_FRAME MakeDisAssFrame(u16 *pDA, u16 reasonCode)
{
    WlMaDataReq *pReq;
    LPDISASS_FRAME pFrm;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_DISASS));

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, DISASS_PACKET_SIZE);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPDISASS_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPDISASS_FRAME)&pReq->frame;

    InitManHeader((LPTXFRM)pFrm, pDA);

    pFrm->Body.ReasonCode = reasonCode;

    pFrm->FirmHeader.Length = 2;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_DISASS;

    return pFrm;
}

LPASSREQ_FRAME MakeAssReqFrame(u16 *pDA)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    WlMaDataReq *pReq;
    LPASSREQ_FRAME pFrm;
    u32 ofst;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_ASSREQ));

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, ASSREQ_PACKET_SIZE);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPASSREQ_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPASSREQ_FRAME)&pReq->frame;

    InitManHeader((LPTXFRM)pFrm, pDA);

    pFrm->Body.CapaInfo.Data = pWork->CapaInfo;
    pFrm->Body.ListenInterval = pWork->ListenInterval;
    ofst = SetSSIDElement(&pFrm->Body.Buf[0]);
    ofst += SetSupRateSet(&pFrm->Body.Buf[ofst]);

    pFrm->FirmHeader.Length = ofst + 2 + 2;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_ASSREQ;

    return pFrm;
}

#ifndef SDK_SMALL_BUILD_WL
LPREASSREQ_FRAME MakeReAssReqFrame(u16 *pDA)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    WlMaDataReq *pReq;
    LPREASSREQ_FRAME pFrm;
    u32 ofst;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_REASSREQ));

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, REASSREQ_PACKET_SIZE);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPREASSREQ_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPREASSREQ_FRAME)&pReq->frame;

    InitManHeader((LPTXFRM)pFrm, pDA);

    pFrm->Body.CapaInfo.Data = pWork->CapaInfo;
    pFrm->Body.ListenInterval = pWork->ListenInterval;
    WSetMacAdrs1(pFrm->Body.CurrAPMacAdrs, pWork->LinkAdrs);
    ofst = SetSSIDElement(&pFrm->Body.Buf[0]);
    ofst += SetSupRateSet(&pFrm->Body.Buf[ofst]);

    pFrm->FirmHeader.Length = ofst + 2 + 2 + 6;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_REASSREQ;

    return pFrm;
}
#endif

LPASSRES_FRAME MakeAssResFrame(u16 camAdrs, u16 statusCode, LPSSID_ELEMENT pSSID)
{
    WlMaDataReq *pReq;
    LPASSRES_FRAME pFrm;
    u32 ofst, i, aid;
    u8 *p;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_ASSRES));

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, ASSRES_PACKET_SIZE + 34);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPASSRES_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPASSRES_FRAME)&pReq->frame;

    if (statusCode == MAN_STS_SUCCESSFUL) {
        aid = CAM_AllocateAID(camAdrs);

        if (aid == 0) {
            statusCode = MAN_STS_NO_ENTRY;
        }
    } else {
        aid = 0;
    }

    InitManHeader((LPTXFRM)pFrm, CAM_GetMacAdrs(camAdrs));

    pFrm->Body.CapaInfo.Data = wlMan->Work.CapaInfo;
    pFrm->Body.StatusCode = statusCode;
    pFrm->Body.AID = aid;
    if (aid) {
        pFrm->Body.AID |= 0xC000;
    }
    ofst = SetSupRateSet(&pFrm->Body.Buf[0]);

    pFrm->FirmHeader.Length = ofst + 2 + 2 + 2;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_ASSRES;

    p = (u8 *)((u32)&pFrm->Dot11Header + pFrm->MacHeader.Tx.MPDU);
    if (pSSID != NULL) {
        u32 len = WL_ReadByte(&pSSID->Length);
        WL_WriteByte(p++, WL_ReadByte(&pSSID->ID));
        WL_WriteByte(p++, len);
        for (i = 0; i < len; i++, p++) {
            WL_WriteByte(p, WL_ReadByte(&pSSID->SSID[i]));
        }
    } else {
        WL_WriteByte(p++, ID_SSID_ELEMENT);
        WL_WriteByte(p, 0);
    }

    return pFrm;
}

LPREASSRES_FRAME MakeReAssResFrame(u16 camAdrs, u16 statusCode, LPSSID_ELEMENT pSSID)
{
    WlMaDataReq *pReq;
    LPREASSRES_FRAME pFrm;
    u32 ofst, i, aid;
    u8 *p;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_REASSRES));

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, ASSRES_PACKET_SIZE + 34);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPREASSRES_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPREASSRES_FRAME)&pReq->frame;

    if (statusCode == MAN_STS_SUCCESSFUL) {
        aid = CAM_AllocateAID(camAdrs);

        if (aid == 0) {
            statusCode = MAN_STS_NO_ENTRY;
        }
    } else {
        aid = 0;
    }

    InitManHeader((LPTXFRM)pFrm, CAM_GetMacAdrs(camAdrs));

    pFrm->Body.CapaInfo.Data = wlMan->Work.CapaInfo;
    pFrm->Body.StatusCode = statusCode;
    pFrm->Body.AID = aid | 0xC000;
    ofst = SetSupRateSet(&pFrm->Body.Buf[0]);

    pFrm->FirmHeader.Length = ofst + 2 + 2 + 2;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_REASSRES;

    p = (u8 *)((u32)&pFrm->Dot11Header + pFrm->MacHeader.Tx.MPDU);
    if (pSSID != NULL) {
        u32 len = WL_ReadByte(&pSSID->Length);
        WL_WriteByte(p, WL_ReadByte(&pSSID->ID));
        p++;
        WL_WriteByte(p, len);
        p++;
        for (i = 0; i < len; i++) {
            WL_WriteByte(p, WL_ReadByte(&pSSID->SSID[i]));
            p++;
        }
    } else {
        WL_WriteByte(p, ID_SSID_ELEMENT);
        p++;
        WL_WriteByte(p, 0);
    }

    return pFrm;
}

LPPRBREQ_FRAME MakeProbeReqFrame(u16 *pDA)
{
    WlMaDataReq *pReq;
    LPPRBREQ_FRAME pFrm;
    u32 ofst;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_PRBREQ));

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, PRBREQ_PACKET_SIZE);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPPRBREQ_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPPRBREQ_FRAME)&pReq->frame;

    InitManHeader((LPTXFRM)pFrm, pDA);

    ofst = SetSSIDElement(&pFrm->Body.Buf[0]);
    ofst += SetSupRateSet(&pFrm->Body.Buf[ofst]);

    pFrm->FirmHeader.Length = ofst;
    pFrm->MacHeader.Tx.MPDU = ofst + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_PRBREQ;

    return pFrm;
}

LPPRBRES_FRAME MakeProbeResFrame(u16 *pDA)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    WlMaDataReq *pReq;
    LPPRBRES_FRAME pFrm;
    u32 ofst;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_PRBRES));

    if (!IsEnableManagement()) {
        DbgPrint("PrbRes\r\n");
        return HEAPBUF_NOT_ENOUGH_MEMORY;
    }

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, PRBRES_PACKET_SIZE + pWork->GameInfoLength);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPPRBRES_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPPRBRES_FRAME)&pReq->frame;

    InitManHeader((LPTXFRM)pFrm, pDA);

    pFrm->Body.BeaconInterval = pWork->BeaconPeriod;
    pFrm->Body.CapaInfo.Data = pWork->CapaInfo;
    ofst = SetSSIDElement(&pFrm->Body.Buf[0]);
    ofst += SetSupRateSet(&pFrm->Body.Buf[ofst]);
    ofst += SetDSParamSet(&pFrm->Body.Buf[ofst]);
    ofst += SetGameInfoElement(&pFrm->Body.Buf[ofst]);

    pFrm->FirmHeader.Length = ofst + 8 + 2 + 2;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_PRBRES;

    return pFrm;
}

LPAUTH_FRAME MakeAuthFrame(u16 *pDA, u16 txtLen, u32 bCheck)
{
    WlMaDataReq *pReq;
    LPAUTH_FRAME pFrm;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_AUTH));

    if (bCheck && !IsEnableManagement()) {
        DbgPrint("Auth\r\n");
        return HEAPBUF_NOT_ENOUGH_MEMORY;
    }

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, AUTH_PACKET_SIZE + txtLen + 2);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPAUTH_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPAUTH_FRAME)&pReq->frame;

    InitManHeader((LPTXFRM)pFrm, pDA);

    if (txtLen != 0) {
        WL_WriteByte(&pFrm->Body.ChallengeText.ID, ID_CHALLENGE_ELEMENT);
        WL_WriteByte(&pFrm->Body.ChallengeText.Length, txtLen);
        txtLen += 2;
    }

    pFrm->FirmHeader.Length = 2 + 2 + 2 + txtLen;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_AUTH;

    return pFrm;
}

LPDEAUTH_FRAME MakeDeAuthFrame(u16 *pDA, u16 reasonCode, u32 bCheck)
{
    WlMaDataReq *pReq;
    LPDEAUTH_FRAME pFrm;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_DEAUTH));

    if (bCheck && !IsEnableManagement()) {
        DbgPrint("DeAuth\r\n");
        return HEAPBUF_NOT_ENOUGH_MEMORY;
    }

    pReq = (WlMaDataReq *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, DEAUTH_PACKET_SIZE);
    if ((u32)pReq == HEAPBUF_NOT_ENOUGH_MEMORY) {
        SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_MANCTRL);
        return (LPDEAUTH_FRAME)pReq;
    }
    pReq->header.code = CONTINUOUS_DATA_MODE;
    pFrm = (LPDEAUTH_FRAME)&pReq->frame;

    InitManHeader((LPTXFRM)pFrm, pDA);

    pFrm->Body.ReasonCode = reasonCode;

    pFrm->FirmHeader.Length = 2;
    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length + sizeof(MAN_HEADER) + 4;
    pFrm->Dot11Header.FrameCtrl.Data = FC_DEAUTH;

    return pFrm;
}

void MakePsPollFrame(u16 aid)
{
    LPTXFRM_MAC pMFrm = wlMan->TxCtrl.Txq[QID_PSPOLL].pMacFrm;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_TX_PSPOLL));

    pMFrm->MacHeader.Tx.Status = 0;
    pMFrm->MacHeader.Tx.Status2 = 0;
    pMFrm->MacHeader.Tx.rsv_RetryCount = 0;
    pMFrm->MacHeader.Tx.MPDU = 28 - 6 - 2;

    pMFrm->Dot11Header.FrameCtrl.Data = FC_PSPOLL;
    pMFrm->Dot11Header.DurationID = 0xC000 | aid;
    WSetMacAdrs2(pMFrm->Dot11Header.Adrs1, wlMan->Work.BSSID, wlMan->Config.MacAdrs);
}

void InitManHeader(LPTXFRM pFrm, u16 *pDA)
{
    MI_CpuClear16(pFrm, MAN_ALL_HEADER_SIZE);

    pFrm->MacHeader.Tx.Service_Rate = WCalcManRate();

    WSetMacAdrs3(pFrm->Dot11Header.Adrs1, pDA, wlMan->Config.MacAdrs, wlMan->Work.BSSID);
}

u32 IsExistManFrame(u16 *pDA, u16 frameCtrl)
{
    WlMaDataReq *pReq;
    LPTXFRM pFrm;

    pReq = (WlMaDataReq *)GetHeapBufHeadAdrs(&wlMan->HeapMan.TxPri[1]);

    while ((u32)pReq != HEAPBUF_HEAD_NONE) {
        pFrm = (LPTXFRM)&pReq->frame;

        if ((pFrm->Dot11Header.FrameCtrl.Data == frameCtrl) && MatchMacAdrs(pFrm->Dot11Header.Adrs1, pDA)) {
            DbgPrint("[MatchMan:%04x%04x%04x:%04x]\r\n", SwapEndianWord(pDA[0]), SwapEndianWord(pDA[1]), SwapEndianWord(pDA[2]), frameCtrl);
            return TRUE;
        }

        pReq = (WlMaDataReq *)GetHeapBufNextAdrs(pReq);
    }

    return FALSE;
}

static u32 SetSSIDElement(u8 *pBuf)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    u32 len = 0, ssid_len = pWork->SSIDLength;
    u32 i;

    WL_WriteByte(&pBuf[len], ID_SSID_ELEMENT);
    len++;

    WL_WriteByte(&pBuf[len], ssid_len);
    len++;

    for (i = 0; i < ssid_len; i++) {
        WL_WriteByte(&pBuf[len], WL_ReadByte(&pWork->SSID[i]));
        len++;
    }

    return len;
}

static u32 SetSupRateSet(u8 *pBuf)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    u8 *pBase = pBuf;
    u32 len = 0;
    u32 i;

    WL_WriteByte(&pBuf[0], ID_SUPRATE_ELEMENT);
    len++;

    len++;

    for (i = 0; i < sizeof(pWork->RateSet.Basic) * 8; i++) {
        if (pWork->RateSet.Support & (0x01 << i)) {
            if (pWork->RateSet.Basic & (0x01 << i)) {
                WL_WriteByte(&pBuf[len], (RateBit2Element[i] | 0x80));
            } else {
                WL_WriteByte(&pBuf[len], RateBit2Element[i]);
            }
            len++;
        }
    }

    WL_WriteByte(&pBase[1], (len - 2));

    return len;
}

static u32 SetDSParamSet(u8 *pBuf)
{
    WL_WriteByte(&pBuf[0], ID_DS_PARAM_ELEMENT);

    WL_WriteByte(&pBuf[1], 1);

    WL_WriteByte(&pBuf[2], wlMan->Work.CurrChannel);

    return 3;
}

static u32 SetGameInfoElement(u8 *pBuf)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    u32 i, j, vtsf;
    u8 *p;

    i = 0;

    WL_WriteByte(&pBuf[i], ID_GAME_INFO_ELEMENT);
    i++;

    WL_WriteByte(&pBuf[i], wlMan->Work.GameInfoLength + 3 + 1 + 4);
    i++;

    WL_WriteByte(&pBuf[i], GAME_INFO_OUI_0);
    i++;
    WL_WriteByte(&pBuf[i], GAME_INFO_OUI_1);
    i++;
    WL_WriteByte(&pBuf[i], GAME_INFO_OUI_2);
    i++;

    WL_WriteByte(&pBuf[i], GAME_INFO_SUBTYPE);
    i++;

    WL_WriteByte(&pBuf[i], pConfig->ActiveZone);
    i++;
    WL_WriteByte(&pBuf[i], pConfig->ActiveZone >> 8);
    i++;

    vtsf = *(vu16 *)V_TSF;
    WL_WriteByte(&pBuf[i], vtsf & 0x00FF);
    i++;
    WL_WriteByte(&pBuf[i], vtsf >> 8);
    i++;

    if (pWork->GameInfoLength != 0) {
        p = (u8 *)pWork->GameInfoAdrs;
        if (pWork->GameInfoAlign & 1) {
            p++;
        }

        for (j = 0; j < pWork->GameInfoLength; j++) {
            WL_WriteByte(&pBuf[i], WL_ReadByte(p));
            i++;
            p++;
        }
    }

    return i;
}

void InitTxCtrl(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;

    WLLIB_DmaClear32((u32)pTxCtrl, sizeof(TX_CTRL));

    pTxCtrl->Txq[0].Busy = FALSE;
    pTxCtrl->Txq[0].pFrm = (LPTXFRM)NULL;

    pTxCtrl->Txq[1].Busy = FALSE;
    pTxCtrl->Txq[1].pFrm = (LPTXFRM)NULL;

    pTxCtrl->Txq[2].Busy = FALSE;
    pTxCtrl->Txq[2].pFrm = (LPTXFRM)NULL;

    pTxCtrl->BkKeyIn = 0xFFFF;
    pTxCtrl->BkKeyOut = 0xFFFF;

    switch (pWork->Mode) {
    case WL_CMDLABEL_MODE_TEST:
        pTxCtrl->Txq[QID_DATA].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_TEST_PRI0);
        pTxCtrl->Txq[QID_MANCTRL].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_TEST_PRI1);
        pTxCtrl->Txq[QID_PSPOLL].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_TEST_PRI2);
        pTxCtrl->Txq[QID_DATA].pEndFunc = TxqEndData;
        pTxCtrl->Txq[QID_MANCTRL].pEndFunc = TxqEndManCtrl;
        pTxCtrl->Txq[QID_PSPOLL].pEndFunc = TxqEndPsPoll;

        *(u16 *)(MAC_MEM_BASE + MBUF_TEST_PRI1 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_TEST_PRI1 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_TEST_PRI0 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_TEST_PRI0 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_TEST_RX - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_TEST_RX - 2) = DESTROY_TXBUF_ID1;

        pWork->FrameCtrl = FC_DATA;

        TxqOpen(TXQ_OPEN_TXQ0);
        break;

    case WL_CMDLABEL_MODE_PARENT:
        pTxCtrl->Txq[QID_DATA].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_PARENT_PRI0);
        pTxCtrl->Txq[QID_MANCTRL].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_PARENT_PRI1);
        pTxCtrl->Txq[QID_BROADCAST].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_PARENT_PRI2);
        pTxCtrl->Txq[QID_DATA].pEndFunc = TxqEndData;
        pTxCtrl->Txq[QID_MANCTRL].pEndFunc = TxqEndManCtrl;
        pTxCtrl->Txq[QID_BROADCAST].pEndFunc = TxqEndBroadCast;

        pTxCtrl->Beacon.pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_PARENT_BEACON);
        pTxCtrl->Mp.pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_PARENT_MP);

        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_BEACON - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_BEACON - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_PRI2 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_PRI2 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_PRI1 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_PRI1 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_PRI0 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_PRI0 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_RX - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_PARENT_RX - 2) = DESTROY_TXBUF_ID1;

        pWork->FrameCtrl = FC_DATA | B_FC_FROMDS;

        pTxCtrl->Beacon.pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_PARENT_BEACON);
        MakeBeaconFrame();
        break;

    case WL_CMDLABEL_MODE_CHILD:
        pTxCtrl->Txq[QID_DATA].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_CHILD_PRI0);
        pTxCtrl->Txq[QID_MANCTRL].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_CHILD_PRI1);
        pTxCtrl->Txq[QID_PSPOLL].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_CHILD_PRI2);
        pTxCtrl->Txq[QID_DATA].pEndFunc = TxqEndData;
        pTxCtrl->Txq[QID_MANCTRL].pEndFunc = TxqEndManCtrl;
        pTxCtrl->Txq[QID_PSPOLL].pEndFunc = TxqEndPsPoll;

        pTxCtrl->Key[0].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_CHILD_CDATA0);
        pTxCtrl->Key[1].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_CHILD_CDATA1);

        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_CDATA1 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_CDATA1 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_PRI2 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_PRI2 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_PRI1 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_PRI1 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_PRI0 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_PRI0 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_RX - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_CHILD_RX - 2) = DESTROY_TXBUF_ID1;

        pWork->FrameCtrl = FC_DATA | B_FC_TODS;

        TxqOpen(TXQ_OPEN_TXQ0 | TXQ_OPEN_TXQ1 | TXQ_OPEN_TXQ2);

        break;

    case WL_CMDLABEL_MODE_HOTSPOT:
        pTxCtrl->Txq[QID_DATA].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_HOTSPOT_PRI0);
        pTxCtrl->Txq[QID_MANCTRL].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_HOTSPOT_PRI1);
        pTxCtrl->Txq[QID_PSPOLL].pMacFrm = (LPTXFRM_MAC)(MAC_MEM_BASE + MBUF_HOTSPOT_PRI2);
        pTxCtrl->Txq[QID_DATA].pEndFunc = TxqEndData;
        pTxCtrl->Txq[QID_MANCTRL].pEndFunc = TxqEndManCtrl;
        pTxCtrl->Txq[QID_PSPOLL].pEndFunc = TxqEndPsPoll;

#if (MAC_BUG_DESTROY_TXBUF)
        *(u16 *)(MAC_MEM_BASE + MBUF_HOTSPOT_PRI1 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_HOTSPOT_PRI1 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_HOTSPOT_PRI0 - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_HOTSPOT_PRI0 - 2) = DESTROY_TXBUF_ID1;
        *(u16 *)(MAC_MEM_BASE + MBUF_HOTSPOT_RX - 4) = DESTROY_TXBUF_ID0;
        *(u16 *)(MAC_MEM_BASE + MBUF_HOTSPOT_RX - 2) = DESTROY_TXBUF_ID1;
#endif

        pWork->FrameCtrl = FC_DATA | B_FC_TODS;

        TxqOpen(TXQ_OPEN_TXQ0 | TXQ_OPEN_TXQ1 | TXQ_OPEN_TXQ2);
        break;
    }

    if (pConfig->WepMode != WL_CMDLABEL_WEP_NO) {
        pWork->FrameCtrl |= B_FC_WEP;
    }
}
