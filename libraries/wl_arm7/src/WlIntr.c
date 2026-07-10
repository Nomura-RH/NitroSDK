#define __WLINTR_C_
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
#include "WlOpe.h"

void WlIntr(void)
{
    u32 isr;

#ifdef SDK_TEG
    if (wlMan->bUnLocked) {
        OS_LockCartridge(wlMan->lockID);
    }
#endif

    while (1) {
        isr = *(vu16 *)MREG_ISR & *(vu16 *)MREG_IMR;
        if (isr == 0) {
            break;
        }

        if (isr & MISR_START_TX) {
            WlIntrStartTx();
        }
        if (isr & MISR_START_RX) {
            WlIntrStartRx();
        }
        if (isr & MISR_PRE_TBTT) {
            WlIntrPreTbtt();
        }
        if (isr & MISR_TBTT) {
            WlIntrTbtt();
        }
        if (isr & MISR_ACTEND) {
            WlIntrActEnd();
        }

        if (isr & MISR_RF_WAKEUP) {
            WlIntrRfWakeup();
        }
        if (isr & MISR_TXERR) {
            WlIntrTxErr();
        }
        if (isr & MISR_RXCNTUP) {
            WlIntrRxCntup();
        }
        if (isr & MISR_RXEND) {
            WlIntrRxEnd();
        }
        if (isr & MISR_CNT_OVF) {
            WlIntrCntOvf();
        }
        if (isr & MISR_TXEND) {
            WlIntrTxEnd();
        }
        if (isr & MISR_MPEND) {
            WlIntrMpEnd(1);
        }
    }

    DbgClrDDO(DDO_WL_TASK, DDO_WL_TASK_INT_WL);

#ifdef SDK_TEG
    if (wlMan->bUnLocked) {
        OS_UnLockCartridge(wlMan->lockID);
    }
#endif

    OS_SetIrqCheckFlag(OS_IE_WIRELESS);
}

static void WlIntrPreTbtt(void) __attribute__((never_inline))
{
    LPWORK_PARAM pWork = &wlMan->Work;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_PRE_TBTT;
#else
    *(vu16 *)MREG_ISR = MISR_PRE_TBTT;
#endif

    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_PRETBTT));
    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_PRETBTT);

    if ((pWork->STA == STA_CLASS3) && (pWork->BeaconLostTh != 0) && (pWork->CurrListenInterval == pWork->ListenInterval)) {
        if (++pWork->BeaconLostCnt > pWork->BeaconLostTh) {
            DbgSetDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_BEACON_LOST);
            DbgSetDDO(DDO_WL_PS2, DDO_WL_PS2_BCN_LOST);

            pWork->BeaconLostCnt = 0;

            AddTask(TASK_HIGH_PRIORITY, MLME_BEACON_LOST_TASK_ID);

            DbgClrDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_BEACON_LOST);
            DbgClrDDO(DDO_WL_PS2, DDO_WL_PS2_BCN_LOST);
        }
    }

    pWork->PowerState = 1;

    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_PRETBTT);
}

static void WlIntrTbtt(void) __attribute__((never_inline))
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u8 *p;
    u32 txq, i, vtsf;
    u32 bWakeUp;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_TBTT;
#else
    *(vu16 *)MREG_ISR = MISR_TBTT;
#endif

    DbgSetDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_TBTT);
    DbgSetDDO(DDO_WL_PS, DDO_WL_PS_TBTT | DDO_WL_PS_ACTZONE | DDO_WL_PS_FIRM_ACT);
    DbgSetDDO(DDO_WL_PS2, DDO_WL_PS2_TBTT | DDO_WL_PS2_ACTZONE);
    DbgSetDDO(DDO_WL_PSPOLL, DDO_WL_PSPOLL_TBTT);
    DbgSetDDO(DDO_WL_MP, DDO_WL_MP_ACTZONE);
    DbgSetDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_ACTZONE);
    DbgSetDDO(DDO_WL_CONNECT, DDO_WL_CONNECT_ACTZONE);
    DbgWlPutchar((B_WL_DBG_INT | B_WL_DBG_POWERSAVE), (WL_DBG_INT_TBTT));
    DbgClrDDO(DDO_WL_PSPOLL, DDO_WL_PSPOLL_TBTT);
    DbgClrDDO(DDO_WL_PS, DDO_WL_PS_TBTT);
    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_TBTT);
    DbgClrDDO(DDO_WL_PS2, DDO_WL_PS2_TBTT);

#if (EMU_MACREG)
    TxqClose(0xFFFF);
