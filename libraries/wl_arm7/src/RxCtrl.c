#define __RXCTRL_C_
#define __INSYSROM__

#include "WlSys.h"
#include "TaskMan.h"

#include "WlLib.h"
#include "RxCtrl.h"
#include "MAC.h"
#include "CAM.h"
#include "MA.h"
#include "WlOpe.h"

void RxDataFrameTask(void)
{
    LPDEAUTH_FRAME pDeAuth;
    WlMaDataInd *pInd;
    LPRXFRM pFrm;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    WlCounter *pCounter = &wlMan->Counter;
    u32 camAdrs, err;

    pInd = (WlMaDataInd *)GetHeapBufHeadAdrs(&pHeapMan->RxData);
    if ((u32)pInd == HEAPBUF_HEAD_NONE) {
        return;
    }

    if (pWork->STA != STA_CLASS3) {
        ReleaseHeapBuf(&pHeapMan->RxData, pInd);
        return;
    }

    pFrm = (LPRXFRM)&pInd->frame;

    if (pFrm->Dot11Header.FrameCtrl.Bit.ToDS) {
        if (pFrm->Dot11Header.Adrs3[0] & 0x0001) {
            pCounter->rx.multicast++;
        } else {
            pCounter->rx.unicast++;
        }
    } else {
        if (pFrm->Dot11Header.Adrs1[0] & 0x0001) {
            pCounter->rx.multicast++;
        } else {
            pCounter->rx.unicast++;
        }
    }
    pCounter->rx.fragment += (GetFragCount(pFrm->MacHeader.Rx.Status) - 1);

    err = 1;
    switch (pWork->Mode) {
    case WL_CMDLABEL_MODE_PARENT:

        if (pFrm->Dot11Header.FrameCtrl.Data & (B_FC_TODS | B_FC_FROMDS) != B_FC_TODS) {
            DbgPuts("fc tods err\r");
            break;
        }

        camAdrs = CAM_Search(pFrm->Dot11Header.Adrs2);
        if ((camAdrs == CAM_NOT_FOUND) || (CAM_GetStaState(camAdrs) != STA_CLASS3)) {
            if (CAM_GetStaState(camAdrs) == STA_CLASS2) {
                if (IsExistManFrame(pFrm->Dot11Header.Adrs2, FC_DISASS)) {
                    break;
                }
                pDeAuth = (LPDEAUTH_FRAME)MakeDisAssFrame(pFrm->Dot11Header.Adrs2, WL_CMDLABEL_RSN_RX_CLASS3_FROM_NONASS_STA);
            } else {
                if (IsExistManFrame(pFrm->Dot11Header.Adrs2, FC_DEAUTH)) {
                    break;
                }
                pDeAuth = MakeDeAuthFrame(pFrm->Dot11Header.Adrs2, WL_CMDLABEL_RSN_RX_CLASS3_FROM_NONASS_STA, TRUE);
            }
            if ((u32)pDeAuth != HEAPBUF_NOT_ENOUGH_MEMORY) {
                TxManCtrlFrame((LPTXFRM)pDeAuth);
            }
            break;
        }

        CAM_SetPowerMgtMode(camAdrs, pFrm->Dot11Header.FrameCtrl.Bit.PowerMan);

        if (pFrm->Dot11Header.SeqCtrl.Data == CAM_GetLastSeqCtrl(camAdrs)) {
            pCounter->rx.duplicateErr++;
            break;
        }

        WSetMacAdrs1(pFrm->Dot11Header.Adrs1, pFrm->Dot11Header.Adrs3);

        err = 0;
        break;

    case WL_CMDLABEL_MODE_CHILD:
    case WL_CMDLABEL_MODE_HOTSPOT:

        if (pFrm->Dot11Header.FrameCtrl.Data & (B_FC_TODS | B_FC_FROMDS) != B_FC_TODS) {
            DbgPuts("fc tods err\r");
            break;
        }

        if (pWork->PowerMgtMode != PWRMODE_ACT) {

            if ((pFrm->Dot11Header.FrameCtrl.Data & B_FC_MOREDATA) == 0) {
                if (pFrm->Dot11Header.Adrs1[0] & 0x0001) {
                    pWork->bExistTIM &= ~TIM_BC;
                } else {
#if (WL_NOTPS_VMAP1)
                    if (pWork->Mode != WL_CMDLABEL_MODE_HOTSPOT) {
                        pWork->bExistTIM &= ~TIM_UC;
                    }
#else
                    {
                        pWork->bExistTIM &= ~TIM_UC;
                    }
#endif
                }

                if ((pWork->bExistTIM == TIM_FREE) && (GetHeapBufCount(&pHeapMan->TxPri[QID_DATA]) == 0) && (GetHeapBufCount(&pHeapMan->TxPri[QID_MANCTRL]) == 0)) {
                    {
                        WSetPowerState(PWRSTS_PS);
                    }
                }
            }
        }

        camAdrs = pWork->APCamAdrs;

        if (pFrm->Dot11Header.SeqCtrl.Data == CAM_GetLastSeqCtrl(camAdrs)) {
            pCounter->rx.duplicateErr++;
            break;
        }

        WSetMacAdrs1(pFrm->Dot11Header.Adrs2, pFrm->Dot11Header.Adrs3);

        err = 0;
        break;
    }

    if (err == 0) {
        DbgWlPutchar(B_WL_DBG_MA, (WL_DBG_MA_DATA_IND));

        pFrm->FirmHeader.CamAdrs = camAdrs;

        CAM_SetRSSI(camAdrs, LoadLow(&pFrm->MacHeader.Rx.rsv_RSSI));
        CAM_SetLastSeqCtrl(camAdrs, pFrm->Dot11Header.SeqCtrl.Data);

        CAM_UpdateLifeTime(camAdrs);

        pFrm->FirmHeader.Length = pFrm->MacHeader.Rx.MPDU - 24;

        pInd->header.code = WL_CMDCODE_MA_DATA_IND;
        pInd->header.length = WL_CalcHeaderLength(pFrm->FirmHeader.Length + sizeof(WlRxFrame) - 4);

        /*		{
                                void DumpMemory(u8*,u32,u32);
                                DbgPrint("[Rx]\r\n");
                                DumpMemory((u8*)pInd,0x100,'W');
                        }*/

        DbgSetDDO(DDO_WL_PS, DDO_WL_PS_MADATA_IND);
        SendMessageToWmDirect(&pHeapMan->RxData, pInd);
        DbgClrDDO(DDO_WL_PS, DDO_WL_PS_MADATA_IND);
    } else {
        ReleaseHeapBuf(&pHeapMan->RxData, pInd);
    }

    if (GetHeapBufCount(&pHeapMan->RxData) != 0) {
        AddTask(TASK_NORMAL_PRIORITY, RXDATA_TASK_ID);
    }
}

u32 RxMpFrame(LPRXFRM pFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPRX_CTRL pRxCtrl = &wlMan->RxCtrl;
    WlMaMpInd *pInd;
    u32 cnt;
    u16 bitmap, out;

    if (pWork->STA != STA_CLASS3) {
        return 1;
    }

    if ((MatchMacAdrs(pFrm->Dot11Header.Adrs2, pWork->BSSID) == 0) || (MatchMacAdrs(pFrm->Dot11Header.Adrs3, pWork->LinkAdrs) == 0)) {
        return 1;
    }

    DbgWlPutchar(B_WL_DBG_MP, (WL_DBG_RX_MP));
    DbgSetDDO(DDO_WL_TSF, DDO_WL_TSF_RXMP);

    bitmap = 0x0001 << pWork->AID;
    if ((bitmap & ((WlRxMpFrame *)pFrm)->bitmap) == 0) {
        DbgSetDDO(DDO_WL_MP, DDO_WL_MP_RX_NP);
        pRxCtrl->TxKeyReg = 0;
        DbgClrDDO(DDO_WL_MP, DDO_WL_MP_RX_NP);
    } else {
        pRxCtrl->TxKeyReg = RXSTS_POLLED_MP;
    }

    out = *(vu16 *)MREG_KEYOUT_ADRS;
    if ((out & 0x8000) && (((LPRXFRM_MAC)GetMacRxAdrs(out & 0x7FFF))->MacHeader.Tx.rsv_RetryCount != 0)) {
        pRxCtrl->TxKeyReg |= RXSTS_TX_KEY;
    }

    CAM_UpdateLifeTime(pWork->APCamAdrs);

    pFrm->FirmHeader.Length = pFrm->MacHeader.Rx.MPDU - 24 - 2 - 2;

    pInd = (WlMaMpInd *)CalcReqAdrsFromFrame(pFrm);

    pInd->header.code = WL_CMDCODE_MA_MP_IND;
    pInd->header.length = WL_CalcHeaderLength(pFrm->FirmHeader.Length + sizeof(WlRxMpFrame) - 2);

    bitmap = pInd->frame.bitmap;
    for (cnt = 0; bitmap != 0; bitmap >>= 1) {
        if (bitmap & 1) {
            cnt++;
        }
    }

    pInd->frame.txKeySts |= (pRxCtrl->TxKeyReg | ((*(vu16 *)MREG_KEYOUT_ADRS & 0x8000) >> 4) | ((*(vu16 *)MREG_KEYIN_ADRS & 0x8000) >> 3));

    pInd->frame.ackTimeStamp = pInd->frame.timeStamp + ((cnt * (pInd->frame.txop + 10) + (TIME_SIFS + TIME_PREAMBLE_SHORT + 32 * TIME_BYTE_2M + 3 + 15)) >> 4);

    SendMessageToWmDirect(&wlMan->HeapMan.TmpBuf, pInd);

    DbgClrDDO(DDO_WL_TSF, DDO_WL_TSF_RXMP);

    return 0;
}

