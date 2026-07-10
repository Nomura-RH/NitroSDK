#define __WLNIC_C_
#define __INSYSROM__

#include "WlSys.h"
#include "WlLib.h"

#include "WlCmdIf.h"
#include "ParamCmd.h"
#include "MA.h"

#include "MAC.h"
#include "CfgDevs.h"
#include "DbgChar.h"
#include "WlOpe.h"
#include "WaitLoop.h"
#include "TaskMan.h"
#include "WlIntr.h"

#include "libirissubpeeprom.h"
#include "EEPROM.h"
#include "Flash.h"

#include "Compati.h"

const u16 NULL_ADRS[3] = { 0x0000, 0x0000, 0x0000 };
const u16 BC_ADRS[3] = { 0xFFFF, 0xFFFF, 0xFFFF };
const u16 MP_ADRS[3] = { MP_ADRS0, MP_ADRS1, MP_ADRS2 };
const u16 MPKEY_ADRS[3] = { MPKEY_ADRS0, MPKEY_ADRS1, MPKEY_ADRS2 };

static const MAC_INIT_REGS macInitRegs[] = {
    { OFST_MREG_CMD, 0x0000 },
    { OFST_MREG_TXCONFIG, 0x0000 },
    { OFST_MREG_RXCONFIG, 0x0000 },
    { OFST_MREG_IMR, 0x0000 },
    { OFST_MREG_ISR, 0xFFFF },
    { OFST_MREG_DDO, 0x0000 },
    { OFST_MREG_TXQ_RESET, 0xFFFF },
    { OFST_MREG_BCN_ADRS, 0x0000 },
    { OFST_MREG_DTIM_PERIOD, 0x0001 },
    { OFST_MREG_DTIM_COUNT, 0x0000 },
    { OFST_MREG_AID, 0x0000 },
    { OFST_MREG_KSID, 0x0000 },
    { OFST_MREG_TSF_ENABLE, 0x0000 },
    { OFST_MREG_TBTT_ENABLE, 0x0000 },
    { OFST_MREG_TMPTT_ENABLE, 0x0001 },
    { OFST_MREG_NAV_ENABLE, 0x3F03 },
    { OFST_MREG_SERIAL_DAT_SEL, 0x0001 },
    { OFST_MREG_EN_DIRECT_CTL, 0x0000 },

    { OFST_MREG_PRE_TBTT, 0x0800 },

    { OFST_MREG_TXPREAMBLE_TYPE, PRE_MP_SHORT },

    { OFST_MREG_RESPONSE_CTRL, 0x0003 },
    { OFST_MREG_BUFOVF_TH, 0x0004 },
    { OFST_MREG_DEFRAG_OFST, 0x0602 },
    { OFST_MREG_WDMA_JUMP_CNT, 0x0000 },
    { OFST_MREG_RESPONSE_TIMEOUT, 326 },
};

#if (!USE_FLASH)
static const MAC_INIT_REGS macTimingRegs[3][16] = {
    {
        { OFST_MREG_TXPE_HOLD, 2 },
        { OFST_MREG_TXDELAY, 23 },
        { OFST_MREG_RXDELAY, 80 },
        { OFST_MREG_TRXPRE_INTERVAL, 0x1818 },
        { OFST_MREG_RDY_TIMEOUT, 0x0048 },
        { OFST_MREG_RX_TIMEOUT, 0x4840 },
        { OFST_MREG_MPACK_DELAY, 0x0058 },
        { OFST_MREG_CCA_DELAY, 0x0042 },
        { OFST_MREG_RESPONSE_TIMEOUT, 326 },
        { OFST_MREG_ACK_CCA_TIMEOUT, 0x8064 },

        { OFST_MREG_TS_TXOFST, 0xE6E6 },
        { OFST_MREG_TS_RXOFST, 0x2443 },

        { OFST_MREG_WAKEUP_CTRL, WAKEUP_CTRL_SLP_MPACK | WAKEUP_CTRL_SLP_MPKEY | WAKEUP_CTRL_WAKEUP_TMPTT },
        { OFST_MREG_TMPTT_ACT_TIME, 0x0001 },
        { OFST_MREG_TBTT_ACT_TIME, 0x0001 },
        { OFST_MREG_RF_WAKEUP_TIME, 0x0402 },
    },

    {
        { OFST_MREG_TXPE_HOLD, 2 },
        { OFST_MREG_TXDELAY, 29 },
        { OFST_MREG_RXDELAY, 44 },
        { OFST_MREG_TRXPRE_INTERVAL, 0x1002 },
        { OFST_MREG_RDY_TIMEOUT, 0x0048 },
        { OFST_MREG_RX_TIMEOUT, 0x4840 },
        { OFST_MREG_MPACK_DELAY, 0x0058 },
        { OFST_MREG_CCA_DELAY, 0x0042 },
        { OFST_MREG_RESPONSE_TIMEOUT, 326 },
        { OFST_MREG_ACK_CCA_TIMEOUT, 0x8064 },

        { OFST_MREG_TS_TXOFST, 0xE0E0 },
        { OFST_MREG_TS_RXOFST, 0x2443 },

        { OFST_MREG_WAKEUP_CTRL, WAKEUP_CTRL_SLP_MPACK | WAKEUP_CTRL_SLP_MPKEY | WAKEUP_CTRL_WAKEUP_TMPTT },
        { OFST_MREG_TMPTT_ACT_TIME, 30 },
        { OFST_MREG_TBTT_ACT_TIME, 300 },
        { OFST_MREG_RF_WAKEUP_TIME, 0x0101 },
    },

    {
        { OFST_MREG_TXPE_HOLD, 2 },
        { OFST_MREG_TXDELAY, 23 },
        { OFST_MREG_RXDELAY, 80 },
        { OFST_MREG_TRXPRE_INTERVAL, 0x1818 },
        { OFST_MREG_RDY_TIMEOUT, 0x0048 },
        { OFST_MREG_RX_TIMEOUT, 0x4840 },
        { OFST_MREG_MPACK_DELAY, 0x0058 },
        { OFST_MREG_CCA_DELAY, 0x0042 },
        { OFST_MREG_RESPONSE_TIMEOUT, 326 },
        { OFST_MREG_ACK_CCA_TIMEOUT, 0x8064 },

        { OFST_MREG_TS_TXOFST, 0xE6E6 },
        { OFST_MREG_TS_RXOFST, 0x2443 },

        { OFST_MREG_WAKEUP_CTRL, WAKEUP_CTRL_SLP_MPACK | WAKEUP_CTRL_SLP_MPKEY | WAKEUP_CTRL_WAKEUP_TMPTT },
        { OFST_MREG_TMPTT_ACT_TIME, 0x0001 },
        { OFST_MREG_TBTT_ACT_TIME, 0x0001 },
        { OFST_MREG_RF_WAKEUP_TIME, 0x0402 },
    }
};
#endif

#ifdef ONLY_WL
static
#endif
    const u16 macTxRxRegAdrs[16]
    = {
          OFST_MREG_TXPE_HOLD,
          OFST_MREG_TXDELAY,
          OFST_MREG_RXDELAY,
          OFST_MREG_TRXPRE_INTERVAL,
          OFST_MREG_RDY_TIMEOUT,
          OFST_MREG_RX_TIMEOUT,
          OFST_MREG_MPACK_DELAY,
          OFST_MREG_CCA_DELAY,
          OFST_MREG_ACK_CCA_TIMEOUT,
          OFST_MREG_ACK_CCA_TIMEOUT,

          OFST_MREG_TS_TXOFST,
          OFST_MREG_TS_RXOFST,

          OFST_MREG_WAKEUP_CTRL,
          OFST_MREG_TMPTT_ACT_TIME,
          OFST_MREG_TBTT_ACT_TIME,
          OFST_MREG_RF_WAKEUP_TIME,
      };

const u16 RateBit2Element[16] = { 2, 4, 11, 12, 18, 22, 24, 36, 48, 72, 96, 108, 0, 0, 0, 0 };
const u16 RateBit2RFRate[] = { RF_RATE_1M, RF_RATE_2M };
const u16 RateElement2Bit[] = {
    0xFF,
    0,
    0xFF,
    1,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    2,
    3,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    4,
    0xFF,
    0xFF,
    0xFF,
    5,
    0xFF,
    6,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    7,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    8,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    9,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    10,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    11,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
};
#define RATE_TABLE_NUM sizeof(RateElement2Bit) / 2

static const u16 def_WepKey[20 * 4 / 2] ATTRIBUTE_ALIGN(4) = WLDEF_WEP_KEY;
static const u16 def_SsidMask[32 / 2] ATTRIBUTE_ALIGN(4) = WLDEF_SSID_MASK;

#if (!USE_FLASH)
static const u16 ch_MAX2822[14] = {
    0x3000 + 12,
    0x3000 + 17,
    0x3000 + 22,
    0x3000 + 27,
    0x3000 + 32,
    0x3000 + 37,
    0x3000 + 42,
    0x3000 + 47,
    0x3000 + 52,
    0x3000 + 57,
    0x3000 + 62,
    0x3000 + 67,
    0x3000 + 72,
    0x3000 + 84
};

static const u32 ch_RF2958[14][2] = {
    { 0x141728, 0x1AE8BA },
    { 0x141737, 0x191746 },
    { 0x141745, 0x1B45D1 },
    { 0x141754, 0x19745D },
    { 0x141762, 0x1BA2E9 },
    { 0x141771, 0x19D174 },
    { 0x141780, 0x180000 },
    { 0x14178E, 0x1A2E8C },
    { 0x14179D, 0x185D17 },
    { 0x1417AB, 0x1A8BA3 },
    { 0x1417BA, 0x18BA2F },
    { 0x1417C8, 0x1AE8BA },
    { 0x1417D7, 0x191746 },
    { 0x1417FA, 0x18BA2F }
};

static const u16 ch_MM3156[14][2] = {
    { 0x0100 + 75, 0x0200 + 12 },
    { 0x0100 + 75, 0x0200 + 17 },
    { 0x0100 + 75, 0x0200 + 22 },
    { 0x0100 + 75, 0x0200 + 27 },
    { 0x0100 + 76, 0x0200 + 0 },
    { 0x0100 + 76, 0x0200 + 5 },
    { 0x0100 + 76, 0x0200 + 10 },
    { 0x0100 + 76, 0x0200 + 15 },
    { 0x0100 + 76, 0x0200 + 20 },
    { 0x0100 + 76, 0x0200 + 25 },
    { 0x0100 + 76, 0x0200 + 30 },
    { 0x0100 + 75, 0x0200 + 3 },
    { 0x0100 + 75, 0x0200 + 8 },
    { 0x0100 + 75, 0x0200 + 20 },
};