#endif

    switch (pWork->Mode) {
    case WL_CMDLABEL_MODE_PARENT:

        p = (u8 *)((u32)pTxCtrl->Beacon.pMacFrm->Body + pWork->Ofst.Beacon.GameInfo + 2 + 3 + 1 + 2);
        vtsf = *(vu16 *)V_TSF;
        WL_WriteByte(p++, vtsf & 0x00FF);
        WL_WriteByte(p, vtsf >> 8);

        if (pWork->PowerMgtMode == WL_CMDLABEL_PMG_PS) {
            *(vu16 *)MREG_ACTZONE = (u32)pConfig->ActiveZone + *(vu16 *)MREG_ACTZONE + 1;
        }

        CAM_InitAllPowerState();

        txq = *(vu16 *)MREG_TXQ_CURR;
        if ((txq & (TXQ_CURR_BEACON | TXQ_CURR_TXQ2)) || ((txq & (TXQ_CURR_TXQ1 | TXQ_CURR_MP)) == TXQ_CURR_MP)) {
            pTxCtrl->Flag &= ~TXQ_FLAG_SUSPEND_TBTT;
            SetParentTbttTxq();
        } else {
            pTxCtrl->Flag |= TXQ_FLAG_SUSPEND_TBTT;
        }
        break;

    case WL_CMDLABEL_MODE_CHILD:
        if (pWork->bSynchro) {
            *(vu16 *)MREG_ACTZONE = (u32)pConfig->ActiveZone + *(vu16 *)MREG_ACTZONE + 1;
        } else {
            *(vu16 *)MREG_ACTZONE = 0xFFFF;
        }

        if (pWork->bFirstTbtt == 2) {
            WSetPowerState(PWRSTS_ACT);
        }

    case WL_CMDLABEL_MODE_HOTSPOT:

        if (pWork->STA != STA_CLASS3) {
            bWakeUp = TRUE;
        } else {
            bWakeUp = FALSE;

            if (pWork->CurrListenInterval == 1) {
                bWakeUp = TRUE;
            }

            if (pWork->RxDtims) {
                if ((pWork->DTIMCount == 1) || ((pWork->DTIMCount == 0) && (pWork->DTIMPeriod == 1))) {
                    bWakeUp = TRUE;
                }
            }
        }

        if (bWakeUp) {
            DbgSetDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_TBTT_ACTIVE);
            *(vu16 *)MREG_WAKEUP_CTRL |= WAKEUP_CTRL_WAKEUP_TBTT;
            DbgClrDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_TBTT_ACTIVE);
        } else {
            DbgSetDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_TBTT_PS);
            *(vu16 *)MREG_WAKEUP_CTRL &= ~WAKEUP_CTRL_WAKEUP_TBTT;
            DbgClrDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_TBTT_PS);
        }

        if (*(vu16 *)MREG_MP_TMPTT > 10) {
            *(vu16 *)MREG_MP_POWER_SEQ = 0;
        }

        if (--pWork->CurrListenInterval == 0) {
            pWork->CurrListenInterval = pWork->ListenInterval;
        }

        if (pWork->DTIMCount-- == 0) {
            pWork->DTIMCount = pWork->DTIMPeriod - 1;
        }

        for (i = 0; i < 2; i++) {
            LPTXQ pTxq = &pTxCtrl->Txq[i];

            if (pTxq->Busy && (pTxq->pFrm->MacHeader.Tx.Status == 0)) {
                if (CheckFrameTimeout(pTxq->pFrm)) {
                    ResetTxqPri(i);

                    DbgPrint("[TO-2]");
                    pTxq->pMacFrm->MacHeader.Tx.Status = 2;

                    AddTask(TASK_CRITICAL_PRIORITY, INTR_TXEND_TASK_ID);

                    pTxCtrl->TimeOutFrm++;
                }
            }
        }

        TxqOpen(TXQ_OPEN_TXQ2 | TXQ_OPEN_TXQ1 | TXQ_OPEN_TXQ0);
        break;
    }

    DbgClrDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_TBTT);
    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_TBTT);
}

static void WlIntrActEnd(void) __attribute__((never_inline))
{
    LPWORK_PARAM pWork = &wlMan->Work;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_ACTEND;
#else
    *(vu16 *)MREG_ISR = MISR_ACTEND;
#endif

    TxqClose(TXQ_OPEN_TXQ2 | TXQ_OPEN_TXQ1 | TXQ_OPEN_TXQ0);

    if (pWork->bFirstTbtt == 1) {
        pWork->bFirstTbtt = 2;
    } else if (pWork->bFirstTbtt == 2) {
        pWork->bFirstTbtt = 0;
    } else if ((pWork->Mode == WL_CMDLABEL_MODE_CHILD) && (pWork->STA != STA_CLASS3)) {
        *(vu16 *)MREG_MP_POWER_SEQ = 0;
    }
}

static void WlIntrRfWakeup(void) __attribute__((never_inline))
{
#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_RF_WAKEUP;
#else
    *(vu16 *)MREG_ISR = MISR_RF_WAKEUP;
#endif

    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_RFWAKEUP);
    DbgSetDDO(DDO_WL_PS, DDO_WL_PS_RF_WAKEUP);
    DbgSetDDO(DDO_WL_PS2, DDO_WL_PS2_RF_WAKEUP);
    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_RFWAKEUP));
    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_RFWAKEUP);
    DbgClrDDO(DDO_WL_PS, DDO_WL_PS_RF_WAKEUP);
    DbgSetDDO(DDO_WL_PS2, DDO_WL_PS2_RF_WAKEUP);
}

