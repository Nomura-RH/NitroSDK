#define __WLINTRTASK_C_
#define __INSYSROM__

#include "WlSys.h"
#include "TaskMan.h"

#include "WlLib.h"
#include "MA.h"
#include "TxCtrl.h"
#include "RxCtrl.h"
#include "MAC.h"
#include "WlIntr.h"
#include "Compati.h"
#include "MA.h"
#include "WlIntrTask.h"
#include "WlOpe.h"

void WlIntrTxBeaconTask(void)
{
    if (wlMan->Work.bUpdateGameInfo) {
        UpdateGameInfoElement();
    }

    if (wlMan->Config.BcnTxRxIndMsg) {
        MLME_IssueBeaconSendIndication();
    }
}

void WlIntrTxEndTask(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    LPTXQ pTxq;
    s32 i;

    for (i = 2; i >= 0; i--) {
        pTxq = (LPTXQ)&pTxCtrl->Txq[i];

        if (((*(vu16 *)(MREG_TXQ + i * 4) & TXQ_ENABLE) == 0) && (pTxq->Busy)) {

            if (pTxq->pFrm != NULL) {
                pTxq->pFrm->MacHeader.Tx.Status = pTxq->pMacFrm->MacHeader.Tx.Status;
                pTxq->pFrm->Dot11Header.FrameCtrl.Data = pTxq->pMacFrm->Dot11Header.FrameCtrl.Data;

                if (pTxq->pMacFrm->Dot11Header.FrameCtrl.Data & B_FC_WEP) {
#if (EMU_MACREG == 0)
                    if (wlMan->WlOperation & MAC_BUG_WEP) {
                        u16 *pIcv = (u16 *)(((u32)&pTxq->pMacFrm->Dot11Header + pTxq->pMacFrm->MacHeader.Tx.MPDU - 8 + 1) & 0xFFFFFFFE);

                        if ((pIcv[0] == 0) && (pIcv[1] == 0)) {
                            DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_WEP_ERR);

                            *(vu16 *)MREG_WEP_CONFIG = 0x0000;
                            *(vu16 *)MREG_WEP_CONFIG = 0x8000;

                            pWork->WepErrCount++;

                            if (CheckFrameTimeout(pTxq->pFrm)) {

                                pTxq->pFrm->MacHeader.Tx.Status = 2;
                                DbgPrint("[TO-3]");

                                pTxq->OutCount++;
                                pTxCtrl->TimeOutFrm++;
                                (*pTxq->pEndFunc)(pTxq->pFrm, 1);
                            } else {
                                pTxq->pFrm->MacHeader.Tx.rsv_RetryCount = 0;
                                *(vu16 *)(MREG_TXQ + i * 4) |= TXQ_ENABLE;
                            }

                            DbgWlPutchar(B_WL_DBG_ERRMSG, 'T');
                            DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_WEP_ERR);

                            continue;
                        }
                    }
#endif

                    pTxq->pFrm->MacHeader.Tx.rsv_RetryCount += LoadLow(&pTxq->pMacFrm->MacHeader.Tx.rsv_RetryCount);
                } else {
                    pTxq->pFrm->MacHeader.Tx.rsv_RetryCount = LoadLow(&pTxq->pMacFrm->MacHeader.Tx.rsv_RetryCount);
                }

                pTxq->OutCount++;
                (*pTxq->pEndFunc)(pTxq->pFrm, 2);
            } else {
                pTxq->OutCount++;
                (*pTxq->pEndFunc)((LPTXFRM)pTxq->pMacFrm, 3);
            }

            DbgClrDDO(DDO_WL_PS2, DDO_WL_PS2_TXQ0 << i);
            DbgClrDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_TXQ0 << i);
        }
    }
}