static const u16 pwr_MM3156[4][15] = {
    { 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 },
    { 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};
#endif

void InitializeParam(LPCAM_ELEMENT pCam, u32 staNum)
{
    void *pGameInfo = wlMan->Work.GameInfoAdrs;

    WLLIB_DmaClear16((u32)&wlMan->Config, sizeof(CONFIG_PARAM));
    WLLIB_DmaClear16((u32)&wlMan->Work, sizeof(WORK_PARAM));

    wlMan->Config.pCAM = pCam;
    wlMan->Config.CamMaxStaNum = (u16)staNum;
    wlMan->Config.MaxStaNum = (u16)staNum;
    wlMan->Work.GameInfoAdrs = pGameInfo;
}

u16 WSetMacAdrs(u16 *pMacAdrs)
{
    if (pMacAdrs[0] & 0x0001) {
        return WL_CMDRES_INVALID_PARAM;
    }

    WSetMacAdrs1(wlMan->Config.MacAdrs, pMacAdrs);

    WSetMacAdrs1((u16 *)MREG_MAC_ADRS0, pMacAdrs);

    wlMan->Config.ParamFlag |= PARAM_FLAG_MACADRS;

    return 0;
}

u16 WSetRetryLimit(u16 retry)
{
    if (retry > MAX_RETRY_LIMIT) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.RetryLimit = retry;

    *(vu16 *)MREG_RETRY_LIMIT = retry;

    return 0;
}

u16 WSetEnableChannel(u16 enableChannel)
{
    if ((enableChannel & MASK_ENABLE_CHANNEL) == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.EnableChannel = enableChannel;

    wlMan->Config.ParamFlag |= PARAM_FLAG_ENCH;

    return 0;
}

u16 WSetMode(u16 mode)
{
    if (mode > MAX_MODE_NUM) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.Mode = mode;
    wlMan->Work.Mode = mode;

    BitSet(*(vu16 *)MREG_CONFIG, MACCFG_WORK_MASK, mode);

    WSetPowerMgtMode(wlMan->Work.PowerMgtMode);

    wlMan->Config.ParamFlag |= PARAM_FLAG_MODE;

    return 0;
}

u16 WSetRate(u16 rate)
{
    if ((rate == WL_CMDLABEL_RATE_AUTO) || (rate == WL_CMDLABEL_RATE_1M) || (rate == WL_CMDLABEL_RATE_2M)) {
        wlMan->Config.Rate = rate;

        WSetTxTimeStampOffset();

        return 0;
    }

    return WL_CMDRES_INVALID_PARAM;
}

u16 WSetWepMode(u16 mode)
{
    LPBEACON_BODY pBody;
    LPWORK_PARAM pWork = &wlMan->Work;

    if (mode > MAX_WEPMODE_NUM) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.WepMode = mode;

    if (mode == WL_CMDLABEL_WEP_NO) {
        pWork->CapaInfo &= ~B_CAPA_PRIVACY;
        pWork->FrameCtrl &= ~B_FC_WEP;
    } else {
        pWork->CapaInfo |= B_CAPA_PRIVACY;
        pWork->FrameCtrl |= B_FC_WEP;
    }

    if ((pWork->STA == STA_CLASS3) && (mode == WL_CMDLABEL_MODE_PARENT)) {
        pBody = (LPBEACON_BODY)wlMan->TxCtrl.Beacon.pMacFrm->Body;
        pBody->CapaInfo.Data = pWork->CapaInfo;
    }

    if (mode == 0) {
        mode = 1;
    }
    BitSet(*(vu16 *)MREG_CONFIG, MACCFG_WEP_MASK, mode * MACCFG_WEP_NO);

    return 0;
}

u16 WSetWepKeyId(u16 keyId)
{
    if (keyId > MAX_WEP_KEYID_NUM) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.WepKeyId = keyId;

    return 0;
}

u16 WSetWepKey(u16 *pKey)
{
    DMA_Write((void *)(MAC_MEM_BASE + MBUF_WEPKEY_0), (void *)((u32)pKey + MAX_WEPKEY_LENGTH * 0), MAX_WEPKEY_LENGTH);
    DMA_Write((void *)(MAC_MEM_BASE + MBUF_WEPKEY_1), (void *)((u32)pKey + MAX_WEPKEY_LENGTH * 1), MAX_WEPKEY_LENGTH);
    DMA_Write((void *)(MAC_MEM_BASE + MBUF_WEPKEY_2), (void *)((u32)pKey + MAX_WEPKEY_LENGTH * 2), MAX_WEPKEY_LENGTH);
    DMA_Write((void *)(MAC_MEM_BASE + MBUF_WEPKEY_3), (void *)((u32)pKey + MAX_WEPKEY_LENGTH * 3), MAX_WEPKEY_LENGTH);

    return 0;
}

u16 WSetBeaconType(u16 type)
{
    if (type > MAX_BEACONTYPE_NUM) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.BeaconType = type;

    return 0;
}

u16 WSetBcSsidResponse(u16 response)
{
    if (response > MAX_BCSSIDRESPONSE_NUM) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.BcSsidResponse = response;

    return 0;
}

u16 WSetBeaconLostThreshold(u16 threshold)
{
    if (threshold > MAX_BCNLOSTTH_NUM) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Work.BeaconLostCnt = 0;
    wlMan->Work.BeaconLostTh = threshold;

    return 0;
}

u16 WSetActiveZoneTime(u16 time, u32 update)
{
    u8 *p;

    if (time < MIN_ACTIVE_ZONE) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.ActiveZone = time;

    if (update) {
        *(vu16 *)MREG_ACTZONE = time;
    }

    if (wlMan->TxCtrl.Beacon.Busy) {
        p = (u8 *)((u32)wlMan->TxCtrl.Beacon.pMacFrm->Body + wlMan->Work.Ofst.Beacon.GameInfo + 2 + 3 + 1);

        if (wlMan->Work.PowerMgtMode == WL_CMDLABEL_PMG_PS) {
            WL_WriteByte(p++, time);
            WL_WriteByte(p, time >> 8);
        } else {
            WL_WriteByte(p++, 0xFF);
            WL_WriteByte(p, 0xFF);
        }
    }

    return 0;
}

u16 WSetSsidMask(u16 *pMask)
{
    u32 i;
    u16 *pDest = (u16 *)wlMan->Work.SSIDMask;

    for (i = 0; i < MAX_SSID_LENGTH / 2; i++) {
        *pDest++ = *pMask++;
    }

    return 0;
}

u16 WSetPreambleType(u16 type)
{
    LPBEACON_BODY pBody;
    LPWORK_PARAM pWork = &wlMan->Work;

    if (type > MAX_PREAMBLE_TYPE) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.PreambleType = type;

    if (type == WL_CMDLABEL_PREAMBLE_LONG) {
        pWork->CapaInfo &= ~B_CAPA_SHORTPREAMBLE;
    } else {
        pWork->CapaInfo |= B_CAPA_SHORTPREAMBLE;
    }

    if ((pWork->STA == STA_CLASS3) && (wlMan->Config.Mode == WL_CMDLABEL_MODE_PARENT)) {
        pBody = (LPBEACON_BODY)wlMan->TxCtrl.Beacon.pMacFrm->Body;
        pBody->CapaInfo.Data = pWork->CapaInfo;
    }

    if (type == WL_CMDLABEL_PREAMBLE_LONG) {
        *(vu16 *)MREG_TXPREAMBLE_TYPE &= ~(BM_PRE_DCF | BM_PRE_BEACON);
    } else {
        *(vu16 *)MREG_TXPREAMBLE_TYPE |= (BM_PRE_DCF | BM_PRE_BEACON);
    }

    WSetTxTimeStampOffset();

    return 0;
}

u16 WSetAuthAlgo(u16 type)
{
    if (type > MAX_AUTHALGO) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.AuthAlgo = type;

    return 0;
}

u16 WSetCCA_ED(u32 ccaMode, u32 edThreshold)
{
    if (ccaMode > MAX_CCA_MODE) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (edThreshold > MAX_ED_THRESHOLD) {
        return WL_CMDRES_INVALID_PARAM;
    }

    BBP_Write(0x13, ccaMode);

    BBP_Write(0x35, edThreshold);

    return 0;
}

u16 WSetMainAntenna(u32 mainAntenna)
{
    if (mainAntenna > MAX_ANTENNA) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.MainAntenna = mainAntenna;

    WSetAntenna();

    return 0;
}

u16 WSetDiversity(u32 diversity, u32 useAntenna)
{
    if ((diversity > MAX_DIVERSITY_MODE) || (useAntenna > MAX_ANTENNA)) {
        return WL_CMDRES_INVALID_PARAM;
    }

    switch (diversity) {
    case WL_CMDLABEL_DIVERSITY_OFF:
        wlMan->Config.UseAntenna = useAntenna;
        break;

    case WL_CMDLABEL_DIVERSITY_ON:
        if (wlMan->Config.Mode != WL_CMDLABEL_MODE_PARENT) {
            return WL_CMDRES_ILLEGAL_MODE;
        }
        wlMan->Config.UseAntenna = WL_CMDLABEL_USE_MAIN_ANTENNA;
        break;
    }

    wlMan->Config.Diversity = diversity;

    WSetAntenna();

    return 0;
}

u16 WSetBeaconSendRecvIndicate(u32 mode)
{
    if (mode > MAX_BCN_TXRX_IND_MODE) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.BcnTxRxIndMsg = mode;

    return 0;
}

u16 WSetNullKeyMode(u32 mode)
{
    if (mode > MAX_NULLKEY_MODE) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Config.NullKeyRes = mode;

    if (mode == WL_CMDLABEL_NULLKEY_ENABLE) {
        *(vu16 *)MREG_KSID = *(vu16 *)MREG_AID;
    }

    return 0;
}

u16 WSetBssid(u16 *pBssid)
{
    WSetMacAdrs1(wlMan->Work.BSSID, pBssid);

    WSetMacAdrs1((u16 *)MREG_BSSID0, pBssid);

    if (pBssid[0] & 0x0001) {
        *(vu16 *)MREG_BUF_SELECT &= ~BF_OTHER_BSSID_MAN;
    } else {
        *(vu16 *)MREG_BUF_SELECT |= BF_OTHER_BSSID_MAN;
    }

    return 0;
}

u16 WSetSsid(u16 length, u8 *pSsid)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    u32 i, b = 0;
    u8 *pBuf;

    if (length > MAX_SSID_LENGTH) {
        return WL_CMDRES_INVALID_PARAM;
    }

    if ((pWork->STA == STA_CLASS3) && (wlMan->Config.Mode == WL_CMDLABEL_MODE_PARENT)) {
        if (pWork->SSIDLength != length) {
            return WL_CMDRES_REFUSE;
        }
        if (pWork->Ofst.Beacon.SSID != 0) {
            b = 1;
        }
    }

    for (i = 0; i < length; i++) {
        WL_WriteByte(&pWork->SSID[i], WL_ReadByte(pSsid));
        pSsid++;
    }
    for (; i < MAX_SSID_LENGTH; i++) {
        WL_WriteByte(&pWork->SSID[i], 0);
    }
    pWork->SSIDLength = length;

    if (b) {
        pBuf = (u8 *)((u32)wlMan->TxCtrl.Beacon.pMacFrm + 12 + 24 + 2 + (u32)pWork->Ofst.Beacon.SSID);
        for (i = 0; i < length; i++) {
            WL_WriteByte(&pBuf[i], WL_ReadByte(&pWork->SSID[i]));
        }
    }
    return 0;
}

u16 WSetBeaconPeriod(u16 period)
{
    if ((period < MIN_BEACON_PERIOD) || (period > MAX_BEACON_PERIOD)) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Work.BeaconPeriod = period;

    *(vu16 *)MREG_BCN_PERIOD = period;

    WSetFrameLifeTime(wlMan->Config.FrameLifeTimePerBeacon);

    return 0;
}

u16 WSetDTIMPeriod(u16 period)
{
    if ((period < MIN_DTIM_PERIOD) || (period > MAX_DTIM_PERIOD)) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Work.DTIMPeriod = period;

    *(vu16 *)MREG_DTIM_PERIOD = period;
    *(vu16 *)MREG_DTIM_COUNT = 0;

    return 0;
}

u16 WSetListenInterval(u16 interval)
{
    if ((interval < MIN_LISTEN_INTERVAL) || (interval > MAX_LISTEN_INTERVAL)) {
        return WL_CMDRES_INVALID_PARAM;
    }

    wlMan->Work.ListenInterval = interval;

    return 0;
}

void WSetDefaultParameters(void)
{
    static const RATE_SET c_RateSet = { WLDEF_RATESET_BASIC, WLDEF_RATESET_SUPPORT };
    u16 macAdrs[3];
    u16 ench;

    DbgPrint("Setup Default Params\r\n");

#if (USE_FLASH)
    FLASH_Read(FLASH_ADRS(macAdrs[0]), 6, (u8 *)macAdrs);
    FLASH_Read(FLASH_ADRS(enableChannel), 2, (u8 *)&ench);
#else
    WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(irisMacAdrs[0]), (u16 *)macAdrs, 6);
    WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(enableChannel), (u16 *)&ench, 2);