static void WlIntrCntOvf(void) __attribute__((never_inline))
{
    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_CNTOVF));
    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_CNTOVF);

    WUpdateCounter();

    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_CNTOVF);

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_CNT_OVF;
#else
    *(vu16 *)MREG_CNTOVF_ISR = 0xFFFF;
    *(vu16 *)MREG_ISR = MISR_CNT_OVF;
#endif
}

static void WlIntrTxErr(void) __attribute__((never_inline))
{
    LPTXFRM_MAC pMFrm;
    u32 i;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_TXERR;
#else
    *(vu16 *)MREG_ISR = MISR_TXERR;
#endif

    if (wlMan->Config.Diversity != WL_CMDLABEL_DIVERSITY_OFF) {
        if ((*(vu16 *)MREG_SIGNAL_STATE & (SIGNAL_STATE_CCA || SIGNAL_STATE_TXPE)) == 0) {
            WChangeAntenna();
        }
    }

    if (wlMan->WlOperation & MAC_BUG_WEP) {
        LPWORK_PARAM pWork = &wlMan->Work;
        LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;

        for (i = 0; i < 3; i++) {
            if (pTxCtrl->Txq[i].Busy) {
                pMFrm = (LPTXFRM_MAC)pTxCtrl->Txq[i].pMacFrm;

                if ((pMFrm->Dot11Header.FrameCtrl.Data & B_FC_WEP) && (LoadLow(&pMFrm->MacHeader.Tx.rsv_RetryCount) != 0)) {
                    u16 *pIcv = (u16 *)(((u32)&pMFrm->Dot11Header + pMFrm->MacHeader.Tx.MPDU - 8 + 1) & 0xFFFFFFFE);

                    if ((pIcv[0] == 0) && (pIcv[1] == 0)) {
                        pMFrm->MacHeader.Tx.rsv_RetryCount = 0;

                        *(vu16 *)MREG_WEP_CONFIG = 0x0000;
                        *(vu16 *)MREG_WEP_CONFIG = 0x8000;

                        pWork->WepErrCount++;

                        DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_WEP_ERR);
                        DbgWlPutchar(B_WL_DBG_ERRMSG, 'E');
                        DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_WEP_ERR);
                    }
                }
            }
        }
    }

    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_TXERR));
}

static void WlIntrRxCntup(void) __attribute__((never_inline))
{
    LPRX_CTRL pRxCtrl = &wlMan->RxCtrl;
    LPTXQ pTxq = wlMan->TxCtrl.Txq;
    u32 isr;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_RXCNTUP;
#else
    *(vu16 *)MREG_ISR = MISR_RXCNTUP;
#endif

    isr = *(vu16 *)MREG_CNTUP_ISR;

    if (wlMan->WlOperation & MAC_BUG_WEP) {
        if (isr & RXCNTSTS_ICV_OK) {
            u16 map;

            DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_ICVOK));

            map = *(vu16 *)MREG_TXQ_MAP;
            if ((((map & TXQ_NBLOCK_TXQ0) == 0) || (pTxq[0].Busy == 0)) && (((map & TXQ_NBLOCK_TXQ1) == 0) || (pTxq[1].Busy == 0)) && (((map & TXQ_NBLOCK_TXQ2) == 0) || (pTxq[2].Busy == 0)) && ((*(vu16 *)MREG_SIGNAL_STATE & SIGNAL_STATE_CCA) == 0)) {
                *(vu16 *)MREG_WEP_CONFIG = 0x0000;
                *(vu16 *)MREG_WEP_CONFIG = 0x8000;

                pRxCtrl->IcvOkCntFlag = 0;

            }

            else if (pRxCtrl->IcvOkCntFlag++ > 12) {
                DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_WEP_ERR);

                pRxCtrl->IcvOkCntFlag = 0;

                *(vu16 *)MREG_WEP_CONFIG = 0x0000;
                *(vu16 *)MREG_WEP_CONFIG = 0x8000;

                wlMan->Work.WepErrCount++;

                DbgWlPutchar(B_WL_DBG_ERRMSG, 'R');
            }
        }
    }

    if (wlMan->WlOperation & MAC_BUG_DIFF_CURR) {
        if (isr & (RXCNTSTS_FCS_ERR | RXCNTSTS_FCS_OK)) {
            u16 curr = *(vu16 *)MREG_RXBUF_CUR;
            if ((curr >= (*(vu16 *)MREG_RXBUF_END - (MAC_MEM_BASE & 0xFFFF)) / 2) || (curr < (*(vu16 *)MREG_RXBUF_STR - (MAC_MEM_BASE & 0xFFFF)) / 2)) {
                *(vu16 *)MREG_RXBUF_WCUR = *(vu16 *)MREG_RXBUF_BNR;
                *(u16 *)MREG_MDP_CONFIG = 0x8001;

                DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_CURROVRRING);
                DbgPrint("CURR over ring.(%x)\r\n", curr);
                DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_CURROVRRING);
            }

            WCheckTxBuf();
        }
    }
}

static void WlIntrTxEnd(void) __attribute__((never_inline))
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 txFrm;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_TXEND;
#else
    *(vu16 *)MREG_ISR = MISR_TXEND;