void RxKeyDataFrame(LPRXFRM pFrm)
{
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;
    WlMpKey *pMpKey = &pTxCtrl->pMpEndInd->mpKey;
    WlMpKeyData *pKeyData;
    u32 camAdrs, aid;
    u16 bitmap;
    if (pTxCtrl->Mp.Busy == FALSE) {
        return;
    }

    if (!MatchMacAdrs(pFrm->Dot11Header.Adrs1, wlMan->Work.BSSID)) {
        DbgPuts("KeyData BSSID err\r");
        return;
    }

    if (pFrm->MacHeader.Rx.MPDU - 24 > pMpKey->length - 8) {
        DbgPuts("KeyData OverLength\r");
        return;
    }

    camAdrs = CAM_Search(pFrm->Dot11Header.Adrs2);
    if ((camAdrs == CAM_NOT_FOUND) || ((camAdrs != 0) && (CAM_GetStaState(camAdrs) != STA_CLASS3))) {
        LPDEAUTH_FRAME pTxDeAuthFrm;

        DbgSetDDO(DDO_WL_MAC, DDO_WL_MP_RXKEY_CAM_ERR);
        DbgPrint("Rx KeyData cam err[%04x-%04x-%04x]\r\n",
            SwapEndianWord(pFrm->Dot11Header.Adrs2[0]),
            SwapEndianWord(pFrm->Dot11Header.Adrs2[1]),
            SwapEndianWord(pFrm->Dot11Header.Adrs2[2]));

        if (IsExistManFrame(pFrm->Dot11Header.Adrs2, FC_DEAUTH)) {
            return;
        }

        pTxDeAuthFrm = MakeDeAuthFrame(pFrm->Dot11Header.Adrs2, MAN_RSN_RX_CLASS3_FROM_NONASS_STA, FALSE);
        if ((u32)pTxDeAuthFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
            pTxDeAuthFrm->FirmHeader.FrameId = FID_DEAUTH_IND2;

            TxManCtrlFrame((LPTXFRM)pTxDeAuthFrm);
        }
        DbgClrDDO(DDO_WL_MAC, DDO_WL_MP_RXKEY_CAM_ERR);
        return;
    }

    if (camAdrs == 0) {
        return;
    }

    CAM_SetPowerMgtMode(camAdrs, pFrm->Dot11Header.FrameCtrl.Bit.PowerMan);

    CAM_UpdateLifeTime(camAdrs);

    aid = CAM_GetAID(camAdrs);

    bitmap = 0x0001 << aid;
    if (bitmap & pTxCtrl->GetKeyMap) {
        DbgPrint("Rx KeyData dup err[GetedMap=%x:Rx=%x]\r\n", pTxCtrl->GetKeyMap, bitmap);
        return;
    }

    if ((bitmap & pTxCtrl->SetKeyMap) == 0) {
        DbgPrint("Rx KeyData not poll sta[%04x-%04x-%04x][SetMap=%x:Rx=%x]\r\n", pFrm->Dot11Header.Adrs2[0], pFrm->Dot11Header.Adrs2[1], pFrm->Dot11Header.Adrs2[2], pTxCtrl->SetKeyMap, bitmap);
        return;
    }

    pTxCtrl->GetKeyMap |= bitmap;
    pMpKey->bitmap &= ~bitmap;

    pKeyData = (WlMpKeyData *)pMpKey->data;
    for (bitmap >>= 1; bitmap != 0x0001; bitmap >>= 1) {
        if (bitmap & pTxCtrl->SetKeyMap) {
            pKeyData = (WlMpKeyData *)((u32)pKeyData + (u32)pMpKey->length);
        }
    }

    pKeyData->length = pFrm->MacHeader.Rx.MPDU - 24;
    WL_WriteByte(&pKeyData->rssi, LoadLow(&pFrm->MacHeader.Rx.rsv_RSSI));
    WL_WriteByte(&pKeyData->rate, LoadLow(&pFrm->MacHeader.Rx.Service_Rate));

    if (pKeyData->length != 0) {
        WLLIB_DmaCopy16((u32)pFrm->Body, (u32)pKeyData->cdata, pKeyData->length + 1);
    }
}

u32 RxMpAckFrame(LPRXFRM pFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    WlMaMpAckInd *pInd;

    if (wlMan->Work.STA != STA_CLASS3) {
        return 1;
    }

    if ((MatchMacAdrs(pFrm->Dot11Header.Adrs2, pWork->BSSID) == 0) || (MatchMacAdrs(pFrm->Dot11Header.Adrs3, pWork->LinkAdrs) == 0)) {
        DbgPrint("[mpack not for me]");
        return 1;
    }

    DbgSetDDO(DDO_WL_TSF, DDO_WL_TSF_RXMPACK);

    pFrm->FirmHeader.Length = pFrm->MacHeader.Rx.MPDU - 24 - 2 - 2;

    pInd = (WlMaMpAckInd *)CalcReqAdrsFromFrame(pFrm);

    pInd->header.code = WL_CMDCODE_MA_MPACK_IND;
    pInd->header.length = WL_CalcHeaderLength(sizeof(WlRxMpAckFrame));

    pInd->ack.txKeySts |= (wlMan->RxCtrl.TxKeyReg | ((*(vu16 *)MREG_KEYOUT_ADRS & 0x8000) >> 4) | ((*(vu16 *)MREG_KEYIN_ADRS & 0x8000) >> 3));

    /*
    {
            u32	delt;

            delt = pInd->ack.timeStamp - ackTimeStamp;
            if( (delt > 0) && (delt < 20) )
            {
                    DbgSetDDO(DDO_WL_TSF, DDO_WL_TSF_OVRTSF);
                    DbgPrint("[MPACK=%04x:%04x/%04x]%u\r\n", mpTimeStamp,ackTimeStamp,pInd->ack.timeStamp,delt);
                    DbgClrDDO(DDO_WL_INT, DDO_WL_INT_TSF_ACKERR);
            }
    }
*/
    /*	{
                    void DumpMemory(u8*,u32,u32);
                    DumpMemory((u8*)pInd,0x80,'W');
            }
    */
    SendMessageToWmDirect(&wlMan->HeapMan.TmpBuf, pInd);

    DbgClrDDO(DDO_WL_TSF, DDO_WL_TSF_RXMPACK);

    return 0;
}

