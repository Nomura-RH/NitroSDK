#include "nitro/os/common/interrupt.h"
#define __MA_C_
#define __INSYSROM__

#include "WlSys.h"

#include "WlLib.h"
#include "MAC.h"
#include "MA.h"
#include "WlOpe.h"

u16 MA_DataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMaDataReq *pReq = (WlMaDataReq *)pReqt;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPTXFRM pFrm;
    u32 camAdrs;

    pFrm = (LPTXFRM)&pReq->frame;

    if (pReq->frame.length > MAX_DATA_LENGTH) {
        DbgPrint("[Data.req Len overflow(%u)]\r\n", pReq->frame.length);
        return WL_CMDRES_INVALID_PARAM;
    }

    switch (pConfig->Mode) {
    case WL_CMDLABEL_MODE_PARENT:
        camAdrs = CAM_Search(pFrm->Dot11Header.Adrs1);
        if ((camAdrs == CAM_NOT_FOUND) || (CAM_GetStaState(camAdrs) != STA_CLASS3)) {
            return WL_CMDRES_NOT_CLASS3_STA_FRAME;
        }
        break;

    default:
        camAdrs = pWork->APCamAdrs;
        break;
    }
    pFrm->FirmHeader.CamAdrs = camAdrs;

    pFrm->FirmHeader.FrameTime = wlMan->Work.IntervalCount;

    if (LoadLow(&pFrm->MacHeader.Tx.rsv_AppRate) == 0) {
        pFrm->MacHeader.Tx.Service_Rate = CAM_GetTxRate(camAdrs);
    } else {
        pFrm->MacHeader.Tx.Service_Rate = pFrm->MacHeader.Tx.rsv_AppRate;
        pFrm->MacHeader.Tx.rsv_AppRate = 0;
    }

    if (pFrm->FirmHeader.Length == 0) {
        pFrm->Dot11Header.FrameCtrl.Data = (pWork->FrameCtrl | B_FC_NULL) & ~B_FC_WEP;
        pFrm->MacHeader.Tx.MPDU = sizeof(pFrm->Dot11Header) + 4;
    } else {
        pFrm->Dot11Header.FrameCtrl.Data = pWork->FrameCtrl;
        if (pConfig->WepMode == WL_CMDLABEL_WEP_NO) {
            pFrm->MacHeader.Tx.MPDU = sizeof(pFrm->Dot11Header) + pFrm->FirmHeader.Length + 4;
        } else {
            pFrm->MacHeader.Tx.MPDU = sizeof(pFrm->Dot11Header) + pFrm->FirmHeader.Length + 4 + 4 + 4;
        }
    }

    switch (pConfig->Mode) {
    case WL_CMDLABEL_MODE_PARENT:
        WSetMacAdrs1(pFrm->Dot11Header.Adrs3, pFrm->Dot11Header.Adrs2);

        WSetMacAdrs1(pFrm->Dot11Header.Adrs2, pWork->BSSID);

        if (camAdrs == 0) {
            CAM_AddBcFrame(&pHeapMan->RequestCmd, pReq);

            if ((wlMan->CamMan.PowerMgtMode & ~wlMan->CamMan.NotClass3) == 0) {
                TxqPri(QID_BROADCAST);
            }
        } else {
            CAM_IncFrameCount(pFrm);
            MoveHeapBuf(&pHeapMan->RequestCmd, &pHeapMan->TxPri[QID_DATA], pReq);
            TxqPri(QID_DATA);
        }
        break;

    case WL_CMDLABEL_MODE_CHILD:
    case WL_CMDLABEL_MODE_HOTSPOT:
        WSetMacAdrs1(pFrm->Dot11Header.Adrs3, pFrm->Dot11Header.Adrs1);

        WSetMacAdrs1(pFrm->Dot11Header.Adrs1, pWork->BSSID);

        CAM_IncFrameCount(pFrm);
        MoveHeapBuf(&pHeapMan->RequestCmd, &pHeapMan->TxPri[QID_DATA], pReq);
        {
            TxqPri(QID_DATA);
        }
        break;
    }

    return WL_CMDRES_OPERATING_MA;
}