#endif

    if (wlMan->Work.STA == STA_IDLE_TEST2) {
        void IntrCarrierSuppresionSignal(void);
        IntrCarrierSuppresionSignal();
        return;
    }

    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_TXEND));
    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_TXEND);

    txFrm = *(vu16 *)MREG_TXSTATUS & TXSTS_FRAME_TYPE_MASK;
    switch (txFrm) {
    case TXSTS_BEACON:
        if (pTxCtrl->Flag & TXQ_FLAG_SUSPEND_TBTT) {
            SetParentTbttTxq();
        }

        wlMan->Counter.tx.beacon++;

        AddTask(TASK_CRITICAL_PRIORITY, INTR_TXBEACON_TASK_ID);

        break;

    case TXSTS_MP:
        DbgWlPutchar(B_WL_DBG_MP, (WL_DBG_TX_MP));

        if (pTxCtrl->RetryLimit <= (u16)LoadLow(&pTxCtrl->Mp.pMacFrm->MacHeader.Tx.rsv_RetryCount)) {
            *(vu16 *)MREG_TXQ_CLOSE = TXQ_CLOSE_MP;

            pTxCtrl->MpBlkCnt++;

            DbgSetDDO(DDO_WL_MP, DDO_WL_MP_LAST_MP);
        }

        pTxCtrl->pMpEndInd->mpKey.txCount++;
        break;

    case TXSTS_MPACK:
        DbgWlPutchar(B_WL_DBG_MP, (WL_DBG_TX_MP_ACK));

        *(u16 *)MREG_DCF_LAST_ADRS0 = 0xFFFF;
        *(u16 *)MREG_DCF_LAST_ADRS1 = 0xFFFF;
        *(u16 *)MREG_LAST_RX_ADRS0 = 0xFFFF;
        *(u16 *)MREG_LAST_RX_ADRS1 = 0xFFFF;

        if (pTxCtrl->Mp.Busy) {
            if (pTxCtrl->Mp.pMacFrm->Dot11Header.SeqCtrl.Data == 0xFFFF) {
                u16 retry = pTxCtrl->Mp.pMacFrm->MacHeader.Tx.rsv_RetryCount;
                if (retry != 0) {
                    pTxCtrl->Mp.pMacFrm->MacHeader.Tx.rsv_RetryCount = 0;
                    DbgPrint("[RCErr:%04x:%04x/%04x]\r\n", retry, pTxCtrl->SetKeyMap, pTxCtrl->Mp.pMacFrm->MacHeader.Tx.Status2);
                    pTxCtrl->Mp.pMacFrm->MacHeader.Tx.Status2 = pTxCtrl->SetKeyMap;
                }
            }
        }

        {
            u16 setmap, pollmap;
            WlMpKeyData *pKeyData;

            setmap = pTxCtrl->SetKeyMap;
            pollmap = pTxCtrl->Mp.pMacFrm->MacHeader.Tx.Status2;
            pKeyData = (WlMpKeyData *)pTxCtrl->pMpEndInd->mpKey.data;

            if ((pollmap > 1) && (wlMan->Config.Diversity != WL_CMDLABEL_DIVERSITY_OFF)) {
                if ((*(vu16 *)MREG_SIGNAL_STATE & (SIGNAL_STATE_CCA || SIGNAL_STATE_TXPE)) == 0) {
                    WChangeAntenna();
                }
            }

            if (wlMan->WlOperation & MAC_BUG_RESPONSE_COUNTER) {
                while (pollmap > 1) {
                    pollmap >>= 1;
                    if (pollmap & 1) {
                        pKeyData->noResponse++;
                    }

                    setmap >>= 1;
                    if (setmap & 1) {
                        pKeyData = (WlMpKeyData *)((u32)pKeyData + pTxCtrl->pMpEndInd->mpKey.length);
                    }
                }
            }
        }
        break;

    default:
        break;
    }

    if ((txFrm != TXSTS_MP) && ((*(vu16 *)MREG_TXQ_MAP & TXQ_NBLOCK_MP) == 0)) {
        if (pTxCtrl->Mp.Busy) {
            *(vu16 *)MREG_TXQ_RESET = TXQ_RST_MP;

            *(vu16 *)MREG_MP_POWER_SEQ = 0;

            if ((*(vu16 *)MREG_ISR & MISR_MPEND) == 0) {
                WlIntrMpEnd(2);
            } else {
                pTxCtrl->MpLastOk++;
            }

            pTxCtrl->MpRstCnt++;
        }

        TxqOpen(TXQ_OPEN_MP);

        DbgClrDDO(DDO_WL_MP, DDO_WL_MP_LAST_MP);
    }

    AddTask(TASK_CRITICAL_PRIORITY, INTR_TXEND_TASK_ID);

    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_TXEND);
}