#endif

    WSetMacAdrs(macAdrs);
    WSetRetryLimit(WLDEF_RETRY_LIMIT);
    WSetEnableChannel(ench & 0x7FFE);
    WSetMode(WLDEF_MODE);
    WSetRate(WLDEF_RATE);
    WSetWepMode(WLDEF_WEP_MODE);
    WSetWepKeyId(WLDEF_WEP_KEYID);
    WSetWepKey((u16 *)def_WepKey);
    WSetBeaconPeriod(WLDEF_BEACON_PERIOD);
    WSetBeaconType(WLDEF_BCN_TYPE);
    WSetBcSsidResponse(WLDEF_BCSSID_RESPONSE);
    WSetBeaconLostThreshold(WLDEF_BCN_LOST_TH);
    WSetActiveZoneTime(WLDEF_ACTIVE_ZONE, FALSE);
    WSetSsidMask((u16 *)def_SsidMask);
    WSetPreambleType(WLDEF_PREAMBLE_TYPE);
    WSetAuthAlgo(WLDEF_AUTHALGO);

    WSetRateSet((LPRATE_SET)&c_RateSet);
    WSetCCA_ED(WLDEF_CCA_MODE, WLDEF_ED_THRESHOLD);
    WSetFrameLifeTime(WLDEF_FRAME_LIFETIME);

    WSetDiversity(WLDEF_DIVERSITY, WLDEF_USEANTENNA);
    WSetMainAntenna(WLDEF_MAIN_ANTENNA);
    WSetBeaconSendRecvIndicate(WLDEF_BCN_TXRX_IND);

    WSetNullKeyMode(WLDEF_NULLKEY_RESPONSE);

    RND_init(*(vu16 *)MREG_MSEQ16 + (*(vu16 *)MREG_MSEQ16 << 8), *(vu16 *)MREG_MSEQ16);

    wlMan->Work.RxDtims = WL_CMDLABEL_RX_ALL_DTIM;
}

u16 WSetChannel(u16 channel, u32 bDirect)
{
#pragma unused(bDirect)
    u32 reg;
    u32 bkpwr;
#if (USE_FLASH)
    u32 flash_adrs, i, adrs;
    void (*pFlashReadFunc)(u32 adrs, u32 size, u8 *pBuf);

    if (bDirect) {
        pFlashReadFunc = FLASH_DirectRead;
    } else {
        pFlashReadFunc = FLASH_Read;
    }
#endif

    if (CheckEnableChannel(channel) == 0) {
        return WL_CMDRES_INVALID_PARAM;
    }

    bkpwr = *(vu16 *)MREG_SET_FORCE_POWER;
    *(vu16 *)MREG_SET_FORCE_POWER = FPWR_STS_SLEEP;

    while (1) {
        u32 ps, st;

        ps = *(vu16 *)MREG_SET_POWER >> 8;
        st = *(vu16 *)MREG_MAC_STATE;

        if ((ps == 2) && ((st == 0) || (st == 9))) {
            break;
        }
    }

    wlMan->Work.CurrChannel = channel;

    switch (wlMan->Rf.Id) {
#if (INCLUDE_BBP_ES1)
    case MTMBBP_ES1:
#endif
#ifdef SDK_INCLUDE_MAX2822
    case MAX2822:
#if (USE_FLASH)
        reg = 0;
        (*pFlashReadFunc)(FLASH_ADRS(rfInit_channelDependData.MAX2822Tbl.ch_Reg[channel - 1]), 2, (u8 *)&reg);
#else
        reg = ch_MAX2822[channel - 1];
#endif
        RF_Write(reg);
        if (bWait) {
            WWaitus(200);
        }

        reg = 0;
#if (USE_FLASH)
        (*pFlashReadFunc)(FLASH_ADRS(rfInit_channelDependData.MAX2822Tbl.ch_PwrBB[channel - 1]), 1, (u8 *)&reg);
#else
        WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(txPwrRegs[channel - 1]), (u16 *)&reg, 1);
#endif
        BBP_Write(30, reg);
        break;
#endif

    case MDRF2958:
#if (USE_FLASH)
        reg = 0;
        (*pFlashReadFunc)(FLASH_ADRS(rfInit_channelDependData.RF2958Tbl.ch_Reg[channel - 1][0]), 3, (u8 *)&reg);
        RF_Write(reg);
        (*pFlashReadFunc)(FLASH_ADRS(rfInit_channelDependData.RF2958Tbl.ch_Reg[channel - 1][3]), 3, (u8 *)&reg);
        RF_Write(reg);
#else
        RF_Write(ch_RF2958[channel - 1][0]);
        RF_Write(ch_RF2958[channel - 1][1]);
#endif

        reg = 0;
        if (wlMan->Rf.BkReg & 0x10000) {
            if ((wlMan->Rf.BkReg & 0x08000) == 0) {
#if (USE_FLASH)
                (*pFlashReadFunc)(FLASH_ADRS(rfInit_channelDependData.RF2958Tbl.ch_PwrRF[channel - 1]), 1, (u8 *)&reg);
                reg = wlMan->Rf.BkReg | ((reg & 0x1F) << 10);
                RF_Write(reg);
#endif
            }
        } else {
#if (USE_FLASH)
            (*pFlashReadFunc)(FLASH_ADRS(rfInit_channelDependData.RF2958Tbl.ch_PwrBB[channel - 1]), 1, (u8 *)&reg);
#else
            WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(txPwrRegs[channel - 1]), (u16 *)&reg, 1);
#endif
            BBP_Write(30, reg);
        }
        break;

    case MM3156:
#if (USE_FLASH)
        flash_adrs = FLASH_ADRS(rfInit_channelDependData.MM3156Tbl.init[0]) + wlMan->Rf.InitNum + 1;
        for (i = 0; i < wlMan->Rf.BbpCnt; i++) {
            adrs = reg = 0;
            (*pFlashReadFunc)(flash_adrs, 1, (u8 *)&adrs);
            (*pFlashReadFunc)(flash_adrs + channel, 1, (u8 *)&reg);
            BBP_Write(adrs, reg);

            flash_adrs += 15;
        }

        for (i = 0; i < wlMan->Rf.ChanNum; i++) {
            reg = 0;
            (*pFlashReadFunc)(flash_adrs, 1, (u8 *)&reg);
            reg <<= 8;
            (*pFlashReadFunc)(flash_adrs + channel, 1, (u8 *)&reg);
            reg |= (5 << 16);
            RF_Write(reg);

            flash_adrs += 15;
        }
#endif
        break;
    }

    *(vu16 *)MREG_SET_FORCE_POWER = bkpwr;
    *(vu16 *)MREG_MP_POWER_SEQ = 3;

    return 0;
}

u16 WSetRateSet(LPRATE_SET pRateSet)
{
    LPRATE_SET pWRS = &wlMan->Work.RateSet;

    pWRS->Basic = pRateSet->Basic;
    pWRS->Support = (u16)(pRateSet->Support | pRateSet->Basic);

    WSetTxTimeStampOffset();

    return 0;
}

void WSetTxTimeStampOffset(void)
{
    u32 ofst = 0xe2e2;

#if (USE_FLASH)
    FLASH_Read(FLASH_ADRS(macTxRxRegs[10]), 2, (u8 *)&ofst);
#else
    {
        u32 i;

        for (i = 0; i < sizeof(macTimingRegs[0]); i++) {
            if (macTimingRegs[1][i].adrs == OFST_MREG_TS_TXOFST) {
                ofst = macTimingRegs[1][i].value;
                break;
            }
        }
    }
#endif
    ofst += 0x0202;

    if (WCalcManRate() == RF_RATE_2M) {
        ofst -= (97 + 97 * 0x100);

        if (*(vu16 *)MREG_TXPREAMBLE_TYPE & PRE_DCF_SHORT) {
            ofst -= (96 + 96 * 0x100);
        }
    }

    *(vu16 *)MREG_TS_TXOFST = ofst;
}

u16 WSetPowerMgtMode(u32 mode)
{
    LPCONFIG_PARAM pConfig = &wlMan->Config;

    wlMan->Work.PowerMgtMode = mode;

    if (mode && (pConfig->Mode != WL_CMDLABEL_MODE_PARENT)) {
        *(vu16 *)MREG_CONFIG |= MACCFG_PWR_PS;
    } else {
        *(vu16 *)MREG_CONFIG &= ~MACCFG_PWR_PS;

        WSetActiveZoneTime(pConfig->ActiveZone, FALSE);
    }

    return 0;
}