u16 MA_KeyDataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMaKeyDataReq *pReq = (WlMaKeyDataReq *)pReqt;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    u32 wlOperation = wlMan->WlOperation;
    LPTXFRM_MAC pFrm;
    u32 pos;

    pCfmt->header.length = 1;

    if (pConfig->Mode != WL_CMDLABEL_MODE_CHILD) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pReq->length > MAX_KEYDATA_LENGTH) {
        return WL_CMDRES_INVALID_PARAM;
    }

    if (pTxCtrl->Key[0].Busy) {
        pos = 1;
    } else {
        pos = 0;
    }
    if (pTxCtrl->Key[pos].Busy) {
        return WL_CMDRES_NOT_ENOUGH_MEM;
    }

    if (*(vu16 *)MREG_KEYIN_ADRS & TXQ_ENABLE) {
        return WL_CMDRES_NOT_ENOUGH_MEM;
    }

    pFrm = (LPTXFRM_MAC)pTxCtrl->Key[pos].pMacFrm;
    pFrm->MacHeader.Tx.Status = 0;
    pFrm->MacHeader.Tx.rsv_RetryCount = 0;
    pFrm->MacHeader.Tx.Service_Rate = RF_RATE_2M;
    pFrm->MacHeader.Tx.MPDU = sizeof(pFrm->Dot11Header) + pReq->length + 2 + 4; // 2-wmHeader,4-FCS

    pFrm->Dot11Header.FrameCtrl.Data = FC_MPKEY;
    WSetMacAdrs3(pFrm->Dot11Header.Adrs1, pWork->BSSID, pConfig->MacAdrs, (u16 *)MPKEY_ADRS);

    *(u16 *)&pFrm->Body[0] = pReq->wmHeader;
    if (pReq->length != 0) {
        if (pos == 0) {
            WUpdateCounter();
        }

        DMA_Write(&pFrm->Body[2], pReq->keyDatap, pReq->length);
    }

    if (wlOperation & MAC_BUG_DESTROY_TXBUF) {
        u16 *pId = (u16 *)(((u32)&pFrm->Body[pReq->length + 2] + 3) & 0xFFFFFFFC);
        *pId++ = DESTROY_TXBUF_ID0;
        *pId = DESTROY_TXBUF_ID1;
    }

    OSIrqMask e = OS_DisableIrqMask(OS_IE_WIRELESS);

    pTxCtrl->Key[pos].Busy = 2;
    pTxCtrl->Key[pos].InCount++;

    *(vu16 *)MREG_KEYIN_ADRS = TXQ_ENABLE | SetMacTxAdrs(pFrm);

    if (pConfig->NullKeyRes == 0) {
        WSetKSID();
    }

    (void)OS_EnableIrqMask(e);

    return WL_CMDRES_SUCCESS;
}