static void WlIntrRxEnd(void) __attribute__((never_inline))
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPRX_CTRL pRxCtrl = &wlMan->RxCtrl;
    LPRXFRM_MAC pMFrm;
    u32 wlOperation = wlMan->WlOperation;
    u16 *pStatus, *pErrSts, *pNextBnry, *pTimeStamp, *pMPDU, *pDur;
    u32 bnry, curr, next_bnry, length, time[4], frameType;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_RXEND;
#else
    *(vu16 *)MREG_ISR = MISR_RXEND;
#endif

    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_RXEND));
    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_RXEND);

    if (pWork->Mode == WL_CMDLABEL_MODE_TEST) {
        *(vu16 *)MREG_RXBUF_BNR = *(vu16 *)MREG_RXBUF_CUR;
    }

    for (;;) {
        bnry = pRxCtrl->wlCurr;
        curr = *(vu16 *)MREG_RXBUF_CUR;

        if (bnry == curr) {
            break;
        }

        DbgSetDDO(DDO_WL_TSF, DDO_WL_TSF_SET);
        time[0] = *(vu16 *)MREG_TSF0;
        time[1] = *(vu16 *)MREG_TSF1;
        time[2] = *(vu16 *)MREG_TSF0;
        time[3] = *(vu16 *)MREG_TSF1;

        if (time[0] > time[2]) {
            time[0] = (time[2] >> 4) | (time[3] << 12);
        } else {
            time[0] = (time[0] >> 4) | (time[1] << 12);
        }
        DbgClrDDO(DDO_WL_TSF, DDO_WL_TSF_SET);

        if ((bnry >= (0x1000 + OFST_MREG_RXCOUNTER - 36) / 2) && (bnry <= (0x1000 + OFST_MREG_RES_CRR_CNT_EF) / 2)) {
            WUpdateCounter();
        }

        pMFrm = (LPRXFRM_MAC)GetMacRxAdrs(bnry);

        pStatus = (u16 *)pMFrm;
        pErrSts = (u16 *)((u32)pStatus + 2);
        pErrSts = (u16 *)AdjustRingPointer(pErrSts);
        pNextBnry = pErrSts;
        pTimeStamp = (u16 *)((u32)pErrSts + 2);
        pTimeStamp = (u16 *)AdjustRingPointer(pTimeStamp);
        pMPDU = (u16 *)((u32)pTimeStamp + 4);
        pMPDU = (u16 *)AdjustRingPointer(pMPDU);
        pDur = (u16 *)AdjustRingPointer(&pMFrm->Dot11Header.DurationID);

        *pStatus |= ((*pErrSts << 1) & RXSTS_ICV_ERR);

        *pTimeStamp = (u16)time[0];

        length = *pMPDU;

        next_bnry = ((bnry * 2 + length + 3 + 12) / 4) * 2;
        if (next_bnry >= MBUF_MAC_RX_END / 2) {
            next_bnry -= pWork->Ofst.RxBuf.Size / 2;
        }

        if (length > MAX_FRAME_LENGTH) {
            DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_CURRERR);
            DbgPrint("Warnning:Length (%u:%03x)\r\n", length, curr);
            *pStatus = 0xFFFF;

            next_bnry = curr;

            pWork->CurrErrCount++;

            DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_CURRERR);
        }

        else if (wlOperation & MAC_BUG_DIFF_CURR) {
            if (next_bnry != curr) {
                LPRXFRM_MAC pNextMFrm;
                u16 nextStatus, nextRate;

                pNextMFrm = (LPRXFRM_MAC)GetMacRxAdrs(next_bnry);

                nextStatus = pNextMFrm->MacHeader.Rx.Status;

                if ((u32)pNextMFrm < (MAC_MEM_BASE + MBUF_MAC_RX_END - 6)) {
                    nextRate = LoadLow(&pNextMFrm->MacHeader.Rx.Service_Rate);
                } else {
                    nextRate = *(u16 *)((u32)pNextMFrm - (u32)pWork->Ofst.RxBuf.Size + 6);
                }

                if ((nextStatus & 0x7C00) || ((nextRate != RF_RATE_1M) && (nextRate != RF_RATE_2M)) || (length > 0x0FFF)) {
                    DbgPrint("Curr Err[%04x:%04x:%04x]\r\n", bnry, next_bnry, curr);
                    pWork->CurrErrCount++;

                    *pStatus = 0xFFFF;

                    pRxCtrl->wlCurr = curr;
                    *pNextBnry = curr;
                    break;
                }
            }
        }

        frameType = *pStatus & RXSTS_FRAME_TYPE_MASK;
        if (frameType == RXSTS_MP) {
            u16 fc = *(u16 *)AdjustRingPointer(&pMFrm->Dot11Header.FrameCtrl.Data);
            u16 seqCtrl = *(u16 *)AdjustRingPointer(&pMFrm->Dot11Header.SeqCtrl.Data);

            if ((pRxCtrl->LastMpSeq == seqCtrl) && (fc & B_FC_RETRY)) {
                wlMan->Counter.rx.mpDuplicateErr++;
                *pStatus = 0xFFFF;
            }

            else if ((wlMan->Config.NullKeyRes == 0) && (wlMan->Work.STA == STA_CLASS3)) {
                if ((*(vu16 *)MREG_KSID != 0) && (*(vu16 *)MREG_KEYOUT_ADRS & TXQ_ENABLE)) {
                    OS_CancelAlarm(&wlMan->KeyAlarm);
                    OS_SetAlarm(&wlMan->KeyAlarm, OS_MicroSecondsToTicks(*pDur), WClearKSID, NULL);
                } else {
                    seqCtrl = 0xFFFF;
                    *(u16 *)MREG_MP_LAST_SEQCTRL = 0xFFFF;
                    *(u16 *)MREG_LAST_RX_ADRS0 = 0xFFFF;
                    *(u16 *)MREG_LAST_RX_ADRS1 = 0xFFFF;

                    *pStatus = 0xFFFF;
                }
            }

            pRxCtrl->LastMpSeq = seqCtrl;

            if (CheckKeyTxEnd() & 1) {
                wlMan->Counter.multiPoll.txNull++;
            }
        } else if (frameType == RXSTS_MPACK) {
            if ((wlMan->Config.NullKeyRes == 0) && (wlMan->Work.STA == STA_CLASS3)) {
                if ((*(vu16 *)MREG_KSID == 0) || ((*(vu16 *)MREG_KEYOUT_ADRS & TXQ_ENABLE) == 0)) {
                    *pStatus = 0xFFFF;
                }
            }
        }

        pRxCtrl->wlCurr = next_bnry;
        *pNextBnry = next_bnry;
    }

    if (wlOperation & MAC_BUG_DIFF_CURR) {
        u16 bkcurr = *(vu16 *)MREG_RXBUF_CUR;
        u16 v = (u16)CheckKeyTxEnd();

        if (v && (bkcurr == *(vu16 *)MREG_RXBUF_CUR)) {
            if (v & 2) {
                SetFatalErr(0x80);
            } else if (v & 1) {
                SetFatalErr(0x100);
            }
        }
    }

    if (*(vu16 *)MREG_RXBUF_BNR != *(vu16 *)MREG_RXBUF_CUR) {
        AddTask(TASK_CRITICAL_PRIORITY, INTR_RXEND_TASK_ID);
    }
}