void RxBeaconFrame(LPBEACON_FRAME pFrm)
{
    LPBEACON_BODY pBeacon;
    LPWORK_PARAM pWork = &wlMan->Work;
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    ELEMENT_CHECKER elementCheck;
    u32 n, n1, n2, ofst, bit, bp;
    u32 BodyLen, cam_adrs;
    u64 lltsf;
    u16 t1[4], t2[4];
    u16 *ptsf;

    DbgWlPutchar(B_WL_DBG_BEACON, (WL_DBG_RX_BEACON));

    wlMan->Counter.rx.beacon++;

    cam_adrs = CAM_SearchAdd(pFrm->Dot11Header.SA);
    pFrm->FirmHeader.CamAdrs = cam_adrs;
    if (cam_adrs == CAM_NOT_FOUND) {
        return;
    }

    CAM_SetRSSI(cam_adrs, pFrm->MacHeader.Rx.rsv_RSSI);

    pBeacon = &pFrm->Body;
    BodyLen = pFrm->FirmHeader.Length;

    if (BodyLen <= 8 + 2 + 2) {
        return;
    }

    MI_CpuClear32(&elementCheck, sizeof(elementCheck));
    elementCheck.pElement = pBeacon->Buf;
    elementCheck.bodyLength = BodyLen - (8 + 2 + 2);
    elementCheck.matchFlag = B_MATCH_CHANNEL;
    if (pWork->SSIDLength == 0) {
        elementCheck.matchFlag |= B_MATCH_SSID;
    }
    elementCheck.foundFlag = B_FOUND_BSSID | B_FOUND_CAPA;
    elementCheck.rxStatus = pFrm->MacHeader.Rx.Status;
    elementCheck.capability = pBeacon->CapaInfo.Data;
    ElementChecker(&elementCheck);

    DbgWlPrint(B_WL_DBG_MLME, "[BcnMatch:%x/%x]", elementCheck.matchFlag, elementCheck.foundFlag);

    if ((elementCheck.pCFP != NULL) && (pFrm->Dot11Header.Duration & 0x8000)) {
        *(vu16 *)MREG_NAV = (u16)WL_ReadByte(&elementCheck.pCFP->CFPDurRemain.u8[0]) + ((u16)WL_ReadByte(&elementCheck.pCFP->CFPDurRemain.u8[1]) << 8);
    }

    if ((pMLME->State == MLME_STATE_SCAN_ING) && (pMLME->pReq.Scan->scanType == WL_CMDLABEL_SCAN_PASSIVE)) {
        DbgWlPrint(B_WL_DBG_MLME, "[SBcnMatch:%x]", elementCheck.matchFlag);
        if ((elementCheck.matchFlag & (B_MATCH_SSID | B_MATCH_BSSID)) == (B_MATCH_SSID | B_MATCH_BSSID)) {
            RxProbeResFrame((LPPRBRES_FRAME)pFrm, &elementCheck);
        }
    }

    else if (elementCheck.matchFlag & B_MATCH_BSSID) {
        {
            DbgSetDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_RX_MYBEACON);

            if (pMLME->State == MLME_STATE_JOIN_ING) {
                ClearTimeOut();

                DbgWlPrint(B_WL_DBG_MLME, "[JoinMatch:%x]", elementCheck.matchFlag);

                if ((elementCheck.matchFlag & (B_MATCH_CAPA_ESS | B_MATCH_CAPA_WEP)) != (B_MATCH_CAPA_ESS | B_MATCH_CAPA_WEP)) {
                    pMLME->Work.Join.Result = WL_CMDRES_FAILURE;
                    pMLME->Work.Join.Status = WL_CMDLABEL_STS_NOT_SUPPORT_CAPABILITY;
                    DbgWlPrint(B_WL_DBG_MLME, "[Join:CAPA_ERR]");
                }
#if (WL_CHK_RATESET)
                else if ((elementCheck.matchFlag & B_MATCH_RATESET) == 0) {
                    pMLME->Work.Join.Result = WL_CMDRES_FAILURE;
                    pMLME->Work.Join.Status = WL_CMDLABEL_STS_INVALID_BASICRATESET;
                    DbgWlPrint(B_WL_DBG_MLME, "[Join:BRS_ERR]");
                }
#endif
                else if (pFrm->Body.BeaconInterval > MAX_BEACON_PERIOD) {
                    pMLME->Work.Join.Result = WL_CMDRES_FAILURE;
                    pMLME->Work.Join.Status = WL_CMDLABEL_STS_UNSPECIFIED;
                    DbgWlPrint(B_WL_DBG_MLME, "[Join:BCN_ERR]");
                }

                else {
                    pMLME->Work.Join.Result = WL_CMDRES_SUCCESS;

                    if ((elementCheck.foundFlag & B_FOUND_CHANNEL) && !(elementCheck.matchFlag & B_MATCH_CHANNEL)) {
                        DbgPrint("[JoinChErr:%u->%u]\r\n", pWork->CurrChannel, elementCheck.channel);
                        WSetChannel(elementCheck.channel, FALSE);
                    }

                    CAM_SetSupRate(cam_adrs, elementCheck.rateSet.Support);

                    if (pWork->Mode == WL_CMDLABEL_MODE_CHILD) {
                        if (elementCheck.pGMIF != NULL) {
                            u32 actZone = (u32)WL_ReadByte(&elementCheck.pGMIF->ActZone[0]) + ((u32)WL_ReadByte(&elementCheck.pGMIF->ActZone[1]) << 8);
                            WSetActiveZoneTime(actZone, TRUE);

                            *(u16 *)V_TSF = (u16)WL_ReadByte(&elementCheck.pGMIF->VTSF[0]) + ((u16)WL_ReadByte(&elementCheck.pGMIF->VTSF[1]) << 8);
                        } else {
                            WSetActiveZoneTime(0xFFFF, TRUE);
                            *(u16 *)V_TSF = 0;
                        }
                    }

                    WSetDTIMPeriod(WL_ReadByte(&elementCheck.pTIM->DTIMPeriod));
                    pWork->DTIMCount = WL_ReadByte(&elementCheck.pTIM->DTIMCount);

                    WSetBeaconPeriod(pFrm->Body.BeaconInterval);

                    pWork->bSynchro = 1;
                    pWork->bFirstTbtt = 1;

                    if (pWork->Mode == WL_CMDLABEL_MODE_CHILD) {
                        *(vu16 *)MREG_MP_POWER_SEQ = 3;
                    }
                    *(vu16 *)MREG_WAKEUP_CTRL |= WAKEUP_CTRL_WAKEUP_TBTT;
                }

                WSetMacAdrs1(pMLME->pCfm.Join->peerMacAdrs, pFrm->Dot11Header.SA);

                pMLME->State = MLME_STATE_JOIN_FIN;

                AddTask(TASK_NORMAL_PRIORITY, MLME_JOIN_TASK_ID);
            }

            switch (pWork->Mode) {

            case WL_CMDLABEL_MODE_CHILD:
                if (elementCheck.pGMIF != NULL) {
                    WSetActiveZoneTime((u16)WL_ReadByte(&elementCheck.pGMIF->ActZone[0]) + ((u16)WL_ReadByte(&elementCheck.pGMIF->ActZone[1]) << 8), FALSE);

                    *(u16 *)V_TSF = (u16)WL_ReadByte(&elementCheck.pGMIF->VTSF[0]) + ((u16)WL_ReadByte(&elementCheck.pGMIF->VTSF[1]) << 8);

                    pWork->GameInfoLength = WL_ReadByte(&elementCheck.pGMIF->Length) - 3 - 1 - 4;

                    if (pWork->GameInfoLength != 0) {
                        if ((u32)elementCheck.pGMIF & 1) {
                            WLLIB_DmaCopy16((u32)&elementCheck.pGMIF->VTSF[1], (u32)pWork->GameInfoAdrs, pWork->GameInfoLength + 2);
                            pWork->GameInfoAlign = 1;
                        } else {
                            WLLIB_DmaCopy16((u32)elementCheck.pGMIF->GameInfo, (u32)pWork->GameInfoAdrs, pWork->GameInfoLength + 1);
                            pWork->GameInfoAlign = 0;
                        }
                    }
                }

            case WL_CMDLABEL_MODE_HOTSPOT:

                pWork->BeaconLostCnt = 0;

                CAM_UpdateLifeTime(cam_adrs);

                lltsf = *(u64 *)pFrm->Body.TimeStamp;
                bp = (u32)pWork->BeaconPeriod * 1024;
                lltsf /= bp;
                lltsf++;
                lltsf *= bp;
                ptsf = (u16 *)&lltsf;
                *(vu16 *)MREG_NEXT_TBTT_TSF3 = ptsf[3];
                *(vu16 *)MREG_NEXT_TBTT_TSF2 = ptsf[2];
                *(vu16 *)MREG_NEXT_TBTT_TSF1 = ptsf[1];
                *(vu16 *)MREG_NEXT_TBTT_TSF0 = ptsf[0] | 0x0001;

                if ((pWork->Mode == WL_CMDLABEL_MODE_CHILD) && (pWork->bFirstTbtt)) {
                    lltsf -= (u64)bp;

                    n = OS_DisableInterrupts();
                    t1[0] = *(vu16 *)MREG_TSF0;
                    t1[1] = *(vu16 *)MREG_TSF1;
                    t1[2] = *(vu16 *)MREG_TSF2;
                    t1[3] = *(vu16 *)MREG_TSF3;
                    t2[0] = *(vu16 *)MREG_TSF0;
                    t2[1] = *(vu16 *)MREG_TSF1;
                    t2[2] = *(vu16 *)MREG_TSF2;
                    t2[3] = *(vu16 *)MREG_TSF3;
                    OS_RestoreInterrupts(n);

                    if (t1[0] < t2[0]) {
                        n = (u32)((*(u64 *)t1 - lltsf) / 1024);
                    } else {
                        n = (u32)((*(u64 *)t2 - lltsf) / 1024);
                    }
                    if (n < pConfig->ActiveZone) {
                        *(vu16 *)MREG_ACTZONE = pConfig->ActiveZone - n;
                    } else {
                        *(vu16 *)MREG_ACTZONE = 0;
                    }
                }

                if ((pWork->STA == STA_CLASS3) && (elementCheck.pTIM != NULL) && (pWork->PowerMgtMode == WL_CMDLABEL_PMG_PS)) {
                    u16 dtimCnt = WL_ReadByte(&elementCheck.pTIM->DTIMCount);

                    if (pWork->DTIMCount != dtimCnt) {
                        DbgPrint("Diff TIMCnt:%u/%u\r\n", pWork->DTIMCount, dtimCnt);

                        pWork->DTIMCount = dtimCnt;
                    }

                    pWork->bExistTIM = TIM_FREE;

                    if (dtimCnt == 0) {
                        if (WL_ReadByte(&elementCheck.pTIM->BitmapCtrl) & 0x01) {
                            pWork->bExistTIM |= TIM_BC;
                        }
                    }

                    n = WL_ReadByte(&elementCheck.pTIM->BitmapCtrl) & 0x00FEUL;
                    n1 = n * 8;
                    n2 = (n + WL_ReadByte(&elementCheck.pTIM->Length) - 3) * 8;
                    if ((n1 <= pWork->AID) && (pWork->AID <= n2)) {
                        ofst = pWork->AID - n1;
                        bit = ofst & 0x07;
                        ofst = ofst / 8;

                        if (WL_ReadByte(&elementCheck.pTIM->VitrualBitmap[ofst]) & (0x01 << bit)) {
                            pWork->bExistTIM |= TIM_UC;

                            TxPsPollFrame();
                        }
                    }

                    if ((pHeapMan->TxPri[QID_DATA].Count == 0) && (pHeapMan->TxPri[QID_MANCTRL].Count == 0) && (pWork->bExistTIM == TIM_FREE)) {
                        {
                            WSetPowerState(PWRSTS_PS);
                        }
                    }
                }
            }

            if (pConfig->BcnTxRxIndMsg) {
                MLME_IssueBeaconRecvIndication(pFrm);
            }

            DbgClrDDO(DDO_WL_LSTN_INT, DDO_WL_LSTN_INT_RX_MYBEACON);
        }
    }

    if (elementCheck.pSSID != NULL) {
        UpdateApList(elementCheck.channel, pFrm, elementCheck.pSSID);
    }
}

static void RxDisAssFrame(LPDISASS_FRAME pFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPDEAUTH_FRAME pDeAuth;
    u32 st, cam_adrs;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_DISASS));

    cam_adrs = pFrm->FirmHeader.CamAdrs;
    switch (pWork->Mode) {
    case WL_CMDLABEL_MODE_PARENT:

        st = CAM_GetStaState(cam_adrs);
        if (st == STA_CLASS3) {
            CAM_SetStaState(cam_adrs, STA_CLASS2);

            MLME_IssueDisAssIndication(pFrm->Dot11Header.SA, pFrm->Body.ReasonCode);

            DeleteTxFrames(cam_adrs);
        } else {
            if (st == STA_CLASS2) {
                pDeAuth = (LPDEAUTH_FRAME)MakeDisAssFrame(pFrm->Dot11Header.SA, WL_CMDLABEL_RSN_RX_CLASS3_FROM_NONASS_STA);
            } else {
                pDeAuth = MakeDeAuthFrame(pFrm->Dot11Header.SA, WL_CMDLABEL_RSN_RX_CLASS3_FROM_NONASS_STA, TRUE);
            }
            if ((u32)pDeAuth != HEAPBUF_NOT_ENOUGH_MEMORY) {
                TxManCtrlFrame((LPTXFRM)pDeAuth);
            }
        }
        break;

    case WL_CMDLABEL_MODE_CHILD:
    case WL_CMDLABEL_MODE_HOTSPOT:

        if ((pWork->STA == STA_CLASS3) && (MatchMacAdrs(pFrm->Dot11Header.SA, pWork->LinkAdrs))) {
            WSetStaState(STA_CLASS2);

            WClearAids();

            MLME_IssueDisAssIndication(pFrm->Dot11Header.SA, pFrm->Body.ReasonCode);
        }
        break;
    }
}

static void RxAssReqFrame(LPASSREQ_FRAME pFrm)
{
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    LPASSREQ_BODY pAssReq;
    LPDEAUTH_FRAME pTxDeAuthFrm;
    LPASSRES_FRAME pTxFrm;
    u32 bodyLen;
    u16 stsCode, aid, cam_adrs;
    ELEMENT_CHECKER elementCheck;

    bodyLen = pFrm->FirmHeader.Length;
    pAssReq = &(pFrm->Body);
    aid = 0;

    if (bodyLen <= 2 + 2) {
        return;
    }

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_ASSREQ));

    if (wlMan->Work.Mode != WL_CMDLABEL_MODE_PARENT) {
        DbgPrint("RxErr AssReq(Mode:%u)\r\n", wlMan->Work.Mode);
        return;
    }

    if (IsExistManFrame(pFrm->Dot11Header.SA, FC_ASSRES)) {
        return;
    }

    cam_adrs = pFrm->FirmHeader.CamAdrs;
    do {
        if ((cam_adrs == 0) || (CAM_GetStaState(cam_adrs) < STA_CLASS2)) {
            if (IsExistManFrame(pFrm->Dot11Header.SA, FC_DEAUTH)) {
                return;
            }

            pTxDeAuthFrm = MakeDeAuthFrame(pFrm->Dot11Header.SA, WL_CMDLABEL_RSN_RX_CLASS2_FROM_NONAUTH_STA, TRUE);
            if ((u32)pTxDeAuthFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
                TxManCtrlFrame((LPTXFRM)pTxDeAuthFrm);
            }
            return;
        }

        if (CAM_GetStaState(cam_adrs) == STA_CLASS3) {
            CAM_SetStaState(cam_adrs, STA_CLASS2);

            MLME_IssueDisAssIndication(pFrm->Dot11Header.SA, MAN_STS_UNSPECIFIED);
        } else if (CAM_GetAID(cam_adrs) != 0) {
            DbgPrint("AssResponsing[%u]\r\n", cam_adrs);
            return;
        }

        MI_CpuClear32(&elementCheck, sizeof(elementCheck));
        elementCheck.pElement = pAssReq->Buf;
        elementCheck.bodyLength = bodyLen - (2 + 2);
        ElementChecker(&elementCheck);

        if ((pAssReq->CapaInfo.Data & BM_CAPA_NG_MASK_AP) || (pAssReq->CapaInfo.Bit.ESS == 0) || ((pConfig->WepMode == WL_CMDLABEL_WEP_NO) && (pAssReq->CapaInfo.Bit.Privacy == 1)) || ((pConfig->WepMode != WL_CMDLABEL_WEP_NO) && (pAssReq->CapaInfo.Bit.Privacy == 0)) || ((pConfig->PreambleType == WL_CMDLABEL_PREAMBLE_SHORT) && (pAssReq->CapaInfo.Bit.ShortPreamble == 0))) {
            stsCode = MAN_STS_NOT_SUPPORT_CAPABILITY;
            break;
        }
        CAM_SetCapaInfo(cam_adrs, pAssReq->CapaInfo.Data);

        if ((elementCheck.matchFlag & B_MATCH_SSID) == 0) {
            stsCode = MAN_STS_UNSPECIFIED;
            break;
        }

        if ((elementCheck.matchFlag & B_MATCH_RATESET) == 0) {
            stsCode = MAN_STS_INVALID_BASICRATESET;
            break;
        }

        CAM_SetSupRate(cam_adrs, elementCheck.rateSet.Support);

        stsCode = MAN_STS_SUCCESSFUL;
    } while (FALSE);

    pTxFrm = MakeAssResFrame(cam_adrs, stsCode, elementCheck.pSSID);
    if ((u32)pTxFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
        TxManCtrlFrame((LPTXFRM)pTxFrm);
    }
}