u16 MA_MpReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMaMpReq *pReq = (WlMaMpReq *)pReqt;
    WlMpKeyData *pKeyData;
    LPTXMPFRM_MAC pFrm;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPTX_CTRL pTxCtrl = (LPTX_CTRL)&wlMan->TxCtrl;
    u32 wlOperation = wlMan->WlOperation;
    u16 i, resume;
    u32 oneLen, x, pollCnt, delt, aid;
    u32 bDataUp = 1;
    u32 mp_time;

    pCfmt->header.length = 1;

    if (pConfig->Mode != WL_CMDLABEL_MODE_PARENT) {
        return WL_CMDRES_ILLEGAL_MODE;
    }

    if (pTxCtrl->Mp.Busy) {
        return WL_CMDRES_NOT_ENOUGH_MEM;
    }

    pFrm = (LPTXMPFRM_MAC)pTxCtrl->Mp.pMacFrm;

    if (pReq->resume & WL_CMDLABEL_MP_RESUME_EN) {
        if ((pReq->resume & WL_CMDLABEL_MP_RESUME_TXOP) == 0) {
            pReq->txop = pTxCtrl->TXOP;
        }
        if ((pReq->resume & WL_CMDLABEL_MP_RESUME_BITMAP) == 0) {
            pReq->pollBitmap = pTxCtrl->RestBitmap;
        }
        if ((pReq->resume & WL_CMDLABEL_MP_RESUME_TMPTT) == 0) {
            pReq->tmptt = pTxCtrl->TMPTT;
        }
        if ((pReq->resume & WL_CMDLABEL_MP_RESUME_DATA) == 0) {
            pReq->dataLength = pTxCtrl->DataLength;
            bDataUp = 0;
        }
    }

    if (pReq->dataLength > MAX_KEYDATA_LENGTH) {
        DbgPrint("[MP Data Len overflow(%u)]\r\n", pReq->dataLength);
        return WL_CMDRES_INVALID_PARAM;
    }

    for (i = 2, pollCnt = 0; i != 0; i <<= 1) {
        if (pReq->pollBitmap & i) {
            pollCnt++;
        }
    }

    pTxCtrl->TXOP = pReq->txop;
    if (pReq->txop & WL_CMDLABEL_MP_TXOP_TIME) {
        pReq->txop &= 0x7FFF;

        oneLen = ((u32)pReq->txop - TXOP_NULL_2M_S) / 4;
        pFrm->Dot11Header.DurationID = MPACK_2M_S;

        if (oneLen > 0x10000) {
            return WL_CMDRES_INVALID_PARAM;
        }
    } else {
        u32 macBug_byte, macBug_us, macBug_dur;

        if (wlOperation & MAC_BUG_TXOP_REV) {
            macBug_byte = MACBUG_MP_DELT_TXOP_BYTE;
            macBug_us = MACBUG_MP_DELT_TXOP_US;
            macBug_dur = MACBUG_MP_DELT_DUR;
        } else {
            macBug_byte = 0;
            macBug_us = 0;
            macBug_dur = 0;
        }

        oneLen = pReq->txop + macBug_byte;

        if (oneLen > MAX_KEYDATA_LENGTH + MAX_WMHEADER_LENGTH + macBug_byte) {
            return WL_CMDRES_INVALID_PARAM;
        }

        pReq->txop = TXOP_NULL_2M_S + oneLen * TIME_BYTE_2M + macBug_us;
        pFrm->Dot11Header.DurationID = MPACK_2M_S + macBug_dur;
    }
    oneLen = ((oneLen + 8 + 1) / 2) * 2;

    pTxCtrl->pMpEndInd = (WlMaMpEndInd *)AllocateHeapBuf(&pHeapMan->TmpBuf,
        sizeof(WlMaMpEndInd) - sizeof(WlMpKey) + 10 + oneLen * pollCnt);
    if ((u32)pTxCtrl->pMpEndInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        return WL_CMDRES_NOT_ENOUGH_MEM;
    }

    pTxCtrl->Mp.Busy = TRUE;
    pTxCtrl->Mp.InCount++;

    pTxCtrl->TMPTT = pReq->tmptt;
    pTxCtrl->SetKeyMap = pReq->pollBitmap;
    pTxCtrl->GetKeyMap = 0;
    pTxCtrl->DataLength = pReq->dataLength;
    pTxCtrl->RetryLimit = pReq->retryLimit;

    pFrm->MacHeader.Tx.Status = 0;
    pFrm->MacHeader.Tx.Status2 = pReq->pollBitmap;
    pFrm->MacHeader.Tx.rsv_RetryCount = 0;
    pFrm->MacHeader.Tx.Service_Rate = RF_RATE_2M;
    pFrm->MacHeader.Tx.MPDU = 24 + 2 + 2 + pReq->dataLength + 2 + 4;

    pFrm->Dot11Header.FrameCtrl.Data = FC_MP;
    pFrm->Dot11Header.DurationID += (10 + pReq->txop) * pollCnt;
    WSetMacAdrs3(pFrm->Dot11Header.Adrs1, (u16 *)MP_ADRS, pWork->BSSID, pConfig->MacAdrs);

    if ((pReq->resume & WL_CMDLABEL_MP_RESUME_EN) && (pFrm->Dot11Header.SeqCtrl.Data != 0xFFFF)) {
        resume = TXQ_RESUME;
    } else {
        resume = 0;
        pFrm->Dot11Header.SeqCtrl.Data = 0xFFFF;
    }

    pFrm->TXOP = pReq->txop;
    pFrm->Bitmap = pReq->pollBitmap;

    *(u16 *)&pFrm->Body[0] = pReq->wmHeader;
    if (bDataUp) {
        if (pReq->dataLength != 0) {
            WUpdateCounter();

            DMA_Write(&pFrm->Body[2], pReq->datap, pReq->dataLength);
        }
    }

    if (wlOperation & MAC_BUG_DESTROY_TXBUF) {
        u16 *pId = (u16 *)(((u32)&pFrm->Body[pReq->dataLength + 2] + 3) & 0xFFFFFFFC);
        *pId++ = DESTROY_TXBUF_ID0;
        *pId = DESTROY_TXBUF_ID1;
    }

    pTxCtrl->pMpEndInd->header.code = WL_CMDCODE_MA_MPEND_IND;
    pTxCtrl->pMpEndInd->header.length = CalcIndMsgLength2(WlMaMpEndInd, (oneLen * pollCnt - sizeof(WlMpKeyData)));

    pTxCtrl->pMpEndInd->mpKey.bitmap = pReq->pollBitmap;
    pTxCtrl->pMpEndInd->mpKey.count = pollCnt;
    pTxCtrl->pMpEndInd->mpKey.length = oneLen;
    pTxCtrl->pMpEndInd->mpKey.txCount = 0;

    pKeyData = (WlMpKeyData *)pTxCtrl->pMpEndInd->mpKey.data;
    aid = 1;
    for (i = 2; i != 0; i <<= 1, aid++) {
        if (pReq->pollBitmap & i) {
            pKeyData->length = 0xFFFF;
            *(u16 *)&pKeyData->rate = 0;
            pKeyData->noResponse = 0;
            pKeyData->aid = aid;

            pKeyData = (WlMpKeyData *)((u32)pKeyData + oneLen);
        }
    }

    *(vu16 *)MREG_MP_TXOP = pReq->txop;

    *(vu16 *)MREG_MP_DUR = pFrm->Dot11Header.DurationID;

    delt = 0x10000ul - (u32)pReq->currTsf;

    if (pReq->tmptt == 0) {
        mp_time = TIME_PREAMBLE_SHORT + (28 + 4 + pReq->dataLength + 2) * TIME_BYTE_2M;
        if (*(vu16 *)MREG_VERSION != 0x1440) {
            mp_time += 1000;
        }
        mp_time += TIME_MAX_MP_BACKOFF + pReq->txop * pollCnt + MPACK_2M_S;
        mp_time = (mp_time + TIME_DELT_MP) / 10;

        x = OS_DisableInterrupts();

        *(vu16 *)MREG_MP_TMPTT = mp_time;
        *(vu16 *)MREG_MP_ADRS = TXQ_ENABLE | SetMacTxAdrs(pFrm) | resume;

        OS_RestoreInterrupts(x);
    } else {
        x = OS_DisableInterrupts();
        delt = ((delt + *(vu16 *)MREG_TSF0) & 0xFFFF) / 10;
        if (delt + 3 < pReq->tmptt) {
            *(vu16 *)MREG_MP_TMPTT = pReq->tmptt - delt - 1;
            *(vu16 *)MREG_MP_ADRS = TXQ_ENABLE | SetMacTxAdrs(pFrm) | resume;
            OS_RestoreInterrupts(x);
        } else {
            OS_RestoreInterrupts(x);
            ReleaseHeapBuf(&pHeapMan->TmpBuf, pTxCtrl->pMpEndInd);

            pTxCtrl->Mp.Busy = FALSE;
            pTxCtrl->Mp.InCount--;

            return WL_CMDRES_INVALID_PARAM;
        }
    }

    return WL_CMDRES_SUCCESS;
}