static void WlIntrMpEnd(u32 bMacBugPatch)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 cnt;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_MPEND;
#else
    *(vu16 *)MREG_ISR = MISR_MPEND;
#endif

    DbgWlPutchar(B_WL_DBG_INT | B_WL_DBG_MP, (WL_DBG_INT_MPEND));

    if (pTxCtrl->Mp.Busy == FALSE) {
        DbgPuts("Mp end err\r");
        return;
    }

    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_MPEND);

    if (wlMan->WlOperation & MAC_BUG_TXMP_AFTER_MPEND) {
        if (bMacBugPatch) {
            u16 pri = *(vu16 *)MREG_TXQ_CURR;
            u16 state = *(vu16 *)MREG_MAC_STATE;

            if ((state == MAC_STATE_TX) || (state == MAC_STATE_WAIT)) {
                if (pri == 0) {
                    u16 tm, map;

                    DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_MPEND_ERR);
                    map = pTxCtrl->Mp.pMacFrm->MacHeader.Tx.Status2;
                    cnt = 0;
                    while (map != 0) {
                        cnt += (map & 1);
                        map >>= 1;
                    }
                    tm = cnt * (10 + *(u16 *)&pTxCtrl->Mp.pMacFrm->Body[0]) + 96 + (pTxCtrl->Mp.pMacFrm->MacHeader.Tx.MPDU) * TIME_BYTE_2M + 96;
                    SetupUsTimeOut(tm, MacBugTxMp);
                    DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_MPEND_ERR);
                    DbgWlPrint(B_WL_DBG_ERRMSG, "MpEnd err(%u:%u)\r\n", tm, bMacBugPatch);

                    wlMan->Work.MpEndErrCount++;
                    return;
                }
            }
        }
    }

    DbgClrDDO(DDO_WL_PS, DDO_WL_PS_MPING);
    DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_MPING | DDO_WL_MAC_MP_RESUME);
    DbgClrDDO(DDO_WL_MP, DDO_WL_MP_MPING);

    AddTask(TASK_CRITICAL_PRIORITY, INTR_MPEND_TASK_ID);

    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_MPEND);
}

static void WlIntrStartTx(void) __attribute__((never_inline))
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
#ifdef SDK_TS
    u32 i, cnt;
#endif

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_START_TX;
#else
    *(vu16 *)MREG_ISR = MISR_START_TX;
#endif

    if (wlMan->WlOperation & MAC_BUG_TXBUG_RXNOTPOLL) {
        u16 sts = *(vu16 *)MREG_MAC_STATE & 0x00FF;
        u16 adr = *(vu16 *)MREG_WDP_CUR_BUF_ADRS;

        if (((sts >= 3) && (sts <= 5)) && ((adr >= ((u32)pTxCtrl->Key[0].pMacFrm / 2 & 0x0FFF)) && (adr <= ((u32)pTxCtrl->Txq[2].pMacFrm / 2 & 0x0FFF)))) {
            DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_TX_FROM_RXBUF);
            *(vu16 *)(MAC_REG_BASE + 0x244) |= 0x0080;
            *(vu16 *)(MAC_REG_BASE + 0x244) &= ~0x0080;
            DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_TX_FROM_RXBUF);
        }
    }