static void RxAssResFrame(LPASSRES_FRAME pFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPASSRES_BODY pAssRes;
    u32 bodyLen;

    bodyLen = pFrm->FirmHeader.Length;
    pAssRes = &pFrm->Body;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_ASSRES));

    if ((pWork->Mode != WL_CMDLABEL_MODE_CHILD) && (pWork->Mode != WL_CMDLABEL_MODE_HOTSPOT)) {
        DbgPrint("RxErr AssRes(Mode:%u)\r\n", wlMan->Work.Mode);
        return;
    }

    if (pMLME->State == MLME_STATE_ASS_ING) {
        if (MatchMacAdrs(pMLME->pReq.Ass->peerMacAdrs, pFrm->Dot11Header.SA)) {
            ClearTimeOut();

            if (pAssRes->StatusCode == MAN_STS_SUCCESSFUL) {
                WSetAids(pAssRes->AID & 0x0FFF);

                MakePsPollFrame(pWork->AID);

                WSetMacAdrs1(pWork->LinkAdrs, pFrm->Dot11Header.SA);

                pWork->APCamAdrs = CAM_Search(pFrm->Dot11Header.SA);
                CAM_SetStaState(pWork->APCamAdrs, STA_CLASS3);
            }

            if (pAssRes->StatusCode == WL_CMDRES_SUCCESS) {
                pMLME->pCfm.Ass->resultCode = WL_CMDRES_SUCCESS;
                pMLME->pCfm.Ass->statusCode = WL_CMDRES_SUCCESS;
                WSetStaState(STA_CLASS3);
            } else {
                pMLME->pCfm.Ass->resultCode = WL_CMDRES_FAILURE;
                pMLME->pCfm.Ass->statusCode = pAssRes->StatusCode;
            }
            pMLME->pCfm.Ass->aid = pWork->AID;

            pMLME->State = MLME_STATE_ASS_FIN;
            AddTask(TASK_NORMAL_PRIORITY, MLME_ASS_TASK_ID);
        }
    }
}

static void RxReAssReqFrame(LPREASSREQ_FRAME pFrm)
{
    LPWL_MAN pWlMan = (LPWL_MAN)&wlMan->TaskMan;
    LPREASSREQ_BODY pReAssReq;
    LPDEAUTH_FRAME pTxDeAuthFrm;
    LPREASSRES_FRAME pTxFrm;
    u32 bodyLen;
    u16 stsCode, aid, cam_adrs;
    ELEMENT_CHECKER elementCheck;

    bodyLen = pFrm->FirmHeader.Length;
    pReAssReq = &pFrm->Body;
    aid = 0;

    if (bodyLen <= 2 + 2 + 6) {
        return;
    }

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_REASSREQ));

    if (pWlMan->Work.Mode != WL_CMDLABEL_MODE_PARENT) {
        DbgPrint("RxErr ReAssReq(Mode:%u)\r\n", wlMan->Work.Mode);
        return;
    }

    if (IsExistManFrame(pFrm->Dot11Header.SA, FC_REASSRES)) {
        return;
    }

    cam_adrs = pFrm->FirmHeader.CamAdrs;
    do {
        if (CAM_GetStaState(cam_adrs) < STA_CLASS2) {
            if (IsExistManFrame(pFrm->Dot11Header.SA, FC_DEAUTH)) {
                return;
            }

            pTxDeAuthFrm = MakeDeAuthFrame(pFrm->Dot11Header.SA, MAN_RSN_RX_CLASS2_FROM_NONAUTH_STA, TRUE);
            if ((u32)pTxDeAuthFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
                TxManCtrlFrame((LPTXFRM)pTxDeAuthFrm);
            }
            return;
        }

        if (CAM_GetStaState(cam_adrs) == STA_CLASS3) {
            CAM_SetStaState(cam_adrs, STA_CLASS2);

            MLME_IssueDisAssIndication(pFrm->Dot11Header.SA, MAN_STS_UNSPECIFIED);
        } else if (CAM_GetAID(cam_adrs) != 0) {
            DbgPrint("(Re)AssResponsing[%u]\r\n", cam_adrs);
            return;
        }

        MI_CpuClear32(&elementCheck, sizeof(elementCheck));
        elementCheck.pElement = pReAssReq->Buf;
        elementCheck.bodyLength = bodyLen - (2 + 2 + 6);
        elementCheck.foundFlag = B_FOUND_BCSSID;
        ElementChecker(&elementCheck);

        if ((pReAssReq->CapaInfo.Data & BM_CAPA_NG_MASK_AP) || ((pWlMan->Config.WepMode == WL_CMDLABEL_WEP_NO) && (pReAssReq->CapaInfo.Bit.Privacy == 1)) || ((pWlMan->Config.WepMode != WL_CMDLABEL_WEP_NO) && (pReAssReq->CapaInfo.Bit.Privacy == 0))) {
            stsCode = MAN_STS_NOT_SUPPORT_CAPABILITY;
            break;
        }
        CAM_SetCapaInfo(cam_adrs, pReAssReq->CapaInfo.Data);

        if ((elementCheck.matchFlag & B_MATCH_SSID) == 0) {
            stsCode = MAN_STS_UNSPECIFIED;
            break;
        }

        if ((elementCheck.matchFlag & B_MATCH_RATESET) == 0) {
            stsCode = MAN_STS_INVALID_BASICRATESET;
            break;
        }

        CAM_SetSupRate(cam_adrs, elementCheck.rateSet.Support);

        stsCode = MAN_STS_SUCCESSFUL;

    } while (FALSE);

    pTxFrm = MakeReAssResFrame(cam_adrs, stsCode, elementCheck.pSSID);
    if ((u32)pTxFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
        TxManCtrlFrame((LPTXFRM)pTxFrm);
    }
}

static void RxReAssResFrame(LPREASSRES_FRAME pFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPREASSRES_BODY pReAssRes;
    u32 bodyLen;

    bodyLen = pFrm->FirmHeader.Length;
    pReAssRes = &pFrm->Body;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_REASSRES));

    if ((pWork->Mode != WL_CMDLABEL_MODE_CHILD) && (pWork->Mode != WL_CMDLABEL_MODE_HOTSPOT)) {
        DbgPrint("RxErr AssRes(Mode:%u)\r\n", pWork->Mode);
        return;
    }

    if (pMLME->State == MLME_STATE_REASS_ING) {
        if (MatchMacAdrs(pMLME->pReq.ReAss->newApMacAdrs, pFrm->Dot11Header.SA)) {
            ClearTimeOut();

            if (pReAssRes->StatusCode == MAN_STS_SUCCESSFUL) {
                WSetAids(pReAssRes->AID & 0x0FFF);

                MakePsPollFrame(pWork->AID);

                WSetMacAdrs1(pWork->LinkAdrs, pFrm->Dot11Header.SA);

                pWork->APCamAdrs = CAM_Search(pFrm->Dot11Header.SA);

                CAM_SetStaState(pWork->APCamAdrs, STA_CLASS3);

                WSetStaState(STA_CLASS3);
            }

            if (pReAssRes->StatusCode == WL_CMDRES_SUCCESS) {
                pMLME->pCfm.ReAss->resultCode = WL_CMDRES_SUCCESS;
                pMLME->pCfm.ReAss->statusCode = WL_CMDRES_SUCCESS;
                WSetStaState(STA_CLASS3);
            } else {
                pMLME->pCfm.ReAss->resultCode = WL_CMDRES_FAILURE;
                pMLME->pCfm.ReAss->statusCode = pReAssRes->StatusCode;
            }
            pMLME->pCfm.ReAss->aid = pWork->AID;

            pMLME->State = MLME_STATE_REASS_FIN;
            AddTask(TASK_NORMAL_PRIORITY, MLME_REASS_TASK_ID);
        }
    }
}

static void RxProbeReqFrame(LPPRBREQ_FRAME pFrm)
{
    LPPRBRES_FRAME pTxFrm;
    ELEMENT_CHECKER elementCheck;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_PRBREQ));

    if (IsExistManFrame(pFrm->Dot11Header.SA, FC_PRBRES)) {
        return;
    }

    if ((pFrm->Dot11Header.BSSID[0] & 0x0001) || (pFrm->MacHeader.Rx.Status & RXSTS_MATCHBSSID)) {
        MI_CpuClear32(&elementCheck, sizeof(elementCheck));
        elementCheck.pElement = pFrm->Body.Buf;
        elementCheck.bodyLength = pFrm->FirmHeader.Length;
        if (wlMan->Config.BcSsidResponse == WL_CMDLABEL_RESPONSE_BC_PRBREQ) {
            elementCheck.foundFlag = B_FOUND_BCSSID;
        }
        ElementChecker(&elementCheck);

        if ((elementCheck.matchFlag & B_MATCH_SSID) == B_MATCH_SSID) {
            pTxFrm = MakeProbeResFrame(pFrm->Dot11Header.SA);
            if ((u32)pTxFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
                TxManCtrlFrame((LPTXFRM)pTxFrm);
            }
        }
    }
}