u16 WSetPowerState(u32 state)
{
    DbgSetDDO(DDO_WL_PS, DDO_WL_PS_SET_POWER);

    wlMan->Work.PowerState = state >> 1;

    *(vu16 *)MREG_SET_POWER = (u16)state;

    DbgClrDDO(DDO_WL_PS, DDO_WL_PS_SET_POWER);

    return 0;
}

u16 WSetForcePowerState(u32 state)
{
    *(vu16 *)MREG_SET_FORCE_POWER = (u16)state;

    return 0;
}

void WShutdown(void)
{
    u32 tmp;

    switch (wlMan->Rf.Id) {
#ifdef SDK_INCLUDE_MAX2822
    case MAX2822:
        break;
#endif

    case MDRF2958:
        RF_Write(0x00C008);
        break;
    }

    tmp = BBP_Read(30) | 0x3f;
    BBP_Write(30, tmp);

    *(vu16 *)MREG_SCF_DIRECT = 0x800D;

    *(vu16 *)MREG_SHUTDOWN = SHDWN_ENABLE;
}

void WWakeUp(void)
{
    u32 bp;

    *(vu16 *)MREG_SHUTDOWN = SHDWN_DISABLE;

    WWait(8);

    *(vu16 *)MREG_SCF_DIRECT = 0;

    switch (wlMan->Rf.Id) {
#ifdef SDK_INCLUDE_MAX2822
    case MAX2822:
        break;
#endif

    case MDRF2958:

        bp = BBP_Read(0x01);
        BBP_Write(0x01, bp & 0x7F);
        BBP_Write(0x01, bp);

        WWait(40);

        InitRF();
        break;

    case MM3156:
        InitRF();
        break;

    default:
        DbgPrint("Unknown RF Device[%u]\r\n", wlMan->Rf.Id);
        break;
    }
}

u16 WSetFrameLifeTime(u32 lifeTimePerBeacon)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;

    if (lifeTimePerBeacon == 0xFFFF) {
        pConfig->FrameLifeTimePerBeacon = 0xFFFF;
        pWork->FrameLifeTime = 0xFFFF;
    } else {
        u32 lifeTime = (u32)lifeTimePerBeacon * (u32)pWork->BeaconPeriod / WINTERVAL_TIMER;

        if (lifeTime > 0x00010000) {
            return WL_CMDRES_INVALID_PARAM;
        }

        pConfig->FrameLifeTimePerBeacon = lifeTimePerBeacon;
        pWork->FrameLifeTime = lifeTime;
    }

    return 0;
}

void WDisableTmpttPowerSave(void)
{
    wlMan->Work.TmpttPs = 1;
    if (wlMan->TxCtrl.Mp.Busy == 0) {
        *(vu16 *)MREG_WAKEUP_CTRL &= ~WAKEUP_CTRL_WAKEUP_TMPTT;
        *(vu16 *)MREG_MP_POWER_SEQ = 0;
    }
}

void WEnableTmpttPowerSave(void)
{
    wlMan->Work.TmpttPs = 0;
    *(vu16 *)MREG_WAKEUP_CTRL |= WAKEUP_CTRL_WAKEUP_TMPTT;
}

u16 WInitGameInfo(u32 length, u8 *pGameInfo)
{
    LPWORK_PARAM pWork = &wlMan->Work;

    if (length > MAX_GAMEINFO_LENGTH) {
        return WL_CMDRES_LENGTH_ERR;
    }

    WLLIB_DmaCopy16((u32)pGameInfo, (u32)pWork->GameInfoAdrs, length + 1);

    pWork->GameInfoLength = length;

    return 0;
}

u16 WSetGameInfo(u32 length, u8 *pGameInfo)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    u8 *p;
    u32 i;

    if (length > MAX_GAMEINFO_LENGTH) {
        return WL_CMDRES_LENGTH_ERR;
    }

    if (length != 0) {
        if (pWork->GameInfoAlign & 1) {
            p = (u8 *)pWork->GameInfoAdrs;
            WL_WriteByte(p, 0xFF);
            p++;
            for (i = 0; i < length; i++) {
                WL_WriteByte(p, WL_ReadByte(pGameInfo));
                p++;
                pGameInfo++;
            }
        } else {
            WLLIB_DmaCopy16((u32)pGameInfo, (u32)pWork->GameInfoAdrs, length + 1);
        }
    }

    pWork->GameInfoLength = length;
    pWork->bUpdateGameInfo = TRUE;

    return 0;
}

void WSetAids(u16 aid)
{
    wlMan->Work.AID = aid;
    *(vu16 *)MREG_AID = aid;
    if (wlMan->Config.NullKeyRes) {
        *(vu16 *)MREG_KSID = aid;
    }
}

void WClearAids(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;

    pWork->AID = 0;
    WaitLoop_ClrAid();
    *(vu16 *)MREG_AID = 0;

    if (pWork->APCamAdrs != 0) {
        DeleteTxFrames(pWork->APCamAdrs);
        CAM_SetStaState(pWork->APCamAdrs, STA_CLASS1);
        pWork->APCamAdrs = 0;
    }
}

void WSetKSID(void)
{
    *(vu16 *)MREG_KSID = wlMan->Work.AID;
}

void WClearKSID(void *arg)
{
#pragma unused(arg)
    if ((*(vu16 *)MREG_KEYIN_ADRS & TXQ_ENABLE) == 0) {
        WaitLoop_ClrAid();
    }
}

void WSetStaState(u32 state)
{
    LPWORK_PARAM pWork = &wlMan->Work;

    if (pWork->STA == state) {
        return;
    }

    if (pWork->STA == STA_CLASS3) {
        ClearTimeOut();
    }

    switch (state) {
    case STA_SHUTDOWN:
        WShutdown();
        break;

    case STA_IDLE:
        WSetForcePowerState(FPWR_STS_DISABLE);
        WStop();
        WWakeUp();
        break;

    case STA_CLASS3:
        if (pWork->Mode == WL_CMDLABEL_MODE_CHILD) {
            WEnableTmpttPowerSave();
        }
        SetupPeriodicTimeOut(WINTERVAL_TIMER, WIntervalTimer);
        break;
    }

    pWork->STA = state;
}

void WSetMacAdrs1(u16 *dst, u16 *src1)
{
    *dst++ = *src1++;
    *dst++ = *src1++;
    *dst = *src1;
}

void WSetMacAdrs2(u16 *dst, u16 *src1, u16 *src2)
{
    *dst++ = *src1++;
    *dst++ = *src1++;
    *dst++ = *src1;
    *dst++ = *src2++;
    *dst++ = *src2++;
    *dst = *src2;
}

void WSetMacAdrs3(u16 *dst, u16 *src1, u16 *src2, u16 *src3)
{
    *dst++ = *src1++;
    *dst++ = *src1++;
    *dst++ = *src1;
    *dst++ = *src2++;
    *dst++ = *src2++;
    *dst++ = *src2;
    *dst++ = *src3++;
    *dst++ = *src3++;
    *dst = *src3;
}

void WInitCounter(void)
{
    WUpdateCounter();
    WLLIB_DmaClear32((u32)&wlMan->Counter, sizeof(WlCounter));
}

#define WORD_B1(_x_) (_x_ >> 8)
#define WORD_B0(_x_) (_x_ & 0x00FF)

void WUpdateCounter(void)
{
    WlCounter *pCounter = &wlMan->Counter;
    u16 *pMRegCounter = (u16 *)MREG_RXCOUNTER;
    u16 tmp;

    tmp = *pMRegCounter++;
    pCounter->rx.plcpErr += (u32)LoadLow(&tmp);

    tmp = *pMRegCounter++;
    pCounter->rx.lengthErr += (u32)LoadHigh(&tmp);
    pCounter->rx.rateErr += (u32)LoadLow(&tmp);

    tmp = *pMRegCounter++;
    pCounter->rx.pathErr += (u32)LoadHigh(&tmp);
    pCounter->rx.bufOvfErr += (u32)LoadLow(&tmp);

    tmp = *pMRegCounter++;
    pCounter->rx.fcsOk += (u32)LoadHigh(&tmp);
    pCounter->rx.fcsErr += (u32)LoadLow(&tmp);

    tmp = *pMRegCounter++;
    pCounter->rx.fcErr += (u32)LoadLow(&tmp);

    tmp = *pMRegCounter++;
    pCounter->rx.rts += (u32)LoadLow(&tmp);

    tmp = *pMRegCounter++;
    pCounter->rx.wep += (u32)LoadHigh(&tmp);
    pCounter->rx.icvErr += (u32)LoadLow(&tmp);

    tmp = *pMRegCounter++;
    pCounter->rx.duplicateErr += (u32)LoadLow(&tmp);
    pCounter->rx.mpDuplicateErr += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->tx.ackErr += (u32)LoadLow(&tmp);

    pMRegCounter += (0xE / 2);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[0] += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[1] += (u32)LoadLow(&tmp);
    pCounter->multiPoll.keyResponseErr[2] += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[3] += (u32)LoadLow(&tmp);
    pCounter->multiPoll.keyResponseErr[4] += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[5] += (u32)LoadLow(&tmp);
    pCounter->multiPoll.keyResponseErr[6] += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[7] += (u32)LoadLow(&tmp);
    pCounter->multiPoll.keyResponseErr[8] += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[9] += (u32)LoadLow(&tmp);
    pCounter->multiPoll.keyResponseErr[10] += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[11] += (u32)LoadLow(&tmp);
    pCounter->multiPoll.keyResponseErr[12] += (u32)LoadHigh(&tmp);

    tmp = *pMRegCounter++;
    pCounter->multiPoll.keyResponseErr[13] += (u32)LoadLow(&tmp);
    pCounter->multiPoll.keyResponseErr[14] += (u32)LoadHigh(&tmp);
}

u32 WCheckSSID(u16 len, u8 *pSSID)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    u8 *pBuf, *pMask;
    u32 mask, ssid, myss;
    u32 i;

    if (len > MAX_SSID_LENGTH) {
        return FALSE;
    }

    if (pWork->SSIDLength == 0) {
        return TRUE;
    }

    if (wlMan->MLME.State == MLME_STATE_SCAN_ING) {
        if (len < pWork->SSIDLength) {
            return FALSE;
        }
        len = pWork->SSIDLength;
    } else {
        if (len != pWork->SSIDLength) {
            return FALSE;
        }
    }

    pBuf = pWork->SSID;
    pMask = pWork->SSIDMask;

    for (i = 0; i < len; i++) {
        mask = WL_ReadByte(pMask);
        pMask++;
        ssid = WL_ReadByte(pSSID);
        pSSID++;
        myss = WL_ReadByte(pBuf);
        pBuf++;

        if ((ssid | mask) != (myss | mask)) {
            return FALSE;
        }
    }

    return TRUE;
}