#ifdef SDK_TS
    if (*(vu16 *)MREG_VERSION != 0x1440) {
        if ((*(vu16 *)MREG_SIGNAL_STATE & (SIGNAL_STATE_TXPE | SIGNAL_STATE_TRRDY)) == (SIGNAL_STATE_TXPE | SIGNAL_STATE_TRRDY)) {
            cnt = *(vu16 *)MREG_TXCOUNT;
            if (cnt != 0) {
                i = 0;
                while (cnt == *(vu16 *)MREG_TXCOUNT) {
                    if (i++ > 1000) {
                        DbgPrint("100Tx Error Detect[%u]\r\n", cnt);

                        SetFatalErr(WL_FATAL_ERR_CONTINUOUS_TX);
                        break;
                    }
                }
            }
        }
    }
#endif

    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_START_TX));
}

static void WlIntrStartRx(void) __attribute__((never_inline))
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR &= ~MISR_START_RX;
#else
    *(vu16 *)MREG_ISR = MISR_START_RX;
#endif

    DbgSetDDO(DDO_WL_INT, DDO_WL_INT_START_RX);

    if (wlMan->WlOperation & MAC_BUG_TXBUG_RXNOTPOLL) {
        if (pTxCtrl->BkKeyOut == 0xFFFF) {
            u32 i, tm;
            u16 *p;
            u16 curr, delt, duration, mpdu, tm_delt;

            DbgSetDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_RXSTART);

            if ((*(vu16 *)MREG_SIGNAL_STATE & (SIGNAL_STATE_CCA | SIGNAL_STATE_TRRDY)) != (SIGNAL_STATE_CCA | SIGNAL_STATE_TRRDY)) {
                goto not_suspend;
            }

            if (*(vu16 *)MREG_WDP_CUR_BUF_ADRS < ((*(vu16 *)MREG_RXBUF_STR / 2) & 0x0FFF)) {
                DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_DESTROY_TXBUF2);
                DbgPuts("WriteTxBuf\r");
                DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_DESTROY_TXBUF2);
                goto not_suspend;
            }

            curr = *(vu16 *)MREG_RXBUF_CUR;

            {
                p = (u16 *)&((LPRXFRM_MAC)GetMacRxAdrs(curr))->MacHeader.Rx.MPDU;

                p = (u16 *)AdjustRingPointer(p);

                mpdu = *p;

                p += 2;

                p = (u16 *)AdjustRingPointer(p);

                if ((*p++ & 0xE7FF) == FC_MP) {
                    DbgSetDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_CHK_FC);
                    DbgClrDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_CHK_FC);

                    p = (u16 *)AdjustRingPointer(p);

                    tm = (u32) * (vu16 *)MREG_TSF0 - 0x10000UL;

                    while (1) {

                        delt = *(vu16 *)MREG_WDP_CUR_BUF_ADRS - curr;
                        if (delt & 0x8000) {
                            delt += (pWork->Ofst.RxBuf.Size / 2);
                        }

                        if (delt > 6 + 1 + 1 + 3 + 3) {
                            break;
                        }

                        tm_delt = (u16)((u32) * (vu16 *)MREG_TSF0 - tm);
                        if (tm_delt > WAIT_RX_ADRS2) {
                            goto not_suspend;
                        }
                    }

                    duration = *p;
                    p += 4;

                    for (i = 0; i < 3; i++) {
                        p = (u16 *)AdjustRingPointer(p);

                        if (*p++ != pWork->BSSID[i]) {
                            goto not_suspend;
                        }
                    }

                    p += 5;

                    p = (u16 *)AdjustRingPointer(p);

                    while (1) {

                        delt = *(vu16 *)MREG_WDP_CUR_BUF_ADRS - curr;
                        if (delt & 0x8000) {
                            delt += (pWork->Ofst.RxBuf.Size / 2);
                        }

                        if (delt > 6 + 1 + 1 + 3 + 3 + 3 + 1 + 1 + 1) {
                            break;
                        }

                        tm_delt = (u16)((u32) * (vu16 *)MREG_TSF0 - tm);
                        if (tm_delt > WAIT_RX_BITMAP) {
                            goto not_suspend;
                        }
                    }

                    if ((*p++ & (1UL << *(vu16 *)MREG_KSID)) == 0) {
                        DbgSetDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_RESET);

                        pTxCtrl->BkKeyOut = *(vu16 *)MREG_KEYOUT_ADRS;

                        *(vu16 *)MREG_TXQ_RESET = TXQ_RST_KEYOUT;

                        pWork->NotPollTxErrCount++;

                        while (1) {
                            if ((*(vu16 *)MREG_SIGNAL_STATE & (SIGNAL_STATE_CCA | SIGNAL_STATE_TRRDY)) != (SIGNAL_STATE_CCA | SIGNAL_STATE_TRRDY)) {
                                break;
                            }
                        }
                        *(vu16 *)(MAC_REG_BASE + 0x244) |= 0x0040;

                        *(vu16 *)(MAC_REG_BASE + 0x244) &= ~0x0040;

                        *(vu16 *)(MAC_REG_BASE + 0x228) = 0x0008;
                        *(vu16 *)(MAC_REG_BASE + 0x228) = 0x0000;
                        MultiPollRevicedClearSeq();
                    }
                }
            }