static void RxProbeResFrame(LPPRBRES_FRAME pFrm, LPELEMENT_CHECKER pChk)
{
    LPMLME_MAN pMLME = &wlMan->MLME;
    WlMlmeScanCfm *pCfm;
    LPPRBRES_BODY pPrbRes;
    WlBssDesc *pDesc;
    u32 bodyLen, i, j, id, len;
    ELEMENT_CHECKER elementCheck;
    u16 *pBssidMask;
    u8 *pSrc, *pDst;

    if (pMLME->State != MLME_STATE_SCAN_ING) {
        return;
    }

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_PRBRES));

    pCfm = pMLME->pCfm.Scan;
    pPrbRes = &pFrm->Body;

    pBssidMask = pMLME->pReq.Scan->bssidMask;
    for (i = 0; i < pMLME->pReq.Scan->bssidMaskCount; i++) {
        if (MatchMacAdrs(pBssidMask, pFrm->Dot11Header.BSSID)) {
            pCfm->foundMap |= (1L << i);
            return;
        }
        pBssidMask += 3;
    }

    pDesc = &pCfm->bssDescList[0];
    for (i = 0; i < pCfm->bssDescCount; i++) {
        if (MatchMacAdrs(pFrm->Dot11Header.BSSID, pDesc->bssid)) {
            return;
        }
        pDesc = (WlBssDesc *)((u32)pDesc + pDesc->length * 2);
    }

    WLLIB_DmaClear16((u32)pDesc, MIN_BSSDESC_LENGTH);

    bodyLen = pFrm->FirmHeader.Length;
    if (bodyLen <= 8 + 2 + 2) {
        return;
    }

    if (pChk == NULL) {
        MI_CpuClear32(&elementCheck, sizeof(elementCheck));
        elementCheck.pElement = pPrbRes->Buf;
        elementCheck.bodyLength = bodyLen - (8 + 2 + 2);
        elementCheck.matchFlag = B_MATCH_SSID | B_MATCH_CHANNEL;
        elementCheck.foundFlag = B_FOUND_BSSID | B_FOUND_CAPA;
        elementCheck.rxStatus = pFrm->MacHeader.Rx.Status;
        elementCheck.capability = pPrbRes->CapaInfo.Data;
        ElementChecker(&elementCheck);

        pChk = &elementCheck;
    }

    DbgWlPrint(B_WL_DBG_MLME, "[SPrbMatch:%x]", pChk->matchFlag);

    if (pChk->pGMIF != NULL) {
        pDesc->gameInfoLength = WL_ReadByte(&pChk->pGMIF->Length) - 3 - 1 - 4;
        pDesc->length = (MIN_BSSDESC_LENGTH + pDesc->gameInfoLength + 1) / 2;
    } else {
        pDesc->length = (MIN_BSSDESC_LENGTH + pChk->otherElementLength + 1) / 2;
    }

    if (((pChk->matchFlag & B_MATCH_SSID) == B_MATCH_SSID) && (wlMan->MLME.Work.Scan.MaxConfirmLength >= pDesc->length)) {
        pDesc->capaInfo = pPrbRes->CapaInfo.Data;

        WSetMacAdrs1(pDesc->bssid, pFrm->Dot11Header.BSSID);

        pDesc->beaconPeriod = pPrbRes->BeaconInterval;

        pDesc->rssi = LoadLow(&pFrm->MacHeader.Rx.rsv_RSSI);

        if (pChk->pGMIF != NULL) {
            for (j = 0; j < pDesc->gameInfoLength; j++) {
                WL_WriteByte(&pDesc->gameInfo[j], WL_ReadByte(&pChk->pGMIF->GameInfo[j]));
            }
        } else {
            pDesc->otherElementCount = pChk->otherElementCount;
            if (pChk->otherElementCount != 0) {
                pSrc = (u8 *)pPrbRes->Buf;
                pDst = (u8 *)pDesc->otherElement;
                for (i = 0; i < pChk->otherElementCount;) {
                    id = WL_ReadByte(&pSrc[0]);
                    len = WL_ReadByte(&pSrc[1]);
                    if ((id > ID_IBSS_PARAM_ELEMENT) && ((u32)pSrc != (u32)pChk->pGMIF)) {
                        for (j = 0; j < len + 2; j++) {
                            WL_WriteByte(pDst, WL_ReadByte(pSrc));
                            pDst++;
                            pSrc++;
                        }
                        i++;
                    } else {
                        pSrc += (len + 2);
                    }
                }
            }
        }

        if (pChk->pSSID != NULL) {
            pDesc->ssidLength = WL_ReadByte(&pChk->pSSID->Length);
            for (j = 0; j < pDesc->ssidLength; j++) {
                WL_WriteByte(&pDesc->ssid[j], WL_ReadByte(&pChk->pSSID->SSID[j]));
            }
        } else {
            pDesc->ssidLength = 0;
            for (j = 0; j < MAX_SSID_LENGTH; j++) {
                WL_WriteByte(&pDesc->ssid[j], 0);
            }
        }

        pDesc->rateSet.basic = pChk->rateSet.Basic;
        pDesc->rateSet.support = pChk->rateSet.Support;

        pDesc->channel = pChk->channel;

        if (pChk->pCFP != NULL) {
            pDesc->cfpPeriod = WL_ReadByte(&pChk->pCFP->CFPPeriod);
        }

        if (pChk->pTIM != NULL) {
            pDesc->dtimPeriod = WL_ReadByte(&pChk->pTIM->DTIMPeriod);
        }

        pCfm->header.length += pDesc->length;
        pCfm->bssDescCount++;
        pMLME->Work.Scan.MaxConfirmLength -= pDesc->length;

        if (pMLME->Work.Scan.MaxConfirmLength < MIN_BSSDESC_LENGTH / 2) {
            ClearTimeOut();

            pMLME->State = MLME_STATE_SCAN_FIN;

            AddTask(TASK_NORMAL_PRIORITY, MLME_SCAN_TASK_ID);
        }
    } else {
        WSetMacAdrs1(pDesc->bssid, (u16 *)NULL_ADRS);
    }
}