u32 MatchMacAdrs(u16 *pAdrs1, u16 *pAdrs2)
{
    return (pAdrs1[2] == pAdrs2[2]) && (pAdrs1[1] == pAdrs2[1]) && (pAdrs1[0] == pAdrs2[0]);
}

u32 CheckEnableChannel(u32 ch)
{
    return ((u32)0x0001 << ch) & wlMan->Config.EnableChannel;
}

void WElement2RateSet(LPSUP_RATE_ELEMENT pSup, LPRATE_SET pRateSet)
{
    u32 i, length, brate, srate;

    pRateSet->Basic = 0;
    pRateSet->Support = 0;

    length = WL_ReadByte(&pSup->Length);
    for (i = 0; i < length; i++) {
        brate = WL_ReadByte(&pSup->SupportedRate[i]);
        srate = (brate & 0x7F) - 1;

        if ((srate < RATE_TABLE_NUM) && (RateElement2Bit[srate] != 0xFF)) {
            pRateSet->Support |= (0x01 << RateElement2Bit[srate]);

            if (brate & 0x80) {
                pRateSet->Basic |= (0x01 << RateElement2Bit[srate]);
            }
        } else {
            pRateSet->Support |= 0x8000;
            if (brate & 0x80) {
                pRateSet->Basic |= 0x8000;
            }
        }
    }
}

u32 WCalcManRate(void)
{
    switch (wlMan->Config.Rate) {
    case WL_CMDLABEL_RATE_AUTO:
        if (wlMan->Work.RateSet.Basic & 0x01) {
            return RF_RATE_1M;
        }
        break;

    case WL_CMDLABEL_RATE_1M:
        return RF_RATE_1M;
        break;
    }

    return RF_RATE_2M;
}

void WStart(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPCONFIG_PARAM pConfig = &wlMan->Config;
    u32 bp;
    u16 *ptsf;
    u64 lltsf;

    WStop();

    RND_init((u32)(*(vu16 *)MREG_MSEQ16 + (*(vu16 *)MREG_MSEQ16 << 8)), *(vu16 *)MREG_MSEQ16);
    pWork->CapaInfo = B_CAPA_ESS;
    if (pConfig->PreambleType == WL_CMDLABEL_PREAMBLE_SHORT) {
        pWork->CapaInfo |= B_CAPA_SHORTPREAMBLE;
    }
    if (pConfig->WepMode != WL_CMDLABEL_WEP_NO) {
        pWork->CapaInfo |= B_CAPA_PRIVACY;
    }
    pWork->bSynchro = 0;
    *(vu16 *)MREG_WEP_CONFIG = WEP_CFG_ON;
    *(vu16 *)MREG_ACTZONE = 0xFFFF;
    *(vu16 *)MREG_AID = 0;
    *(vu16 *)MREG_KSID = 0;
    *(vu16 *)MREG_WAKEUP_CTRL = WAKEUP_CTRL_SLP_MPACK | WAKEUP_CTRL_SLP_MPKEY | WAKEUP_CTRL_WAKEUP_TMPTT | WAKEUP_CTRL_WAKEUP_TBTT;

    InitCAM();
    InitApList();
    InitTxCtrl();
    InitRxCtrl();

    *(u16 *)MREG_MDP_CONFIG = 0x8000;

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR = 0x0000;
#else
    *(vu16 *)MREG_ISR = 0xFFFF;
#endif
    *(vu16 *)MREG_CNTOVF_IMR = 0x1FFF;
    if (wlMan->WlOperation & MAC_BUG_WEP) {
        *(vu16 *)MREG_CNTUP_IMR = RXCNTSTS_ICV_OK;
    } else {
        *(vu16 *)MREG_CNTUP_IMR = 0;
    }
    *(vu16 *)MREG_TXCONFIG = 0;
    *(vu16 *)MREG_RXCONFIG = 0;

    switch (pWork->Mode) {
    case WL_CMDLABEL_MODE_TEST:
        *(vu16 *)MREG_IMR = MISR_ACKCNT_OVF | MISR_CNT_OVF | MISR_TXERR | MISR_TXEND | MISR_RXCNTUP | MISR_RXEND;
        *(vu16 *)MREG_BUF_SELECT = 0xFFFF;
        *(vu16 *)MREG_DS_MASK = TODS_FROMDS_TEST;
        *(vu16 *)MREG_TXCONFIG = 0;
        *(vu16 *)MREG_RXCONFIG = 0;
        *(vu16 *)MREG_TSF_ENABLE = 0;
        *(vu16 *)MREG_CMD = MACCMD_START;
        break;

    case WL_CMDLABEL_MODE_PARENT:
        *(vu16 *)MREG_IMR = MISR_TBTT | MISR_ACTEND | MISR_ACKCNT_OVF | MISR_CNT_OVF | MISR_TXERR | MISR_TXEND | MISR_RXCNTUP | MISR_RXEND | MISR_MPEND | 0;
        *(vu16 *)MREG_CNTOVF_IMR = 0x1FFF;
        *(vu16 *)MREG_BUF_SELECT = BF_OTHER_BSSID_BEACON | BF_BC_BSSID_MAN | BF_MP_NULLKEY;
        *(vu16 *)MREG_DS_MASK = TODS_FROMDS_PARENT;
        *(vu16 *)MREG_TXCONFIG = TXCFG_INT_BCN | TXCFG_INT_MP | TXCFG_INT_MP_ACK;
        *(vu16 *)MREG_CMD = MACCMD_START;

        ptsf = (u16 *)&lltsf;
        ptsf[0] = *(vu16 *)MREG_TSF0;
        ptsf[1] = *(vu16 *)MREG_TSF1;
        ptsf[2] = *(vu16 *)MREG_TSF2;
        ptsf[3] = *(vu16 *)MREG_TSF3;
        bp = (u32)pWork->BeaconPeriod * 1024;
        lltsf /= bp;
        lltsf++;
        lltsf *= bp;
        *(vu16 *)MREG_NEXT_TBTT_TSF3 = ptsf[3];
        *(vu16 *)MREG_NEXT_TBTT_TSF2 = ptsf[2];
        *(vu16 *)MREG_NEXT_TBTT_TSF1 = ptsf[1];
        *(vu16 *)MREG_NEXT_TBTT_TSF0 = (u16)(ptsf[0] | 0x0001);

        *(vu16 *)MREG_TSF_ENABLE = 1;
        *(vu16 *)MREG_TBTT_ENABLE = 1;

        WSetStaState(STA_CLASS3);

        StartBeaconFrame();

        TxqOpen(TXQ_OPEN_MP);
        break;

    case WL_CMDLABEL_MODE_CHILD:
        *(vu16 *)MREG_IMR = MISR_PRE_TBTT | MISR_TBTT | MISR_ACTEND | MISR_ACKCNT_OVF | MISR_CNT_OVF | MISR_TXERR | MISR_TXEND | MISR_RXCNTUP | MISR_RXEND | MISR_START_TX | 0;
        if (wlMan->WlOperation & MAC_BUG_TXBUG_RXNOTPOLL) {
            *(vu16 *)MREG_IMR |= MISR_START_RX;
            *(vu16 *)MREG_CNTUP_IMR |= (RXCNTSTS_FCS_ERR | RXCNTSTS_FCS_OK | RXCNTSTS_BUF_OVF_ERR);
        }
        if (pWork->BSSID[0] & 0x001) {
            *(vu16 *)MREG_BUF_SELECT = BF_OTHER_BSSID_BEACON | BF_MP_NULLKEY | BF_MP_ACK | BF_OTHER_BSSID_MAN;
        } else {
            *(vu16 *)MREG_BUF_SELECT = BF_OTHER_BSSID_BEACON | BF_MP_NULLKEY | BF_MP_ACK;
        }
        *(vu16 *)MREG_DS_MASK = TODS_FROMDS_CHILD;
        *(vu16 *)MREG_CMD = MACCMD_START;
        *(vu16 *)MREG_TSF_ENABLE = 1;
        *(vu16 *)MREG_TBTT_ENABLE = 1;

        WSetStaState(STA_CLASS1);
        break;

    case WL_CMDLABEL_MODE_HOTSPOT:
#if (EMU_MACREG)
        *(vu16 *)MREG_ISR = 0x0000;
#else
        *(vu16 *)MREG_ISR = 0xFFFF;
#endif
        *(vu16 *)MREG_IMR = MISR_PRE_TBTT | MISR_TBTT | MISR_ACKCNT_OVF | MISR_CNT_OVF | MISR_TXERR | MISR_TXEND | MISR_RXCNTUP | MISR_RXEND | 0;
        if (pWork->BSSID[0] & 0x001) {
            *(vu16 *)MREG_BUF_SELECT = BF_OTHER_BSSID_BEACON | BF_OTHER_BSSID_MAN;
        } else {
            *(vu16 *)MREG_BUF_SELECT = BF_OTHER_BSSID_BEACON;
        }
        *(vu16 *)MREG_DS_MASK = TODS_FROMDS_HOTSPOT;
        *(vu16 *)MREG_CMD = MACCMD_START;
        *(vu16 *)MREG_TSF_ENABLE = 1;
        *(vu16 *)MREG_TBTT_ENABLE = 1;
        *(vu16 *)MREG_MP_POWER_SEQ = 0;

        WSetStaState(STA_CLASS1);
        break;

    case WL_CMDLABEL_MODE_MEASCHAN:
        *(vu16 *)MREG_IMR = 0x0000;
        *(vu16 *)MREG_CNTOVF_IMR = 0x0000;
        *(vu16 *)MREG_CMD = MACCMD_START;

        WSetStaState(STA_CLASS1);
        break;
    }

    *(vu16 *)MREG_MP_POWER_SEQ = 0;
    WDisableTmpttPowerSave();
    TxqOpen(TXQ_OPEN_MP);

    if (pWork->PowerMgtMode == WL_CMDLABEL_PMG_PS) {
        WSetPowerState(PWRSTS_ACT);
    }

    WaitLoop_Rxpe();
}

void WStop(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;

    ClearPeriodicTimeOut();
    ClearTimeOut();

    WSetStaState(STA_CLASS1);
    pWork->bUpdateGameInfo = FALSE;
    pWork->bSynchro = 0;

    *(vu16 *)MREG_IMR = 0;
    *(vu16 *)MREG_CMD = 0;

    *(vu16 *)MREG_TBTT_ENABLE = 0;
    *(vu16 *)MREG_TSF_ENABLE = 0;
    *(vu16 *)MREG_TXCONFIG = 0;
    *(vu16 *)MREG_RXCONFIG = 0;

    switch (pWork->Mode) {
    case WL_CMDLABEL_MODE_PARENT:
        StopBeaconFrame();
        break;
    }

    TxqClose(0xFFFF);

    *(vu16 *)MREG_TXQ_RESET = 0xFFFF;

    DeleteAllTxFrames();
    ReleaseAllWlHeapBuf();
}