void WlIntrRxEndTask(void)
{
    LPRX_CTRL pRxCtrl = &wlMan->RxCtrl;
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPRXFRM_MAC pMFrm;
    LPRXFRM pFrm;
    u32 bnry, curr, length, status, i, bRelease, next_bnry;

    for (i = 0;; i++) {
        bnry = *(vu16 *)MREG_RXBUF_BNR;
        curr = pRxCtrl->wlCurr;

        if (bnry == curr) {
            break;
        }

        if (bnry >= (0x1000 + OFST_MREG_RXCOUNTER - 36) / 2) {
            WUpdateCounter();
        }

        pMFrm = (LPRXFRM_MAC)GetMacRxAdrs(bnry);

        next_bnry = *(u16 *)AdjustRingPointer(&pMFrm->MacHeader.Rx.NextBnry);

        if (pMFrm->MacHeader.Rx.Status == 0xFFFF) {

            *(vu16 *)MREG_RXBUF_BNR = next_bnry;
            continue;
        }

        length = *(u16 *)AdjustRingPointer(&pMFrm->MacHeader.Rx.MPDU);

        pFrm = TakeoutRxFrame(pMFrm, length);
        *(vu16 *)MREG_RXBUF_BNR = next_bnry;

        if ((u32)pFrm == HEAPBUF_NOT_ENOUGH_MEMORY) {
            if ((pMFrm->MacHeader.Rx.Status & RXSTS_FRAME_TYPE_MASK) == RXSTS_MP) {
                SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_RXMP);
            } else {
                SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_RXFRM);
            }
            continue;
        }

        if (wlMan->WlOperation & MAC_BUG_WEP) {
            if (pFrm->Dot11Header.FrameCtrl.Data & B_FC_WEP) {
                pRxCtrl->IcvOkCntFlag = 0;
            }
        }

        status = pFrm->MacHeader.Rx.Status;

        bRelease = TRUE;
        if (status & RXSTS_INCOMPLETE) {
            if ((pFrm->Dot11Header.FrameCtrl.Bit.MoreFrag == 1) || (pFrm->Dot11Header.SeqCtrl.Bit.FragNum != 0)) {
                bRelease = FALSE;
                MoveHeapBuf(&pHeapMan->TmpBuf, &pHeapMan->Defrag, CalcReqAdrsFromFrame(pFrm));
                AddTask(TASK_NORMAL_PRIORITY, DEFRAG_TASK_ID);
            }
        } else {
            switch (status & RXSTS_FRAME_TYPE_MASK) {
            case RXSTS_DATA:
                if ((pFrm->Dot11Header.FrameCtrl.Data & (BM_FC_PROTVER | BM_FC_TYPE)) == TYPE_DATA * B_FC_TYPE) {
                    bRelease = FALSE;
                    MoveHeapBuf(&pHeapMan->TmpBuf, &pHeapMan->RxData, CalcReqAdrsFromFrame(pFrm));
                    AddTask(TASK_NORMAL_PRIORITY, RXDATA_TASK_ID);
                }
                break;

            case RXSTS_BEACON:

                if (pFrm->Dot11Header.FrameCtrl.Data == FC_BEACON) {
                    DbgSetDDO(DDO_WL_PSPOLL, DDO_WL_PSPOLL_RXBEACON);
                    DbgSetDDO(DDO_WL_MP, DDO_WL_MP_RXBEACON);
                    RxBeaconFrame((LPBEACON_FRAME)pFrm);
                    DbgClrDDO(DDO_WL_PSPOLL, DDO_WL_PSPOLL_RXBEACON);
                    DbgClrDDO(DDO_WL_MP, DDO_WL_MP_RXBEACON);
                }
                break;

            case RXSTS_MAN:
                if ((pFrm->Dot11Header.FrameCtrl.Data & (BM_FC_PROTVER | BM_FC_TYPE)) == TYPE_MANAGEMENT * B_FC_TYPE) {
                    bRelease = FALSE;
                    MoveHeapBuf(&pHeapMan->TmpBuf, &pHeapMan->RxManCtrl, CalcReqAdrsFromFrame(pFrm));
                    AddTask(TASK_HIGH_PRIORITY, RXMANCTRL_TASK_ID);
                }
                break;

            case RXSTS_PSPOLL:
                if ((pFrm->Dot11Header.FrameCtrl.Data & 0xE7FF) == FC_PSPOLL) {
                    bRelease = FALSE;
                    MoveHeapBuf(&pHeapMan->TmpBuf, &pHeapMan->RxManCtrl, CalcReqAdrsFromFrame(pFrm));
                    AddTask(TASK_HIGH_PRIORITY, RXMANCTRL_TASK_ID);
                }
                break;

            case RXSTS_KEY:
            case RXSTS_NULLKEY:
                if ((pFrm->Dot11Header.FrameCtrl.Data & 0xE7BF) == FC_MPKEY) {
                    RxKeyDataFrame(pFrm);
                }
                break;

            case RXSTS_MP:
                if ((pFrm->Dot11Header.FrameCtrl.Data & 0xE7FF) == FC_MP) {

                    DbgSetDDO(DDO_WL_MP, DDO_WL_MP_RX_MP);
                    DbgSetDDO(DDO_WL_PS, DDO_WL_PS_RX_MP);
                    DbgSetDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_RXMP);

                    if (wlMan->Work.PowerState == (PWRSTS_PS >> 1)) {
                        *(vu16 *)MREG_SET_POWER = PWRSTS_PS;
                    }

                    wlMan->Counter.multiPoll.rxMp++;
                    bRelease = RxMpFrame(pFrm);

                    DbgClrDDO(DDO_WL_MP, DDO_WL_MP_RX_MP);
                    DbgClrDDO(DDO_WL_PS, DDO_WL_PS_RX_MP);
                    DbgClrDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_RXMP);
                }
                break;

            case RXSTS_MPACK:
                if ((pFrm->Dot11Header.FrameCtrl.Data & 0xE7FF) == FC_MPACK) {
                    DbgSetDDO(DDO_WL_PS, DDO_WL_PS_RX_MPACK);
                    DbgSetDDO(DDO_WL_MP, DDO_WL_MP_RX_MPACK);
                    DbgWlPutchar(B_WL_DBG_MP, (WL_DBG_RX_MP_ACK));
                    wlMan->Counter.multiPoll.rxMpAck++;
                    bRelease = RxMpAckFrame(pFrm);
                    DbgClrDDO(DDO_WL_PS, DDO_WL_PS_RX_MPACK);
                    DbgClrDDO(DDO_WL_MP, DDO_WL_MP_RX_MPACK);
                }
                break;

            default:
                break;
            }
        }

        if (bRelease) {
            ReleaseHeapBuf(&pHeapMan->TmpBuf, CalcReqAdrsFromFrame(pFrm));
        }

        if (wlMan->WlOperation & MAC_BUG_DIFF_CURR) {
            u16 *pClr = (u16 *)pMFrm;
            u32 j;

            for (j = 0; j < 7; j++) {
                if ((u32)pClr >= (MAC_MEM_BASE + MBUF_MAC_RX_END)) {
                    pClr = (u16 *)((u32)pClr - (u32)wlMan->Work.Ofst.RxBuf.Size);
                }
                *pClr++ = 0xFFFF;
            }
        }
    }
}