static void RxAuthFrame(LPAUTH_FRAME pFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPMLME_MAN pMLME = &wlMan->MLME;
    LPAUTH_BODY pAuth;
    LPAUTH_FRAME pTxFrm;
    u32 bodyLen, cam_adrs;
    u32 bTxAuth;
    u16 seqNum, txtLen, stsCode;

    bodyLen = pFrm->FirmHeader.Length;
    pAuth = &pFrm->Body;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTH));

    if (wlMan->WlOperation & MAC_BUG_WEP) {
        LPTXQ pTxq = wlMan->TxCtrl.Txq;
        u16 map = *(vu16 *)MREG_TXQ_MAP;

        if ((((map & TXQ_NBLOCK_TXQ0) == 0) || (pTxq[0].Busy == 0)) && (((map & TXQ_NBLOCK_TXQ1) == 0) || (pTxq[1].Busy == 0)) && (((map & TXQ_NBLOCK_TXQ2) == 0) || (pTxq[2].Busy == 0)) && ((*(vu16 *)MREG_SIGNAL_STATE & SIGNAL_STATE_CCA) == 0)) {
            *(vu16 *)MREG_WEP_CONFIG = 0x0000;
            *(vu16 *)MREG_WEP_CONFIG = 0x8000;

            wlMan->RxCtrl.IcvOkCntFlag = 0;
        }
    }

    if (IsExistManFrame(pFrm->Dot11Header.SA, FC_AUTH)) {
        return;
    }

    bTxAuth = FALSE;
    txtLen = 0;
    seqNum = pAuth->SeqNum + 1;

    cam_adrs = pFrm->FirmHeader.CamAdrs;
    if (cam_adrs == 0) {
        stsCode = MAN_STS_NO_ENTRY;
        bTxAuth = TRUE;
    } else {
        if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
            if (CAM_GetStaState(cam_adrs) > STA_CLASS1) {
                CAM_SetStaState(cam_adrs, STA_CLASS1);

                MLME_IssueDeAuthIndication(pFrm->Dot11Header.SA, MAN_STS_UNSPECIFIED);
            }

            if ((pFrm->MacHeader.Rx.Status & RXSTS_ICV_ERR) && (CAM_GetAuthSeed(cam_adrs) != 0)) {
                pAuth->AlgoType = WL_CMDLABEL_AUTH_SHARED_KEY;
                stsCode = MAN_STS_CHALLENGE_FAILURE;
                seqNum = 4;
                bTxAuth = TRUE;

                CAM_SetAuthSeed(cam_adrs, 0);
                goto tx_auth;
            }
        }

        switch (pAuth->AlgoType) {
        case WL_CMDLABEL_AUTH_OPEN_SYSTEM:

            if ((pWork->Mode == WL_CMDLABEL_MODE_PARENT) && (wlMan->Config.AuthAlgo == WL_CMDLABEL_AUTH_SHARED_KEY)) {
                stsCode = MAN_STS_NOT_SUPPORT_AUTH_ALGORITHM;
                bTxAuth = TRUE;
                break;
            }

            if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
                if (pAuth->SeqNum == 1) {
                    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTH1));

                    stsCode = MAN_STS_SUCCESSFUL;
                    bTxAuth = TRUE;
                } else {
                    stsCode = MAN_STS_OUT_OF_AUTH_SEQ_NUM;
                    seqNum = 2;
                    bTxAuth = TRUE;
                }
            }

            else if ((pWork->Mode != WL_CMDLABEL_MODE_PARENT) && (pAuth->SeqNum == 2)) {
                DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTH2));

                if ((pMLME->pReq.Auth->algorithm == WL_CMDLABEL_AUTH_OPEN_SYSTEM) && MatchMacAdrs(pMLME->pReq.Auth->peerMacAdrs, pFrm->Dot11Header.SA)) {
                    if (pMLME->State == MLME_STATE_AUTH_ING) {
                        ClearTimeOut();

                        if (pAuth->StatusCode == WL_CMDRES_SUCCESS) {
                            WSetStaState(STA_CLASS2);

                            pMLME->pCfm.Auth->resultCode = WL_CMDRES_SUCCESS;
                            pMLME->pCfm.Auth->statusCode = WL_CMDRES_SUCCESS;
                        } else {
                            pMLME->pCfm.Auth->resultCode = WL_CMDRES_FAILURE;
                            pMLME->pCfm.Auth->statusCode = pAuth->StatusCode;
                        }

                        pMLME->State = MLME_STATE_AUTH_FIN;
                        AddTask(TASK_NORMAL_PRIORITY, MLME_AUTH_TASK_ID);
                    }
                } else {
                }
            }
            break;

        case WL_CMDLABEL_AUTH_SHARED_KEY:

            if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
                CAM_SetStaState(cam_adrs, STA_CLASS1);

                if (pAuth->SeqNum == 1) {
                    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTH1));

                    pTxFrm = MakeAuthFrame(pFrm->Dot11Header.SA, CHALLENGE_TXT_LENGTH, TRUE);
                    if ((u32)pTxFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
                        pTxFrm->Body.AlgoType = pAuth->AlgoType;
                        pTxFrm->Body.SeqNum = seqNum;
                        pTxFrm->Body.StatusCode = 0;
                        SetChallengeText(cam_adrs, pTxFrm);

                        TxManCtrlFrame((LPTXFRM)pTxFrm);
                    }
                }

                else if (pAuth->SeqNum == 3) {
                    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTH3));

                    if ((CAM_GetStaState(cam_adrs) != STA_CLASS1) || (CAM_GetAuthSeed(cam_adrs) == 0)) {
                        stsCode = MAN_STS_UNSPECIFIED;
                        bTxAuth = TRUE;
                        break;
                    }

                    if (CheckChallengeText(pFrm) == FALSE) {
                        stsCode = MAN_STS_CHALLENGE_FAILURE;
                        bTxAuth = TRUE;

                        CAM_SetAuthSeed(cam_adrs, 0);
                        break;
                    }

                    CAM_SetAuthSeed(cam_adrs, 0);

                    stsCode = MAN_STS_SUCCESSFUL;
                    bTxAuth = TRUE;

                }

                else {
                    CAM_SetAuthSeed(cam_adrs, 0);

                    stsCode = MAN_STS_OUT_OF_AUTH_SEQ_NUM;
                    seqNum = 2;
                    bTxAuth = TRUE;
                }
                break;
            }

            else {
                if ((pMLME->pReq.Auth->algorithm != WL_CMDLABEL_AUTH_SHARED_KEY) || (MatchMacAdrs(pMLME->pReq.Auth->peerMacAdrs, pFrm->Dot11Header.SA) == 0)) {
                    break;
                }

                if (pAuth->SeqNum == 2) {
                    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTH2));

                    if (pMLME->State == MLME_STATE_AUTH_ING) {
                        if (pAuth->StatusCode != MAN_STS_SUCCESSFUL) {
                            ClearTimeOut();

                            pMLME->State = MLME_STATE_AUTH_FIN;
                            pMLME->pCfm.Auth->resultCode = WL_CMDRES_FAILURE;
                            pMLME->pCfm.Auth->statusCode = pAuth->StatusCode;

                            AddTask(TASK_NORMAL_PRIORITY, MLME_AUTH_TASK_ID);

                            WSetStaState(STA_CLASS1);
                        } else {
                            pMLME->State = MLME_STATE_AUTH_ING2;

                            pTxFrm = MakeAuthFrame(pFrm->Dot11Header.SA, WL_ReadByte(&pFrm->Body.ChallengeText.Length), TRUE);
                            if ((u32)pTxFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
                                pTxFrm->Dot11Header.FrameCtrl.Bit.WEP = 1;

                                WLLIB_DmaCopy16((u32)&pFrm->Body, (u32)&pTxFrm->Body, pFrm->FirmHeader.Length + 1);

                                pTxFrm->Body.AlgoType = pAuth->AlgoType;
                                pTxFrm->Body.SeqNum = 3;
                                pTxFrm->Body.StatusCode = 0;

                                TxManCtrlFrame((LPTXFRM)pTxFrm);
                            }
                        }
                    }
                }

                else if (pAuth->SeqNum == 4) {
                    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTH4));

                    if (pMLME->State == MLME_STATE_AUTH_ING2) {
                        ClearTimeOut();

                        if (pAuth->StatusCode == WL_CMDRES_SUCCESS) {
                            WSetStaState(STA_CLASS2);

                            pMLME->pCfm.Auth->resultCode = WL_CMDRES_SUCCESS;
                            pMLME->pCfm.Auth->statusCode = WL_CMDRES_SUCCESS;
                        } else {
                            pMLME->pCfm.Auth->resultCode = WL_CMDRES_FAILURE;
                            pMLME->pCfm.Auth->statusCode = pAuth->StatusCode;
                        }

                        pMLME->State = MLME_STATE_AUTH_FIN;

                        AddTask(TASK_NORMAL_PRIORITY, MLME_AUTH_TASK_ID);
                    }
                }

                else {
                }
            }
            break;

        default:
            DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_AUTHX));

            if (pWork->Mode == WL_CMDLABEL_MODE_PARENT) {
                stsCode = MAN_STS_NOT_SUPPORT_AUTH_ALGORITHM;
                bTxAuth = TRUE;
            }
            break;
        }
    }

tx_auth:
    if (bTxAuth) {
        pTxFrm = MakeAuthFrame(pFrm->Dot11Header.SA, txtLen, (stsCode != MAN_STS_SUCCESSFUL));
        if ((u32)pTxFrm != HEAPBUF_NOT_ENOUGH_MEMORY) {
            pTxFrm->Body.AlgoType = pAuth->AlgoType;
            pTxFrm->Body.SeqNum = seqNum;
            pTxFrm->Body.StatusCode = stsCode;

            TxManCtrlFrame((LPTXFRM)pTxFrm);
        }
    }
}

static void RxDeAuthFrame(LPDEAUTH_FRAME pFrm)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPDEAUTH_BODY pDeAuth;
    u32 bodyLen, cam_adrs;

    bodyLen = pFrm->FirmHeader.Length;
    pDeAuth = &pFrm->Body;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_DEAUTH));

    cam_adrs = pFrm->FirmHeader.CamAdrs;

    switch (pWork->Mode) {
    case WL_CMDLABEL_MODE_PARENT:

        if (CAM_GetStaState(cam_adrs) > STA_CLASS1) {
            CAM_SetStaState(cam_adrs, STA_CLASS1);

            MLME_IssueDeAuthIndication(pFrm->Dot11Header.SA, pFrm->Body.ReasonCode);

            DeleteTxFrames(cam_adrs);
        }
        break;

    case WL_CMDLABEL_MODE_CHILD:
    case WL_CMDLABEL_MODE_HOTSPOT:

        if ((pWork->STA > STA_CLASS1) && (MatchMacAdrs(pFrm->Dot11Header.SA, pWork->LinkAdrs))) {
            WSetStaState(STA_CLASS1);

            WClearAids();

            MLME_IssueDeAuthIndication(pFrm->Dot11Header.SA, pFrm->Body.ReasonCode);
            break;
        }
    }
}

static void RxPsPollFrame(LPPSPOLL_FRAME pFrm)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    u32 cam_adrs;

    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_PSPOLL));

    cam_adrs = pFrm->FirmHeader.CamAdrs;

    if (CAM_GetStaState(cam_adrs) == STA_CLASS3) {
        CAM_SetAwake(cam_adrs);

        if (GetHeapBufCount(&pHeapMan->TxPri[QID_MANCTRL]) != 0) {
            TxqPri(QID_MANCTRL);
        }
        if (GetHeapBufCount(&pHeapMan->TxPri[QID_DATA]) != 0) {
            TxqPri(QID_DATA);
        }
    }
}

static void RxCfEndFrame(LPCFEND_FRAME pFrm)
{
    DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_CFEND));
}

static void ElementChecker(LPELEMENT_CHECKER p)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    u8 *pBuf;
    s32 i;
    u32 id, len;

    pBuf = p->pElement;
    p->channel = pWork->CurrChannel;

    if (p->foundFlag & B_FOUND_BCSSID) {
        p->matchFlag |= B_MATCH_SSID;
    }

    for (i = p->bodyLength; i > 0;) {
        id = WL_ReadByte(pBuf++);
        len = WL_ReadByte(pBuf++);

        switch (id) {

        case ID_SSID_ELEMENT:
            if (len <= MAX_SSID_LENGTH) {
                p->foundFlag |= B_FOUND_SSID;
                p->pSSID = (LPSSID_ELEMENT)&pBuf[-2];
                if ((len == 0) && (p->foundFlag & B_FOUND_BCSSID)) {
                    p->matchFlag |= B_MATCH_SSID;
                } else {
                    p->matchFlag &= ~B_MATCH_SSID;
                    if (WCheckSSID(len, pBuf)) {
                        p->matchFlag |= B_MATCH_SSID;
                    }
                }
            }
            break;

        case ID_SUPRATE_ELEMENT:
            if (len < 1) {
                break;
            }
            p->foundFlag |= B_FOUND_RATESET;
            WElement2RateSet((LPSUP_RATE_ELEMENT)&pBuf[-2], &p->rateSet);
            if (((p->rateSet.Basic & ~(pWork->RateSet.Basic | pWork->RateSet.Support)) == 0) && (((p->rateSet.Basic | p->rateSet.Support) & pWork->RateSet.Basic) == pWork->RateSet.Basic)) {
                p->matchFlag |= B_MATCH_RATESET;
            } else {
                p->matchFlag &= ~B_MATCH_RATESET;
            }
            break;

        case ID_FH_PARAM_ELEMENT:
            break;

        case ID_DS_PARAM_ELEMENT:
            if (len < 1) {
                break;
            }
            p->foundFlag |= B_FOUND_CHANNEL;
            p->channel = WL_ReadByte(&pBuf[0]);
            if (p->channel == wlMan->MLME.pReq.Join->bssDesc.channel) {
                p->matchFlag |= B_MATCH_CHANNEL;
            } else {
                p->matchFlag &= ~B_MATCH_CHANNEL;
            }
            break;

        case ID_TIM_ELEMENT:
            if (len < 3) {
                break;
            }
            p->foundFlag |= B_FOUND_TIM;
            p->pTIM = (LPTIM_ELEMENT)&pBuf[-2];
            break;

        case ID_CF_PARAM_ELEMENT:
            if (len < 6) {
                break;
            }
            p->foundFlag |= B_FOUND_CFP;
            p->pCFP = (LPCF_PARAM_ELEMENT)&pBuf[-2];
            break;

        case ID_IBSS_PARAM_ELEMENT:
            break;

        case ID_GAME_INFO_ELEMENT:
            if ((len < 3 + 1 + 4) || (WL_ReadByte(&pBuf[0]) != GAME_INFO_OUI_0) || (WL_ReadByte(&pBuf[1]) != GAME_INFO_OUI_1) || (WL_ReadByte(&pBuf[2]) != GAME_INFO_OUI_2) || (WL_ReadByte(&pBuf[3]) != GAME_INFO_SUBTYPE)) {
                p->otherElementCount++;
                p->otherElementLength += (len + 2);
            } else {
                p->foundFlag |= B_FOUND_GAMEINFO;
                p->pGMIF = (LPGAME_INFO_ELEMENT)&pBuf[-2];
            }
            break;

        default:
            p->otherElementCount++;
            p->otherElementLength += (len + 2);
        }

        pBuf += len;
        i -= (len + 2);
    }

    if (p->foundFlag & B_FOUND_BSSID) {
        if ((pWork->BSSID[0] & 0x0001) || (p->rxStatus & RXSTS_MATCHBSSID)) {
            p->matchFlag |= B_MATCH_BSSID;
        }
    }

    if (p->foundFlag & B_FOUND_CAPA) {
        if ((p->capability & (B_CAPA_ESS | B_CAPA_IBSS)) == (pWork->CapaInfo & (B_CAPA_ESS | B_CAPA_IBSS))) {
            p->matchFlag |= B_MATCH_CAPA_ESS;
        }
        if ((p->capability & B_CAPA_PRIVACY) == (pWork->CapaInfo & B_CAPA_PRIVACY)) {
            p->matchFlag |= B_MATCH_CAPA_WEP;
        }
    }
}