u32 BBP_Read(u32 adrs)
{
    *(vu16 *)MREG_BBP_CMD = (u16)(SRLBBP_CMD_READ | (adrs * SRLDEV_CMD_ADRS));

    WaitLoop_BbpAccess();

    return (u32)(*(vu16 *)MREG_BBP_RDAT);
}

u32 BBP_Write(u32 adrs, u32 data)
{
    *(vu16 *)MREG_BBP_WDAT = (u16)data;

    *(vu16 *)MREG_BBP_CMD = (u16)(SRLBBP_CMD_WRITE | (adrs * SRLDEV_CMD_ADRS));

    if (WaitLoop_BbpAccess()) {
        return -1;
    }

    return 0;
}

void RF_Write(u32 data)
{
    *(vu16 *)MREG_RF_DAT = (u16)data;
    *(vu16 *)MREG_RF_CMD = (u16)(data >> 16);

    WaitLoop_RfAccess();
}

u32 RF_Read(u32 data)
{
    RF_Write(data);

    return ((u32) * (vu16 *)MREG_RF_CMD << 16) + (u32) * (vu16 *)MREG_RF_DAT;
}

u16 CalcBbpCRC(void)
{
    u32 adrs, i, data;
    u16 crc;

#if (USE_FLASH)
    adrs = FLASH_ADRS(bbpInitRegs[0]);
#else
    adrs = IRIS_EEPROM_ADRS(bbpRegs[0]);
#endif
    data = 0;
    for (i = crc = 0; i < BBP_REG_SIZE; adrs++, i++) {
#if (USE_FLASH)
        FLASH_Read(adrs, 1, (u8 *)&data);
#else
        WLi_IrisReadEeprom((u16 *)adrs, (u16 *)&data, 1);
#endif

        if (i == 1) {
            data &= 0x0080;
        }

        crc = calc_NextCRC((u8)data, crc);
    }

    return crc;
}

u32 CheckPllLock(void)
{
    if (wlMan->Work.STA != STA_SHUTDOWN) {
        switch (wlMan->Rf.Id) {
        case MDRF2958: {
            u32 bLock, reg, reg_rf, reg_if, reg_ld, dat;

            bLock = 0;
            reg_ld = 0x00800000 + (0x1F << 18);
            reg = 0x00800000 + (0x1B << 18);
            dat = RF_Read(reg) & 0x3FFFF;
            if (dat == 0x3FFFF) {
                return TRUE;
            }
            reg = (0x1B << 18) | (dat & (~0x01800)) | 0x20000;
            reg_if = reg | 0x01000;
            reg_rf = reg | 0x00800;

            RF_Write(reg_if);
            dat = RF_Read(reg_ld);
            if ((dat & (1 << 17)) && ((dat & (1 << 16)) == 0)) {
                bLock++;
            }

            RF_Write(reg_rf);
            dat = RF_Read(reg_ld);
            if ((dat & (1 << 17)) && ((dat & (1 << 16)) == 0)) {
                bLock++;
            }

            return bLock == 2;
        } break;

        case MM3156:
            return RF_Read(0x61100) & 1;
            break;

        default:
            break;
        }
    }

    return TRUE;
}

void WConfigDevice(void)
{
    LPRF_CONFIG pRf = &wlMan->Rf;

    MI_CpuClear16(pRf, sizeof(wlMan->Rf));
#if (USE_FLASH)
    FLASH_Read(FLASH_ADRS(rfDeviceId), 1, (u8 *)&pRf->Id);
    FLASH_Read(FLASH_ADRS(rfBitWidth), 1, (u8 *)&pRf->Bits);
    FLASH_Read(FLASH_ADRS(rfInitRegNum), 1, (u8 *)&pRf->InitNum);
    FLASH_Read(FLASH_ADRS(rfDepChRegNum), 1, (u8 *)&pRf->ChanNum);
#else
    WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(rfId), (u16 *)&pRf->Id, 1);
    WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(rfBits), (u16 *)&pRf->Bits, 1);
    WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(rfInitNum_ChanNum), (u16 *)&pRf->ChanNum, 1);
    WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(rfInitNum), (u16 *)&pRf->InitNum, 1);
    pRf->InitNum = (u32)(pRf->ChanNum >> 4) | (u32)(pRf->InitNum << 4);
    ;
    pRf->ChanNum &= 0x0F;
#endif
}

void InitMac(void)
{
    u32 i;

#if (EMU_MACREG)
    u16 *pSrc = (u16 *)MAC_REG_RBASE;
    u16 *pDest = (u16 *)MAC_REG_BASE;

    for (i = 0; i < MAC_REG_SIZE; i += 2) {
        if (((u32)pSrc != MREG_RDMA_PORT) && ((u32)pSrc != MREG_WDMA_PORT)) {
            *pDest = *pSrc;
        }

        pDest++;
        pSrc++;
    }
#endif

    DbgPuts("Init MAC\r");

    for (i = 0; i < sizeof(macInitRegs) / sizeof(MAC_INIT_REGS); i++) {
        *(vu16 *)(MAC_REG_BASE + macInitRegs[i].adrs) = macInitRegs[i].value;
    }

#if (EMU_MACREG)
    *(vu16 *)MREG_ISR = 0x0000;
#endif
}

void InitBaseBand(void)
{
    u32 i, data, eep_adrs;

    DbgPuts("Init BBP\r\n");

    *(vu16 *)MREG_BBP_CFG = SDV_RF_CLK_4M | SDV_RF_ADD_CLK;

#if (USE_FLASH)
    eep_adrs = FLASH_ADRS(bbpInitRegs[0]);
#else
    eep_adrs = IRIS_EEPROM_ADRS(bbpRegs[0]);
#endif
    for (i = data = 0; i < BBP_REG_SIZE; eep_adrs++, i++) {
#if (USE_FLASH)
        FLASH_Read(eep_adrs, 1, (u8 *)&data);
#else
        WLi_IrisReadEeprom((u16 *)eep_adrs, (u16 *)&data, 1);
#endif
        BBP_Write(i, data);
    }

    BBP_Write(90, 2);
}

void InitRF(void)
{
    u32 eep_adrs, num, reg, bytes, adrs, i, j;
    LPRF_CONFIG pRf = &wlMan->Rf;
#if (USE_FLASH)
    u32 data;
#endif

#if (RF_ID == TESTRF)
    DbgPuts("No Initialize Test RF\r");
#else

#if (USE_FLASH)
    for (i = data = 0; i < sizeof(macTxRxRegAdrs) / 2; i++) {
        FLASH_Read(FLASH_ADRS(macTxRxRegs[i]), 2, (u8 *)&data);

        *(vu16 *)(MAC_REG_BASE + macTxRxRegAdrs[i]) = data;
    }
#else
    if ((pRf->Id >= MAX2822) && (pRf->Id <= MM3156)) {
        MAC_INIT_REGS *pInitReg = (MAC_INIT_REGS *)&macTimingRegs[pRf->Id - 1][0];

        for (i = 0; i < sizeof(macTimingRegs[0]) / sizeof(MAC_INIT_REGS); i++) {
            *(vu16 *)(MAC_REG_BASE + pInitReg[i].adrs) = pInitReg[i].value;
        }
    }
#endif

    {
        reg = SDV_RF_CLK_4M | (((u32)pRf->Bits >> 7) * SDV_RF_ADD_CLK);
        reg |= pRf->Bits & 0x7F;
        *(vu16 *)MREG_RF_CFG = (u16)reg;

#if (USE_FLASH)
        eep_adrs = FLASH_ADRS(rfInit_channelDependData);
#else
        eep_adrs = IRIS_EEPROM_ADRS(rfInit[0]);
#endif
        bytes = (u32)(((pRf->Bits & 0x7F) + 7) / 8);
        num = pRf->InitNum;

        if (pRf->Id == MM3156) {
#if (USE_FLASH)
            FLASH_Read(FLASH_ADRS(rfInit_channelDependData.MM3156Tbl.init[0]) + pRf->InitNum, 1, (u8 *)&pRf->BbpCnt);
#else
            pRf->BbpCnt = 0;
#endif

            for (i = j = 0; i < num; i++, eep_adrs++) {
                reg = 0;
#if (USE_FLASH)
                FLASH_Read(eep_adrs, 1, (u8 *)&reg);
#else
                WLi_IrisReadEeprom((u16 *)eep_adrs, (u16 *)&reg, 1);
#endif
                reg |= ((i << 8) + (5 << 16));
                RF_Write(reg);
            }
        } else {
            for (reg = 0; num > 0; num--, eep_adrs += bytes) {
#if (USE_FLASH)
                FLASH_Read(eep_adrs, bytes, (u8 *)&reg);
#else
                WLi_IrisReadEeprom((u16 *)eep_adrs, (u16 *)&reg, bytes);
#endif
                RF_Write(reg);
                if (pRf->Id == MDRF2958) {
                    adrs = reg >> 18;
                    if (adrs == 9) {
                        pRf->BkReg = reg & ~(0x1F << 10);
                    }
                }
            }
        }
    }

#endif
}

void InitializeAlarm(void)
{
    LPWL_MAN pWl = (LPWL_MAN)&wlMan->TaskMan;

    if (OS_IsAlarmAvailable()) {
        OS_CreateAlarm(&pWl->PeriodicAlarm);
        OS_CreateAlarm(&pWl->Alarm);
        OS_CreateAlarm(&pWl->KeyAlarm);
    } else {
        pWl->Config.DiagResult |= WL_DIAG_ERR_ALARM;
    }
}

static void TimeoutDummy(void *arg)
{
    u32 *flag = (u32 *)arg;

    *flag = FALSE;
}
void WWait(u16 ms)
{
    WWaitus(ms * 1000);
}

void WWaitus(u32 us)
{
    WaitLoop_Waitus(us, TimeoutDummy);
}

void SetupPeriodicTimeOut(u32 ms, void (*pFunc)(void *))
{
    OSTick startTime;

    OS_CancelAlarm(&wlMan->PeriodicAlarm);

    ms = OS_MilliSecondsToTicks(ms);

    startTime = OS_GetTick() + ms;
    OS_SetPeriodicAlarm(&wlMan->PeriodicAlarm, startTime, ms, pFunc, NULL);
}

void ClearPeriodicTimeOut(void)
{
    OS_CancelAlarm(&wlMan->PeriodicAlarm);
}