u16 MA_TestDataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMaDataReq *pReq = (WlMaDataReq *)pReqt;
    LPTXFRM pFrm;

    pCfmt->header.length = 1;

    pReqt->header.code = CONTINUOUS_DATA_MODE;

    pFrm = (LPTXFRM)&pReq->frame;

    pFrm->FirmHeader.CamAdrs = 0;

    pFrm->MacHeader.Tx.MPDU = pFrm->FirmHeader.Length;

    CAM_IncFrameCount(pFrm);
    MoveHeapBuf(&wlMan->HeapMan.RequestCmd, &wlMan->HeapMan.TxPri[QID_DATA], pReq);
    TxqPri(QID_DATA);

    return WL_CMDRES_SUCCESS;
}

u16 MA_ClrDataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlMaClrDataReq *pReq = (WlMaClrDataReq *)pReqt;

    pCfmt->header.length = 1;

    if (pReq->flag & WL_CMDLABEL_CLRDATA_KEYDATA) {
        ClearTxKeyData();
    }
    if (pReq->flag & WL_CMDLABEL_CLRDATA_MP) {
        ClearTxMp();
    }
    if (pReq->flag & WL_CMDLABEL_CLRDATA_DATA) {
        ClearTxData();
    }

    return WL_CMDRES_SUCCESS;
}

void IssueMaDataConfirm(LPHEAPBUF_MAN pBufMan, void *pBuf)
{
    WlMaDataReq *pReq;
    WlMaDataCfm *pCfm;

    pReq = (WlMaDataReq *)pBuf;
    pCfm = (WlMaDataCfm *)WL_CalcConfirmPointer(pReq);

    pReq->header.code = pCfm->header.code;

    pCfm->header.length = CalcCfmMsgLength(WlMaDataCfm);
    pCfm->resultCode = WL_CMDRES_SUCCESS;
    pCfm->txStatus = pReq->frame.status;

    SendMessageToWmDirect(pBufMan, pReq);
}