void RxManCtrlTask(void)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    WlCounter *pCounter = &wlMan->Counter;
    u32 mode = wlMan->Work.Mode;
    LPRXFRM pFrm;
    LPRXPACKET pPacket;
    LPMAN_HEADER pDot11Header;
    u32 cam_adrs, type, subtype;

    pPacket = (LPRXPACKET)GetHeapBufHeadAdrs(&pHeapMan->RxManCtrl);
    if ((u32)pPacket == HEAPBUF_HEAD_NONE) {
        return;
    }
    pFrm = &pPacket->frame;
    pDot11Header = (LPMAN_HEADER)&pFrm->Dot11Header;

    if (pFrm->Dot11Header.Adrs1[0] & 0x0001) {
        pCounter->rx.multicast++;
    } else {
        pCounter->rx.unicast++;
    }
    pCounter->rx.fragment += (GetFragCount(pFrm->MacHeader.Rx.Status) - 1);

    type = pDot11Header->FrameCtrl.Bit.Type;
    subtype = pDot11Header->FrameCtrl.Bit.SubType;

    cam_adrs = CAM_SearchAdd(pDot11Header->SA);
    pFrm->FirmHeader.CamAdrs = cam_adrs;
    switch (cam_adrs) {
    case CAM_NOT_FOUND:
        cam_adrs = 0;
        pFrm->FirmHeader.CamAdrs = 0;

        switch (mode) {
        case WL_CMDLABEL_MODE_PARENT:
            if (type == TYPE_MANAGEMENT) {
                switch (subtype) {
                case SUBTYPE_AUTH:
                    RxAuthFrame((LPAUTH_FRAME)pFrm);
                    break;
                case SUBTYPE_PRBREQ:
                    RxProbeReqFrame((LPPRBREQ_FRAME)pFrm);
                    break;
                case SUBTYPE_ASSREQ:
                    RxAssReqFrame((LPASSREQ_FRAME)pFrm);
                    break;
                }
            }
        }
        break;

    default:
        CAM_UpdateLifeTime(cam_adrs);
        CAM_SetRSSI(cam_adrs, LoadLow(&pFrm->MacHeader.Rx.rsv_RSSI));

        if (type == TYPE_MANAGEMENT) {
            u32 seqctrl = pFrm->Dot11Header.SeqCtrl.Data;

            if (seqctrl == CAM_GetLastSeqCtrl(cam_adrs)) {
                DbgWlPutchar(B_WL_DBG_MANCTRL, (WL_DBG_RX_DUP));

                pCounter->rx.duplicateErr++;
                break;
            }
            CAM_SetLastSeqCtrl(cam_adrs, seqctrl);
        }

        switch (mode) {
        case WL_CMDLABEL_MODE_PARENT:

        {
            CAM_SetPowerMgtMode(cam_adrs, pDot11Header->FrameCtrl.Bit.PowerMan);
        }

            if (type == TYPE_MANAGEMENT) {
                switch (subtype) {
                case SUBTYPE_BEACON:
                    RxBeaconFrame((LPBEACON_FRAME)pFrm);
                    break;
                case SUBTYPE_ASSREQ:
                    RxAssReqFrame((LPASSREQ_FRAME)pFrm);
                    break;
                case SUBTYPE_REASSREQ:
                    RxReAssReqFrame((LPREASSREQ_FRAME)pFrm);
                    break;
                case SUBTYPE_PRBREQ:
                    RxProbeReqFrame((LPPRBREQ_FRAME)pFrm);
                    break;
                case SUBTYPE_PRBRES:
                    RxProbeResFrame((LPPRBRES_FRAME)pFrm, NULL);
                    break;
                case SUBTYPE_DISASS:
                    RxDisAssFrame((LPDISASS_FRAME)pFrm);
                    break;
                case SUBTYPE_AUTH:
                    RxAuthFrame((LPAUTH_FRAME)pFrm);
                    break;
                case SUBTYPE_DEAUTH:
                    RxDeAuthFrame((LPDEAUTH_FRAME)pFrm);
                    break;
                default:
                    DbgPrint("Unknown Subtype(MAN):%x\r\n", subtype);
                }
            } else if (type == TYPE_CONTROL) {
                switch (subtype) {
                case SUBTYPE_PSPOLL:
                    RxPsPollFrame((LPPSPOLL_FRAME)pFrm);
                    break;
                default:
                    DbgPrint("Unknown Subtype(CTRL):%x\r\n", subtype);
                }
            } else {
                DbgPrint("Unknown FC Type Frame(%08x)\r\n", (u32)pFrm);
            }
            break;

        case WL_CMDLABEL_MODE_CHILD:
        case WL_CMDLABEL_MODE_HOTSPOT:

            if (type == TYPE_MANAGEMENT) {
                switch (subtype) {
                case SUBTYPE_BEACON:
                    RxBeaconFrame((LPBEACON_FRAME)pFrm);
                    break;
                case SUBTYPE_ASSRES:
                    RxAssResFrame((LPASSRES_FRAME)pFrm);
                    break;
                case SUBTYPE_REASSRES:
                    RxReAssResFrame((LPREASSRES_FRAME)pFrm);
                    break;
                case SUBTYPE_PRBREQ:
                    break;
                case SUBTYPE_PRBRES:
                    RxProbeResFrame((LPPRBRES_FRAME)pFrm, NULL);
                    break;
                case SUBTYPE_DISASS:
                    RxDisAssFrame((LPDISASS_FRAME)pFrm);
                    break;
                case SUBTYPE_AUTH:
                    RxAuthFrame((LPAUTH_FRAME)pFrm);
                    break;
                case SUBTYPE_DEAUTH:
                    RxDeAuthFrame((LPDEAUTH_FRAME)pFrm);
                    break;
                default:
                    DbgPrint("Unknown Subtype(MAN):%x\r\n", subtype);
                }
            } else if (type == TYPE_CONTROL) {
                switch (subtype) {
                case SUBTYPE_CFEND:
                case SUBTYPE_CFEND_CFACK:
                    RxCfEndFrame((LPCFEND_FRAME)pFrm);
                    break;
                default:
                    DbgPrint("Unknown Subtype(CTRL):%x\r\n", subtype);
                }
            } else {
                DbgPrint("FC Err[%08x:%04x]\r\n", (u32)pFrm, pDot11Header->FrameCtrl.Data);
            }
            break;
        }
    }

    ReleaseHeapBuf(&pHeapMan->RxManCtrl, pPacket);

    if (GetHeapBufCount(&pHeapMan->RxManCtrl) != 0) {
        AddTask(TASK_HIGH_PRIORITY, RXMANCTRL_TASK_ID);
    }
}

static void SetChallengeText(u32 camAdrs, LPAUTH_FRAME pFrm)
{
    u32 i, txtLen;
    u16 *pText;
    u16 rnd;

    ASSERT(camAdrs == 0xFFFF);

    rnd = *(vu16 *)MREG_MSEQ16 + (*(vu16 *)MREG_MSEQ16 << 8);
    if (rnd == 0) {
        rnd = 1;
    }
    RND_seed(rnd);
    CAM_SetAuthSeed(camAdrs, rnd);

    pText = (u16 *)pFrm->Body.ChallengeText.Text;
    txtLen = WL_ReadByte(&pFrm->Body.ChallengeText.Length);
    for (i = 0; i < txtLen; i += 2) {
        *pText++ = RND_rand();
    }
}

static u32 CheckChallengeText(LPAUTH_FRAME pFrm)
{
    u32 i, txtLen;
    u16 *pText;

    RND_seed(CAM_GetAuthSeed(pFrm->FirmHeader.CamAdrs));

    pText = (u16 *)pFrm->Body.ChallengeText.Text;
    txtLen = WL_ReadByte(&pFrm->Body.ChallengeText.Length);
    for (i = 0; i < txtLen / 2; i++) {
        if (*pText++ != RND_rand()) {
            return FALSE;
        }
    }

    if (txtLen & 1) {
        if ((*pText & 0x00FF) != (RND_rand() & 0x00FF)) {
            return FALSE;
        }
    }

    return TRUE;
}

void DefragTask(void)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPRXPACKET pPacket;
    LPRXFRM pFrm;
    DEFRAG_TBL defragTbl;
    u32 fc;

    pPacket = (LPRXPACKET)GetHeapBufHeadAdrs(&pHeapMan->Defrag);
    if ((u32)pPacket == HEAPBUF_HEAD_NONE) {
        return;
    }
    pFrm = &pPacket->frame;

    if (wlMan->Work.STA != STA_CLASS3) {
        goto defrag_task_exit;
    }

    DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_IN));

    if (pFrm->MacHeader.Rx.MPDU > 24 + MAX_DEFRAG_DATASIZE) {
        DbgPuts("Defrag Limit Len\r");
        goto defrag_task_exit;
    }

    fc = pFrm->Dot11Header.FrameCtrl.Data;

    if (fc & B_FC_TODS) {
        WSetMacAdrs1(defragTbl.DA, pFrm->Dot11Header.Adrs3);
        if (fc & B_FC_FROMDS) {
            DbgPuts("defrag from/to ds err\r");
            goto defrag_task_exit;
        } else {
            WSetMacAdrs1(defragTbl.SA, pFrm->Dot11Header.Adrs2);
        }
    } else {
        WSetMacAdrs1(defragTbl.DA, pFrm->Dot11Header.Adrs1);
        if (fc & B_FC_FROMDS) {
            WSetMacAdrs1(defragTbl.SA, pFrm->Dot11Header.Adrs3);
        } else {
            WSetMacAdrs1(defragTbl.SA, pFrm->Dot11Header.Adrs2);
        }
    }
    defragTbl.SeqCtrl.Data = pFrm->Dot11Header.SeqCtrl.Data;

    if ((fc & B_FC_MOREFRAG) && (pFrm->Dot11Header.SeqCtrl.Bit.FragNum == 0)) {
        NewDefragment((LPRXFRM_MAC)&pFrm->MacHeader, &defragTbl);
    } else {
        MoreDefragment((LPRXFRM_MAC)&pFrm->MacHeader, &defragTbl);
    }