void WIntervalTimer(void *arg)
{
    LPWL_MAN pWl = (LPWL_MAN)&wlMan->TaskMan;

    pWl->Work.IntervalCount++;

    AddTask(TASK_HIGH_PRIORITY, CAM_TIMER_TASK_ID);

    AddTask(TASK_NORMAL_PRIORITY, UPDATE_APLIST_TASK_ID);

    AddTask(TASK_HIGH_PRIORITY, DEFRAG_TIMER_TASK_ID);

    if (GetHeapBufCount(&pWl->HeapMan.ToWM) != 0) {
        AddTask(TASK_NORMAL_PRIORITY, SEND_MSG_TASK_ID);
    }

    if (pWl->Work.FatalErr) {
        AddTask(TASK_NORMAL_PRIORITY, SEND_FATALERR_MSG_TASK_ID);
    }
}

void SetupTimeOut(u32 ms, void (*pFunc)(void *))
{
    OS_CancelAlarm(&wlMan->Alarm);

    OS_SetAlarm(&wlMan->Alarm, OS_MilliSecondsToTicks(ms), pFunc, NULL);
}

void SetupUsTimeOut(u32 us, void (*pFunc)(void *arg))
{
    OS_CancelAlarm(&wlMan->Alarm);

    OS_SetAlarm(&wlMan->Alarm, OS_MicroSecondsToTicks(us), pFunc, NULL);
}

void ClearTimeOut(void)
{
    OS_CancelAlarm(&wlMan->Alarm);
}

void DMA_Read(void *destAdrs, void *srcAdrs, u32 length)
{
#ifdef SDK_CPU_COPY_WL
    u32 count = ((length + 1) / 2) * 2;
    u32 dst = (u32)destAdrs;
    u32 src = (u32)srcAdrs;
    u32 cpy1, cpy2;

    if (((u32)src + count) > (MAC_MEM_BASE + MBUF_MAC_RX_END)) {
        cpy1 = ((MAC_MEM_BASE + MBUF_MAC_RX_END) - (u32)src);
        cpy2 = count - cpy1;
    } else {
        cpy1 = count;
        cpy2 = 0;
    }

    MI_CpuCopy16((void *)src, (void *)dst, cpy1);
    if (cpy2 != 0) {
        src = src + cpy1 - wlMan->Work.Ofst.RxBuf.Size;
        dst += cpy1;
        MI_CpuCopy16((void *)src, (void *)dst, cpy2);
    }

#else
    u32 count = (length + 1) / 2;
    u32 dmaCnt, cpyCnt;
    OSIrqMask BkIe;

    vu32 *p = (vu32 *)((u32)REG_DMA0SAD_ADDR + WLLIB_DMA_CHANNEL * 12);

    for (cpyCnt = 0; cpyCnt < count; cpyCnt += dmaCnt) {
        dmaCnt = count - cpyCnt;
        if (dmaCnt > wlMan->DmaMaxCount) {
            dmaCnt = wlMan->DmaMaxCount;
        }

        BkIe = OS_DisableIrqMask(WLLIB_DMA_MASK);

        *(vu16 *)MREG_RDMA_STR = (u32)srcAdrs;

        *p = (vu32)(MREG_RDMA_PORT);
        *(p + 1) = (vu32)(destAdrs);
        *(p + 2) = (vu32)(DMA_ENABLE | DMA_TIMMING_IMM | DMA_SRC_FIX | DMA_DEST_INC | DMA_16BIT_BUS | dmaCnt);

        OS_EnableIrqMask(BkIe);

        destAdrs = (void *)((u32)destAdrs + dmaCnt * 2);
        srcAdrs = (void *)((u32)srcAdrs + dmaCnt * 2);
        if ((u32)srcAdrs >= (MAC_MEM_BASE + MBUF_MAC_RX_END)) {
            srcAdrs = (void *)((u32)srcAdrs - wlMan->Work.Ofst.RxBuf.Size);
        }
    }
#endif
}

void DMA_WriteCore(void *destAdrs, void *srcAdrs, u32 count)
{
#ifdef SDK_CPU_COPY_WL

    MI_CpuCopy16(srcAdrs, destAdrs, count * 2);

#else
    u32 dmaCnt, cpyCnt;
    OSIrqMask BkIe;

    vu32 *p = (vu32 *)((u32)REG_DMA0SAD_ADDR + WLLIB_DMA_CHANNEL * 12);

    BkIe = OS_DisableIrqMask(WLLIB_DMA_MASK);

    for (cpyCnt = 0; cpyCnt < count; cpyCnt += dmaCnt) {
        dmaCnt = count - cpyCnt;
        if (dmaCnt > wlMan->DmaMaxCount) {
            dmaCnt = wlMan->DmaMaxCount;
        }

        *(vu16 *)MREG_WDMA_STR = (u32)destAdrs;

        *p = (vu32)srcAdrs;
        *(p + 1) = (vu32)(MREG_WDMA_PORT);
        *(p + 2) = (vu32)(DMA_ENABLE | DMA_TIMMING_IMM | DMA_SRC_INC | DMA_DEST_FIX | DMA_16BIT_BUS | dmaCnt);

        srcAdrs = (void *)((u32)srcAdrs + dmaCnt * 2);
        destAdrs = (void *)((u32)destAdrs + dmaCnt * 2);
    }

    OS_EnableIrqMask(BkIe);
#endif
}

void DMA_Write(void *destAdrs, void *srcAdrs, u32 length)
{
    DMA_WriteCore(destAdrs, srcAdrs, (length + 1) / 2);
}

void DMA_WriteHeaderData(LPTXFRM_MAC destAdrs, LPMAC_HEADER header, u8 *data, u32 length)
{
    DMA_WriteCore(destAdrs, header, (sizeof(MAC_HEADER) + sizeof(DATA_HEADER)) / 2);

    if (length != 0) {
        destAdrs = (LPTXFRM_MAC)((u32)destAdrs + sizeof(MAC_HEADER) + sizeof(DATA_HEADER));
        DMA_WriteCore(destAdrs, data, (length + 1) / 2);
    }
}

void DMA_WepWriteHeaderData(LPTXFRM_MAC destAdrs, LPMAC_HEADER header, u8 *data, u32 length)
{
    DMA_WriteCore(destAdrs, header, (sizeof(MAC_HEADER) + sizeof(DATA_HEADER)) / 2);

    if (length != 0) {
        destAdrs = (LPTXFRM_MAC)((u32)destAdrs + sizeof(MAC_HEADER) + sizeof(DATA_HEADER) + 4);
        DMA_WriteCore(destAdrs, data, (length + 1) / 2);
    }
}

#ifndef SDK_SMALL_BUILD_WL
#ifndef SDK_CPU_COPY_WL
void WLLIB_DmaCopy32(u32 src, u32 dst, u32 length)
{
    u32 cpyLen, dmaLen;

    length = (length + 3) / 4;

    for (cpyLen = 0; cpyLen < length; cpyLen += dmaLen) {
        dmaLen = length - cpyLen;
        if (dmaLen > wlMan->DmaMaxCount / 2) {
            dmaLen = wlMan->DmaMaxCount / 2;
        }

        MI_DmaCopy32(WLLIB_DMA_CHANNEL, (void *)src, (void *)dst, dmaLen * 4);

        src += (dmaLen * 4);
        dst += (dmaLen * 4);
    }
}
#endif
#endif

#ifndef SDK_SMALL_BUILD_WL
#ifndef SDK_CPU_COPY_WL
void WLLIB_DmaCopy16(u32 src, u32 dst, u32 length)
{
    u32 cpyLen, dmaLen;

    length = (length + 1) / 2;

    for (cpyLen = 0; cpyLen < length; cpyLen += dmaLen) {
        dmaLen = length - cpyLen;
        if (dmaLen > wlMan->DmaMaxCount) {
            dmaLen = wlMan->DmaMaxCount;
        }

        MI_DmaCopy16(WLLIB_DMA_CHANNEL, (void *)src, (void *)dst, dmaLen * 2);

        src += (dmaLen * 2);
        dst += (dmaLen * 2);
    }
}
#endif
#endif

#ifndef SDK_SMALL_BUILD_WL
#ifndef SDK_CPU_COPY_WL
void WLLIB_DmaClear32(u32 dst, u32 length)
{
    u32 clrLen, dmaLen;

    length = (length + 3) / 4;

    for (clrLen = 0; clrLen < length; clrLen += dmaLen) {
        dmaLen = length - clrLen;
        if (dmaLen > wlMan->DmaMaxCount / 2) {
            dmaLen = wlMan->DmaMaxCount / 2;
        }

        MI_DmaClear32(WLLIB_DMA_CHANNEL, (void *)dst, dmaLen * 4);

        dst += (dmaLen * 4);
    }
}
#endif
#endif

#ifndef SDK_SMALL_BUILD_WL
#ifndef SDK_CPU_COPY_WL
void WLLIB_DmaClear16(u32 dst, u32 length)
{
    u32 clrLen, dmaLen;

    length = (length + 1) / 2;

    for (clrLen = 0; clrLen < length; clrLen += dmaLen) {
        dmaLen = length - clrLen;
        if (dmaLen > wlMan->DmaMaxCount) {
            dmaLen = wlMan->DmaMaxCount;
        }

        MI_DmaClear16(WLLIB_DMA_CHANNEL, (void *)dst, dmaLen * 2);

        dst += (dmaLen * 2);
    }
}
#endif
#endif

void WL_WriteByte(void *p, u8 data)
{
    WriteByte(p, data);
}

u8 WL_ReadByte(void *p)
{
    u16 data;

    ReadByte(p, data);

    return (u8)data;
}

#if (USE_FLASH == 0)
u16 WLi_IrisReadEeprom(u16 *eep_src, u16 *dst, u32 hw_size)
{
    u8 *pDest = (u8 *)dst;
    u32 adrs = (u32)eep_src;
    u32 r;

    for (; hw_size > 0; hw_size--) {
        r = WLi_EEPROM_Read(adrs / 2);
        if (r == 0xFFFFFFFF) {
            return EEPROM_ERR;
        }
        if (adrs & 1) {
            r >>= 8;
        }

        WL_WriteByte(pDest, r);
        pDest++;
        adrs++;
    }

    return 0;
}

u32 WLi_EEPROM_Read(u32 adrs)
{
    u32 i = 0;

    while (*(vu16 *)MREG_ROM_STS & SRLDEV_STS_BUSY) {
        if (i++ > 10000) {
            DbgPuts("Check ROM Timeout\r");
            return -1;
        }
    }

    *(vu16 *)MREG_ROM_CMD = (u16)(SRLROM_CMD_READ | (adrs * SRLDEV_CMD_ADRS));

    i = 0;
    while (*(vu16 *)MREG_ROM_STS & SRLDEV_STS_BUSY) {
        if (i++ > 10000) {
            DbgPuts("Read ROM Timeout\r");
            return -1;
        }
    }

    return (u32)(*(vu16 *)MREG_ROM_RDAT);
}
#endif