void WlIntrMpEndTask(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 cnt;

    if (pTxCtrl->Mp.Busy == 0) {
        return;
    }

    if (pTxCtrl->pMpEndInd->mpKey.bitmap != pTxCtrl->Mp.pMacFrm->MacHeader.Tx.Status2) {
        WlIntrRxEndTask();
    }

    cnt = LoadLow(&pTxCtrl->Mp.pMacFrm->MacHeader.Tx.rsv_RetryCount);
    if (cnt) {
        wlMan->Counter.multiPoll.txMp += cnt;
    } else {
        wlMan->Counter.multiPoll.txMp++;
    }

    pTxCtrl->Mp.OutCount++;

    pTxCtrl->pMpEndInd->mpKey.errBitmap = pTxCtrl->pMpEndInd->mpKey.bitmap ^ pTxCtrl->Mp.pMacFrm->MacHeader.Tx.Status2;

    pTxCtrl->RestBitmap = pTxCtrl->pMpEndInd->mpKey.bitmap;

    pTxCtrl->Mp.Busy = FALSE;

    if (wlMan->Work.TmpttPs) {
        WDisableTmpttPowerSave();
    }

    SendMessageToWmDirect(&wlMan->HeapMan.TmpBuf, pTxCtrl->pMpEndInd);
}

void SetParentTbttTxqTask(void)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;

    if ((*(vu16 *)MREG_DTIM_COUNT == 0) && (GetHeapBufCount(&pHeapMan->TxPri[QID_BROADCAST]) != 0)) {
        TxqOpen(TXQ_OPEN_TXQ2);
        TxqPri(QID_BROADCAST);
    } else {
        if ((wlMan->CamMan.PowerMgtMode & ~wlMan->CamMan.NotClass3) == 0) {
            TxqOpen(TXQ_OPEN_TXQ2);

            if (GetHeapBufCount(&pHeapMan->TxPri[QID_BROADCAST]) != 0) {
                TxqPri(QID_BROADCAST);
            }
        }

        TxqOpen(TXQ_OPEN_TXQ1 | TXQ_OPEN_TXQ0);
        if (GetHeapBufCount(&pHeapMan->TxPri[QID_MANCTRL]) != 0) {
            TxqPri(QID_MANCTRL);
        }
        if (GetHeapBufCount(&pHeapMan->TxPri[QID_DATA]) != 0) {
            TxqPri(QID_DATA);
        }
    }
}

static LPRXFRM TakeoutRxFrame(LPRXFRM_MAC pMFrm, u32 length) __attribute__((never_inline))
{
    LPRXPACKET pPacket;
    LPRXFRM pFrm;

    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_RXFRAME);

    pPacket = (LPRXPACKET)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, length + sizeof(WlCmdHeader) + sizeof(MAC_HEADER) + sizeof(FIRM_HEADER) + sizeof(WlCmdCfm));

    if ((u32)pPacket == HEAPBUF_NOT_ENOUGH_MEMORY) {
        DbgClrDDO(DDO_WL_INT, DDO_WL_INT_RXFRAME);
#if (WL_USE_EXT_HEAPBUF)
        DbgPrint("HO(%u)", length);
#else
        DbgPrint("HO(%u:%d)", length, OS_CheckHeap(wlMan->HeapMan.HeapInfo.func.os.id, wlMan->HeapMan.HeapInfo.func.os.heapHandle));
#endif
        return (LPRXFRM)HEAPBUF_NOT_ENOUGH_MEMORY;
    }
    pFrm = (LPRXFRM)&pPacket->frame;

    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_RXDMA);

    DMA_Read((void *)&pFrm->MacHeader, (void *)pMFrm, length + sizeof(MAC_HEADER));

    pFrm->FirmHeader.Length = length - 24;
    StoreHigh(&pFrm->MacHeader.Rx.Service_Rate, LoadLow(&pFrm->MacHeader.Rx.rsv_RSSI));

    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_RXDMA | DDO_WL_INT_RXFRAME);

    return pFrm;
}