defrag_task_exit:
    ReleaseHeapBuf(&pHeapMan->Defrag, pPacket);

    if (GetHeapBufCount(&pHeapMan->Defrag) != 0) {
        AddTask(TASK_NORMAL_PRIORITY, DEFRAG_TASK_ID);
    }
}

static void NewDefragment(LPRXFRM_MAC pMFrm, LPDEFRAG_TBL pDefragTbl)
{
    LPDEFRAG_LIST pList = wlMan->RxCtrl.DefragList;
    LPRXPACKET pPacket;
    LPRXFRM pFrm;
    u32 i, pos, fragCnt, SrcOfst, WSize, OvrCnt;

    DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_NEW));

    pos = -1;
    for (i = 0; i < MAX_DEFRAG_NUM; i++) {
        if (pList[i].RestTime != 0) {
            if ((MatchMacAdrs(pList[i].DefragTbl.DA, pDefragTbl->DA)) && (MatchMacAdrs(pList[i].DefragTbl.SA, pDefragTbl->SA)) && (pList[i].DefragTbl.SeqCtrl.Bit.SeqNum == pDefragTbl->SeqCtrl.Bit.SeqNum)) {
                DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_DUP));

                fragCnt = GetFragCount(pMFrm->MacHeader.Rx.Status);
                OvrCnt = fragCnt - pList[i].DefragTbl.SeqCtrl.Bit.FragNum;
                if ((OvrCnt != 0) && (OvrCnt & 0x80000000) == 0) {
                    pFrm = &pList[i].pPacket->frame;

                    SrcOfst = pFrm->MacHeader.Rx.MPDU;
                    WSize = pMFrm->MacHeader.Rx.MPDU - SrcOfst - 24;
                    if ((WSize != 0) && ((WSize & 0x80000000) == 0)) {
                        WLLIB_DmaCopy16((u32)&pMFrm->Body[SrcOfst], (u32)&pFrm->Body[pFrm->MacHeader.Rx.MPDU], WSize);

                        pFrm->MacHeader.Rx.MPDU += WSize;

                        pList[i].DefragTbl.SeqCtrl.Bit.FragNum = fragCnt;
                        wlMan->Counter.rx.fragment += fragCnt;
                    }
                }

                return;
            }
        } else {
            pos = i;
        }
    }

    if (pos != -1) {
        pPacket = (LPRXPACKET)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, MAX_DEFRAG_PACKETSIZE);
        if ((u32)pPacket != HEAPBUF_NOT_ENOUGH_MEMORY) {
            DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_ADD));

            WLLIB_DmaCopy16((u32)pDefragTbl, (u32)&pList[pos].DefragTbl, sizeof(DEFRAG_TBL));
            pList[pos].RestTime = MAX_DEFRAG_LIFETIME;
            pList[pos].pPacket = pPacket;

            pFrm = &pPacket->frame;

            WLLIB_DmaCopy16((u32)pMFrm, (u32)&pFrm->MacHeader, pMFrm->MacHeader.Rx.MPDU + sizeof(MAC_HEADER));
            WLLIB_DmaWait();

            fragCnt = GetFragCount(pFrm->MacHeader.Rx.Status);
            pList[pos].DefragTbl.SeqCtrl.Bit.FragNum = fragCnt;
            wlMan->Counter.rx.fragment += fragCnt;

            pFrm->MacHeader.Rx.MPDU = pMFrm->MacHeader.Rx.MPDU - 24;
            pList[pos].UnitLength = pFrm->MacHeader.Rx.MPDU / fragCnt;
        } else {
            SetFatalErr(WL_FATAL_ERR_NOT_ENOUGH_MEMORY_DEFRAG);
        }
    }
}

static void MoreDefragment(LPRXFRM_MAC pMFrm, LPDEFRAG_TBL pDefragTbl)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPDEFRAG_LIST pList = wlMan->RxCtrl.DefragList;
    LPRXFRM pFrm;
    u32 i, fragCnt, length, OvrCnt, SrcOfst, WSize;

    DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_MORE));

    pMFrm->MacHeader.Rx.MPDU -= 24;

    for (i = 0; i < MAX_DEFRAG_NUM; i++) {
        if (pList[i].RestTime != 0) {
            if ((MatchMacAdrs(pList[i].DefragTbl.DA, pDefragTbl->DA)) && (MatchMacAdrs(pList[i].DefragTbl.SA, pDefragTbl->SA)) && (pList[i].DefragTbl.SeqCtrl.Bit.SeqNum == pDefragTbl->SeqCtrl.Bit.SeqNum)) {
                OvrCnt = pList[i].DefragTbl.SeqCtrl.Data - pDefragTbl->SeqCtrl.Data;

                if ((OvrCnt & 0x80000000) == 0) {
                    SrcOfst = OvrCnt * pList[i].UnitLength;
                    WSize = pMFrm->MacHeader.Rx.MPDU - SrcOfst;
                    if ((WSize != 0) && ((WSize & 0x80000000) == 0)) {
                        break;
                    }
                }

                return;
            }
        }
    }

    if (i != MAX_DEFRAG_NUM) {
        DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_FOUND));

        pFrm = &pList[i].pPacket->frame;

        length = pFrm->MacHeader.Rx.MPDU + WSize;
        if (length > MAX_DEFRAG_DATASIZE) {
            DbgPuts("DefragOvrLen\r");

            ReleaseHeapBuf(&pHeapMan->TmpBuf, CalcReqAdrsFromFrame(pFrm));

            pList[i].RestTime = 0;
            return;
        }

        WLLIB_DmaCopy16((u32)&pMFrm->Body[SrcOfst], (u32)&pFrm->Body[pFrm->MacHeader.Rx.MPDU], WSize + 1);

        pFrm->MacHeader.Rx.MPDU = length;

        fragCnt = GetFragCount(pMFrm->MacHeader.Rx.Status);
        pList[i].DefragTbl.SeqCtrl.Bit.FragNum += (fragCnt - OvrCnt);
        wlMan->Counter.rx.fragment += fragCnt;

        if ((pMFrm->MacHeader.Rx.Status & RXSTS_MOREFRAG) == 0) {
            DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_LAST));

            pList[i].RestTime = 0;

            pFrm->MacHeader.Rx.Status = (pFrm->MacHeader.Rx.Status & ~RXSTS_FRAGCNT_MASK) + (1 * RXSTS_FRAGCNT);

            pFrm->MacHeader.Rx.MPDU += 24;

            switch (pFrm->MacHeader.Rx.Status & RXSTS_FRAME_TYPE_MASK) {
            case RXSTS_DATA:
                MoveHeapBuf(&pHeapMan->TmpBuf, &pHeapMan->RxData, CalcReqAdrsFromFrame(pFrm));
                AddTask(TASK_NORMAL_PRIORITY, RXDATA_TASK_ID);
                break;

            case RXSTS_MAN:
                MoveHeapBuf(&pHeapMan->TmpBuf, &pHeapMan->RxManCtrl, CalcReqAdrsFromFrame(pFrm));
                AddTask(TASK_HIGH_PRIORITY, RXMANCTRL_TASK_ID);
                break;

            default:
                ReleaseHeapBuf(&pHeapMan->TmpBuf, CalcReqAdrsFromFrame(pFrm));
                break;
            }
        }
    }
}

void DefragTimerTask(void)
{
    LPDEFRAG_LIST pList = wlMan->RxCtrl.DefragList;
    u32 i;

    for (i = 0; i < MAX_DEFRAG_NUM; i++) {
        if (pList[i].RestTime != 0) {
            if (--pList[i].RestTime == 0) {
                DbgWlPutchar(B_WL_DBG_DEFRAG, (WL_DBG_DEFRAG_TIMEOUT));

                ReleaseHeapBuf(&wlMan->HeapMan.TmpBuf, pList[i].pPacket);
            }
        }
    }
}

void InitRxCtrl(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPRX_CTRL pRxCtrl = &wlMan->RxCtrl;
    u32 str_madrs;

    WLLIB_DmaClear32((u32)pRxCtrl, sizeof(RX_CTRL));

    *(u16 *)MREG_MDP_CONFIG = 0x8000;

    switch (wlMan->Work.Mode) {
    case WL_CMDLABEL_MODE_TEST:
        str_madrs = MBUF_TEST_RX;
        break;

    case WL_CMDLABEL_MODE_PARENT:
        str_madrs = MBUF_PARENT_RX;
        break;

    case WL_CMDLABEL_MODE_CHILD:
        str_madrs = MBUF_CHILD_RX;
        break;

    case WL_CMDLABEL_MODE_HOTSPOT:
        str_madrs = MBUF_HOTSPOT_RX;
        break;
    }

    *(vu16 *)MREG_RXBUF_STR = (u16)(MAC_MEM_BASE + str_madrs);
    *(vu16 *)MREG_RXBUF_WCUR = str_madrs / 2;
    *(vu16 *)MREG_RXBUF_END = (u16)(MAC_MEM_BASE + MBUF_MAC_RX_END);
    *(vu16 *)MREG_RXBUF_BNR = str_madrs / 2;

    pRxCtrl->wlCurr = str_madrs / 2;
    pRxCtrl->LastMpSeq = 0xFFFF;
    pWork->Ofst.RxBuf.Size = MBUF_MAC_RX_END - str_madrs;

#if (EMU_MACREG)
    *(vu16 *)MREG_RXBUF_CUR = *(vu16 *)MREG_RXBUF_WCUR;
#endif

    *(vu16 *)MREG_RDMA_JUMP = (u16)(MAC_MEM_BASE + MBUF_MAC_RX_END - 2);

    *(u16 *)MREG_MDP_CONFIG = 0x8001;

    *(u16 *)MREG_LAST_RX_ADRS0 = 0xFFFF;
    *(u16 *)MREG_LAST_RX_ADRS1 = 0xFFFF;
    *(u16 *)MREG_DCF_LAST_ADRS0 = 0xFFFF;
    *(u16 *)MREG_DCF_LAST_ADRS1 = 0xFFFF;
    *(vu16 *)MREG_MP_LAST_SEQCTRL = 0xFFFF;
    *(vu16 *)MREG_DCF_LAST_SEQCTRL = 0xFFFF;
}