not_suspend:
            DbgClrDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_RXSTART);
        }
    }
    DbgWlPutchar(B_WL_DBG_INT, (WL_DBG_INT_START_RX));

    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_START_RX);
}

static void SetParentTbttTxq(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 bTask = FALSE;

    ResetTxqPri(QID_BROADCAST);
    ResetTxqPri(QID_MANCTRL);
    ResetTxqPri(QID_DATA);

    if (pTxCtrl->Txq[QID_BROADCAST].Busy) {
        if (pTxCtrl->Txq[QID_BROADCAST].pMacFrm->MacHeader.Tx.Status != 0) {
            bTask = TRUE;
        } else {
            pTxCtrl->Txq[QID_BROADCAST].Busy = 0;
        }
    }

    if (pTxCtrl->Txq[QID_MANCTRL].Busy) {
        if (pTxCtrl->Txq[QID_MANCTRL].pMacFrm->MacHeader.Tx.Status != 0) {
            bTask = TRUE;
        } else {
            pTxCtrl->Txq[QID_MANCTRL].Busy = 0;
        }
    }

    if (pTxCtrl->Txq[QID_DATA].Busy) {
        if (pTxCtrl->Txq[QID_DATA].pMacFrm->MacHeader.Tx.Status != 0) {
            bTask = TRUE;
        } else {
            pTxCtrl->Txq[QID_DATA].Busy = 0;
        }
    }

    if (bTask) {
        AddTask(TASK_CRITICAL_PRIORITY, INTR_TXEND_TASK_ID);
    }
    AddTask(TASK_CRITICAL_PRIORITY, PARENT_TBTT_TXQ_TASK_ID);
}

void MacBugTxMp(void *arg)
{
#pragma unused(arg)

    u32 x, i;
    u16 seqCtrl, seqCtrl2;

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    seqCtrl = *(vu16 *)MREG_CURR_SEQCTRL;
    *(vu16 *)MREG_JUMP_TST = 0x1000;
    for (i = 100; i > 0; i--) {
        seqCtrl2 = *(vu16 *)MREG_CURR_SEQCTRL;
        if (seqCtrl != seqCtrl2) {
            break;
        }
    }
    *(vu16 *)MREG_JUMP_TST = 0x0000;
    WlIntrMpEnd(FALSE);

    OS_EnableIrqMask(x);

    DbgWlPrint(B_WL_DBG_ERRMSG, "Exec WlIntrMpEnd\r\n");
}

void *AdjustRingPointer(void *p)
{
    if ((u32)p >= (MAC_MEM_BASE + MBUF_MAC_RX_END)) {
        p = (void *)((u32)p - (u32)wlMan->Work.Ofst.RxBuf.Size);
    }

    return p;
}

static void MultiPollRevicedClearSeq(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;

    if (pTxCtrl->BkKeyOut != 0xFFFF) {
        u16 bkKeyIn;

        bkKeyIn = *(vu16 *)MREG_KEYIN_ADRS;
        *(vu16 *)MREG_KEYIN_ADRS = pTxCtrl->BkKeyOut;
        *(vu16 *)MREG_MDP_CONFIG |= 0x0080;
        *(vu16 *)MREG_KEYIN_ADRS = bkKeyIn;

        pTxCtrl->BkKeyOut = 0xFFFF;

        DbgClrDDO(DDO_WL_MACBUG_NOTPOLL, DDO_WL_MACBUG_NOTPOLL_RESET);
    }
}

static u32 CheckKeyTxEnd(void)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;

    return CheckKeyTxEndMain(&pTxCtrl->Key[0]) | CheckKeyTxEndMain(&pTxCtrl->Key[1]);
}

static u32 CheckKeyTxEndMain(LPTXQ pTxq)
{
    u32 ret = 0;

    if (pTxq->Busy == 2) {
        if ((SetMacTxAdrs(pTxq->pMacFrm) | TXQ_ENABLE) != *(vu16 *)MREG_KEYIN_ADRS) {
            pTxq->Busy = 1;
            ret |= 1;
        }
    }
    if (pTxq->Busy == 1 && (pTxq->pMacFrm->MacHeader.Tx.Status & TXSTS_FINISH)) {
        TxEndKeyData(pTxq);
        ret |= 2;
    }

    return ret;
}

void InitializeIntr(void)
{
    OS_SetIrqFunction(OS_IE_WIRELESS, WlIntr);

    OS_EnableIrqMask(OS_IE_WIRELESS);
}

void ReleaseIntr(void)
{
    OS_DisableIrqMask(OS_IE_WIRELESS);

    OS_SetIrqFunction(OS_IE_WIRELESS, NULL);
}