void RND_init(u32 a, u32 b)
{
    LPRAND_CTRL pRand = &wlMan->Rand;

    pRand->a = (u16)((a & 0xFFF8) + 5);
    pRand->b = (u16)(b | 1);
}

void RND_seed(u32 seed)
{
    wlMan->Rand.seed = (u16)seed;
}

u16 RND_rand(void)
{
    LPRAND_CTRL pRand = &wlMan->Rand;

    pRand->seed = (u16)((u32)pRand->seed * (u32)pRand->a + (u32)pRand->b);

    return pRand->seed;
}

static const u16 crc16_table[16] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};
u16 calc_NextCRC(u8 data, u16 total)
{
    u16 r1;

    r1 = crc16_table[total & 0xf];
    total = (total >> 4) & 0x0FFF;
    total = total ^ r1 ^ crc16_table[data & 0xf];

    r1 = crc16_table[total & 0xf];
    total = (total >> 4) & 0x0fff;
    total = total ^ r1 ^ crc16_table[(data >> 4) & 0xf];

    return total;
}

static u32 WCheckTxBufIdBeforeFrame(LPTXQ pTxq)
{
    u16 *pId;

    pId = (u16 *)((u32)pTxq->pMacFrm - 4);
    if ((pId[0] != DESTROY_TXBUF_ID0) || (pId[1] != DESTROY_TXBUF_ID1)) {
        DbgSetDDO(DDO_WL_MAC, DDO_WL_MAC_DESTROY_TXBUF);

        pTxq->pMacFrm->MacHeader.Tx.MPDU = 1;

        pId[0] = DESTROY_TXBUF_ID0;
        pId[1] = DESTROY_TXBUF_ID1;

        pId = (u16 *)&pTxq->pMacFrm->Dot11Header;
        pId[0] = DESTROY_TXBUF_ID0;
        pId[1] = DESTROY_TXBUF_ID1;

        DbgClrDDO(DDO_WL_MAC, DDO_WL_MAC_DESTROY_TXBUF);

        wlMan->Work.TxBufErrCount++;

        return 1;
    }

    return 0;
}

static void WaitMacStop(void)
{
    u32 i, state;

    *(vu16 *)MREG_CMD = 0;

    for (i = 16; i > 0; i--) {
        state = *(vu16 *)MREG_MAC_STATE;
        if ((state == MAC_STATE_STANDBY) || (state == MAC_STATE_SLEEP)) {
            break;
        }

        DbgPrint("[W:%x]", *(vu16 *)MREG_MAC_STATE);
    }

    if (i == 0) {
        DbgPrint("MAC STOP TIMEOUT!!\r\n");
    }
}

static void RestoreTxFrame(LPTXQ pTxq)
{
    if (pTxq->Busy) {
        WaitMacStop();

        CopyTxFrmToMacBuf(pTxq->pMacFrm, (WlMaDataReq *)CalcReqAdrsFromFrame(pTxq->pFrm));

        DbgPrint("Restore TxFrame\r\n");

        wlMan->Work.TxBufResCount++;
    }
}

void WCheckTxBuf(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    LPTX_CTRL pTxCtrl = &wlMan->TxCtrl;

    switch (wlMan->Work.Mode) {
    case WL_CMDLABEL_MODE_PARENT:
        if (WCheckTxBufIdBeforeFrame(&pTxCtrl->Beacon)) {
            WaitMacStop();

            MakeBeaconFrame();

            DbgPrint("Restore Beacon frame\r\n");
            DbgPrint("Maybe destroyed Beacon frame\r\n");
        }
        if (WCheckTxBufIdBeforeFrame(&pTxCtrl->Txq[2])) {
            DbgPrint("Maybe destroyed Txq[2] frame\r\n");

            RestoreTxFrame(&pTxCtrl->Txq[2]);
        }
        break;

    case WL_CMDLABEL_MODE_CHILD:
        if (WCheckTxBufIdBeforeFrame(&pTxCtrl->Key[1])) {
            DbgPrint("Maybe destroyed Key[1] frame\r\n");
        }
        if (WCheckTxBufIdBeforeFrame(&pTxCtrl->Txq[2])) {
            DbgPrint("Maybe destroyed Txq[2] frame\r\n");

            if (pTxCtrl->Txq[2].Busy) {
                WaitMacStop();
            }

            MakePsPollFrame(pWork->AID);

            DbgPrint("Restore PsPoll Frame\r\n");

            pWork->TxBufResCount++;
        }
        break;

    case WL_CMDLABEL_MODE_HOTSPOT:
        break;
    }

    if (WCheckTxBufIdBeforeFrame(&pTxCtrl->Txq[1])) {
        DbgPrint("Maybe destroyed Txq[1] frame\r\n");

        RestoreTxFrame(&pTxCtrl->Txq[1]);
    }
    if (WCheckTxBufIdBeforeFrame(&pTxCtrl->Txq[0])) {
        DbgPrint("Maybe destroyed Txq[0] frame\r\n");

        RestoreTxFrame(&pTxCtrl->Txq[0]);
    }

    if (*(vu16 *)MREG_CMD == 0) {
        *(vu16 *)MREG_CMD = 1;
    }
}

void SetFatalErr(u32 errCode)
{
    u32 x;

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    wlMan->Work.FatalErr |= errCode;

    OS_EnableIrqMask(x);

    DbgPrint("Fatal Err(%04x)\r\n", errCode);

    AddTask(TASK_NORMAL_PRIORITY, SEND_FATALERR_MSG_TASK_ID);
}

void SendFatalErrMsgTask(void)
{
    LPWORK_PARAM pWork = &wlMan->Work;
    WlMaFatalErrInd *pInd;
    u32 x;

    if (pWork->FatalErr == 0) {
        return;
    }

    pInd = (WlMaFatalErrInd *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, sizeof(WlMaFatalErrInd));
    if ((u32)pInd == HEAPBUF_NOT_ENOUGH_MEMORY) {
        return;
    }

    pInd->header.code = WL_CMDCODE_MA_FATAL_ERR_IND;
    pInd->header.length = CalcIndMsgLength(WlMaFatalErrInd);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    pInd->errCode = pWork->FatalErr;
    pWork->FatalErr = 0;

    OS_EnableIrqMask(x);

    SendMessageToWmDirect(&wlMan->HeapMan.TmpBuf, pInd);
}

#if (BBP_RF_REGCHK)

static const u16 BBPDiagSkipAdrsRelease[] = {
    0,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    39,
    77,
    93,
    94,
    95,
    96,
    97,
    100,
    102,
    0
};

u32 CheckBbpRfRegs(u16 *p);
u32 CheckBbpRfRegs(u16 *p)
{
#if (USE_FLASH)
    u32 bbpErrCnt, rfErrCnt;
    u32 i, k, data, eep_adrs, rd, reg, regr, regw;
    u16 *pBuf = &p[1];
    u16 *pBbpCnt = p;
    u16 *pRfCnt;

    eep_adrs = FLASH_ADRS(bbpInitRegs[0]);
    for (bbpErrCnt = i = k = data = 0; i < BBP_REG_SIZE; eep_adrs++, i++) {
        if (i == BBPDiagSkipAdrsRelease[k]) {
            k++;
            continue;
        } else if (i == 30) {
            FLASH_Read(FLASH_ADRS(rfInit_channelDependData.RF2958Tbl.ch_PwrBB[wlMan->Work.CurrChannel - 1]), 1, (u8 *)&data);
        } else {
            FLASH_Read(eep_adrs, 1, (u8 *)&data);
        }
        rd = BBP_Read(i);

        if (data != rd) {
            bbpErrCnt++;
            *pBuf++ = i;
            *pBuf++ = data;
            *pBuf++ = rd;
            DbgPrint("BBP err %02x = %02x -> %02x\r\n", i, data, rd);
        }
    }
    pRfCnt = pBuf++;

    eep_adrs = FLASH_ADRS(rfInit_channelDependData);
    for (rfErrCnt = reg = i = 0; i < wlMan->Rf.InitNum; i++, eep_adrs += 3) {
        if (i == 2) {
            FLASH_Read(FLASH_ADRS(rfInit_channelDependData.RF2958Tbl.ch_Reg[wlMan->Work.CurrChannel - 1][0]), 3, (u8 *)&reg);
        } else if (i == 3) {
            FLASH_Read(FLASH_ADRS(rfInit_channelDependData.RF2958Tbl.ch_Reg[wlMan->Work.CurrChannel - 1][3]), 3, (u8 *)&reg);
        } else {
            FLASH_Read(eep_adrs, 3, (u8 *)&reg);
        }

        regw = reg & 0xFFFC0000;
        regr = RF_Read(regw | 0x800000) & 0x7FFFFF;

        if (reg != regr) {
            rfErrCnt++;
            *pBuf++ = (u16)reg;
            *pBuf++ = (u16)(reg >> 16);
            *pBuf++ = (u16)regr;
            *pBuf++ = (u16)(regr >> 16);
            DbgPrint("rf verify err(%x:%x)\r\n", reg, regr);
        }
    }

    if ((bbpErrCnt + rfErrCnt) != 0) {
        *pBbpCnt = bbpErrCnt;
        *pRfCnt = rfErrCnt;
    }

    return bbpErrCnt + rfErrCnt;
#else
#pragma unused(p)
    return 0;
#endif
}
#endif

void TerminateWlTask(void)
{
    DbgPrint("TerminateWlTask\r\n");

    wlMan->Config.DiagResult |= 0x8000;

    if (wlMan->Work.STA != STA_SHUTDOWN) {
        WStop();

        if (wlMan->MLME.State != MLME_STATE_IDLE) {
            wlMan->MLME.State = MLME_STATE_IDLE;

            wlMan->MLME.pCfm.Cfm->resultCode = WL_CMDRES_REFUSE;
            IssueMlmeConfirm();
        }

        WShutdown();
    }

    while (DeleteTask(TASK_LOW_PRIORITY) != TASK_LAST) {}
    AddTask(TASK_LOW_PRIORITY, RELEASE_WL_TASK_ID);
}

void ReleaseWlTask(void)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;

    DbgPrint("ReleaseWlTask\r\n");

    ReleaseIntr();

    ReleaseHeapBuf(&pHeapMan->TmpBuf, (void *)((u32)wlMan->pFlashImg - WL_RSV * 2));

    ReleaseHeapBuf(&pHeapMan->TmpBuf, (void *)((u32)wlMan->Work.GameInfoAdrs - WL_RSV * 2));

#ifdef SDK_TEG
    OS_UnLockCartridge(wlMan->lockID);
    wlMan->bUnLocked = TRUE;
#endif

    OS_ExitThread();
}
