#define __DEVCMD_C_
#define __INSYSROM__

#include "WlSys.h"
#include "DbgChar.h"

#include "WlLib.h"
#include "WlCmdIf.h"
#include "WlNic.h"
#include "MAC.h"
#include "Flash.h"
#include "DevCmd.h"

u16 DEV_ShutdownReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    pCfmt->header.length = CalcCfmMsgLength(WlDevShutdownCfm);

    if ((wlMan->Work.STA != STA_SHUTDOWN) && (wlMan->Work.STA != STA_IDLE)) {
        return WL_CMDRES_STATE_WRONG;
    }

    WSetStaState(STA_SHUTDOWN);

    return WL_CMDRES_SUCCESS;
}

u16 DEV_IdleReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    pCfmt->header.length = CalcCfmMsgLength(WlDevIdleCfm);

    if (wlMan->Work.STA > STA_CLASS1) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (wlMan->Work.bSynchro) {
        return WL_CMDRES_STATE_WRONG;
    }

#ifndef SDK_NOCHK_ERR_WL
    if (FLASH_VerifyCheckSum(NULL) != 0) {
        return WL_CMDRES_FLASH_ERR;
    }
#endif

    WSetStaState(STA_IDLE);

    return WL_CMDRES_SUCCESS;
}

u16 DEV_Class1ReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    pCfmt->header.length = CalcCfmMsgLength(WlDevClass1Cfm);

    if ((wlMan->Work.STA == STA_IDLE) || ((wlMan->Work.STA == STA_CLASS1) && (wlMan->Work.bSynchro == 0))) {
        WSetStaState(STA_CLASS1);

        return WL_CMDRES_SUCCESS;
    }

    return WL_CMDRES_STATE_WRONG;
}

u16 DEV_RebootReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    pCfmt->header.length = CalcCfmMsgLength(WlDevRebootCfm);

    if (wlMan->Work.STA >= STA_CLASS1) {
        WStop();
    }

    WlessLibReboot();

    return WL_CMDRES_SUCCESS;
}

u16 DEV_ClearWlInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    if (wlMan->Work.STA == STA_SHUTDOWN) {
        return WL_CMDRES_STATE_WRONG;
    }

    pCfmt->header.length = CalcCfmMsgLength(WlDevClrInfoCfm);

    WInitCounter();

    return WL_CMDRES_SUCCESS;
}

u16 DEV_GetVerInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    static const u8 wlVersion[8] ATTRIBUTE_ALIGN(2) = WL_REVISION;

    WlDevGetVerInfoCfm *pCfm = (WlDevGetVerInfoCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlDevGetVerInfoCfm);

    MI_CpuCopy16((u16 *)wlVersion, (u16 *)pCfm->wlVersion, sizeof(wlVersion));
    pCfm->macVersion = *(vu16 *)MREG_VERSION;
    if (wlMan->WlOperation & 0x8000) {
        pCfm->bbpVersion[0] = BBP_Read(0);
        pCfm->bbpVersion[1] = CalcBbpCRC();
    } else {
        pCfm->bbpVersion[0] = 0x6D;
        pCfm->bbpVersion[1] = 0x933D;
    }
    if (wlMan->WlOperation & 0x4000) {
        pCfm->rfVersion = wlMan->Rf.Id;
    } else {
        pCfm->rfVersion = 2;
    }

    return WL_CMDRES_SUCCESS;
}

u16 DEV_GetWlInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlDevGetInfoCfm *pCfm = (WlDevGetInfoCfm *)pCfmt;

    if (wlMan->Work.STA == STA_SHUTDOWN) {
        return WL_CMDRES_STATE_WRONG;
    }

    pCfm->header.length = CalcCfmMsgLength(WlDevGetInfoCfm);

    WUpdateCounter();
    WLLIB_DmaCopy32((u32)&wlMan->Counter, (u32)&pCfm->counter, sizeof(WlCounter));

    return WL_CMDRES_SUCCESS;
}

u16 DEV_GetStateReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlDevGetStateCfm *pCfm = (WlDevGetStateCfm *)pCfmt;

    pCfm->header.length = CalcCfmMsgLength(WlDevGetStateCfm);

    pCfm->state = wlMan->Work.STA;

    return WL_CMDRES_SUCCESS;
}

u16 DEV_TestSignalReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlDevTestSignalReq *pReq = (WlDevTestSignalReq *)pReqt;
    WlDevTestSignalCfm *pCfm = (WlDevTestSignalCfm *)pCfmt;
    LPWORK_PARAM pWork = &wlMan->Work;
    u32 tmp;

    pCfm->header.length = CalcCfmMsgLength(WlDevTestSignalCfm);

    if ((pWork->STA & STA_MAIN_MASK) != STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    if (pReq->control > MAX_TEST_SIGNAL_CONTROL) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if ((pReq->rate != RF_RATE_1M) && (pReq->rate != RF_RATE_2M)) {
        return WL_CMDRES_INVALID_PARAM;
    }
    if (pReq->signal > MAX_TEST_SIGNAL) {
        return WL_CMDRES_INVALID_PARAM;
    }

#ifndef SDK_NOCHK_ERR_WL
    if (FLASH_VerifyCheckSum(NULL) != 0) {
        return WL_CMDRES_FLASH_ERR;
    }
#endif

    switch (pReq->control) {
    case WL_CMDLABEL_TEST_SIGNAL_ON:
        if (pWork->STA != STA_IDLE) {
            return WL_CMDRES_STATE_WRONG;
        }

        tmp = 0;
        FLASH_DirectRead(FLASH_ADRS(bbpInitRegs[1]), 1, (u8 *)&tmp);
        if (BBP_Read(0x01) != tmp) {
            BBP_Write(0x01, tmp);

            WWaitus(5000);
        }

        switch (pReq->signal) {
        case WL_CMDLABEL_TEST_SIGNAL_NOMOD_0:
        case WL_CMDLABEL_TEST_SIGNAL_NOMOD_1:
        case WL_CMDLABEL_TEST_SIGNAL_PN15:
            pWork->STA = STA_IDLE_TEST;

            WSetChannel(pReq->channel, TRUE);

            *(vu16 *)MREG_SET_FORCE_POWER = FPWR_STS_ACTIVE;

            WWaitus(1500);

            pWork->PN15Rate = pReq->rate;

            tmp = BBP_Read(2);
            if (pReq->signal <= WL_CMDLABEL_TEST_SIGNAL_NOMOD_1) {
                tmp |= 0x10;
            }

            *(vu16 *)MREG_SP_DIRECT = pReq->rate;
            if (pReq->signal == WL_CMDLABEL_TEST_SIGNAL_NOMOD_1) {
                tmp |= 0x20;
                *(vu16 *)MREG_SERIAL_DAT_SEL = 3;
            } else {
                *(vu16 *)MREG_SERIAL_DAT_SEL = pReq->signal;
            }
            BBP_Write(2, tmp);
            *(vu16 *)MREG_EN_DIRECT_CTL = 0x0823; // Long, Short:0x0923

#ifdef SDK_INCLUDE_MAX2822
            if (wlMan->Rf.Id == MAX2822) {
                SetupPeriodicTimeOut(50, TestSignalRestart);
            }
#endif
            break;

        case WL_CMDLABEL_TEST_SIGNAL_01PTN_SCR_ON:
        case WL_CMDLABEL_TEST_SIGNAL_01PTN_SCR_OFF:
            CarrierSuppresionSignal(pReq);
            break;
        }
        break;

    case WL_CMDLABEL_TEST_SIGNAL_OFF:
        if (pWork->STA == STA_IDLE_TEST) {
            ClearPeriodicTimeOut();

            *(vu16 *)MREG_EN_DIRECT_CTL = 0x0000;
            *(vu16 *)MREG_SERIAL_DAT_SEL = 0x0001;

            *(vu16 *)MREG_SET_FORCE_POWER = FPWR_STS_DISABLE;

            tmp = BBP_Read(2);
            tmp &= ~0x30;
            BBP_Write(2, tmp);
        } else if (pWork->STA == STA_IDLE_TEST2) {
            pWork->SigTest2 = 0;
            while (*(vu16 *)MREG_CMD != 0) {}

            BBP_Write(6, pWork->Scrambler);
        } else {
            return WL_CMDRES_STATE_WRONG;
        }

        pWork->STA = STA_IDLE;
        break;
    }

    return WL_CMDRES_SUCCESS;
}

#ifdef SDK_INCLUDE_MAX2822
static void TestSignalRestart(void *arg)
{
    u32 k;
    BOOL i;

    i = OS_DisableInterrupts();

    *(vu16 *)MREG_EN_DIRECT_CTL ^= 1;

    *(vu16 *)MREG_EN_DIRECT_CTL |= 0x10;

    OS_SpinWait(HW_CPU_CLOCK_ARM7 / 500000);

    *(vu16 *)MREG_EN_DIRECT_CTL &= ~0x10;

    OS_SpinWait(HW_CPU_CLOCK_ARM7 / 500000);

    *(vu16 *)MREG_SP_DIRECT = wlMan->Work.PN15Rate;

    *(vu16 *)MREG_EN_DIRECT_CTL ^= 1;

    OS_RestoreInterrupts(i);
}
#endif //	SDK_INCLUDE_MAX2822

static void CarrierSuppresionSignal(WlDevTestSignalReq *pReq)
{
    u32 i;
    u16 *p;
    LPTXFRM_MAC pTxFrm = (LPTXFRM_MAC)MAC_MEM_BASE;
    LPWORK_PARAM pWork = &wlMan->Work;

    WStart();
    WStop();

    pWork->Scrambler = BBP_Read(6);
    if (pReq->signal == WL_CMDLABEL_TEST_SIGNAL_01PTN_SCR_OFF) {
        BBP_Write(6, 0);
    }

    MI_CpuClear16(&pTxFrm->MacHeader, sizeof(pTxFrm->MacHeader));
    pTxFrm->MacHeader.Tx.Service_Rate = RF_RATE_2M;
    pTxFrm->MacHeader.Tx.MPDU = 2000;

    p = (u16 *)&pTxFrm->Dot11Header;
    for (i = 0; i < 2000 + sizeof(pTxFrm->Dot11Header) + 4; i += 2) {
        *p++ = 0x5555;
    }

    pTxFrm->Dot11Header.FrameCtrl.Data = 0x0008;

    *(vu16 *)MREG_TX_TEST_MODE = 0x0006;

    pWork->STA = STA_IDLE_TEST2;
    pWork->SigTest2 = 1;

    WSetChannel(pReq->channel, TRUE);

    *(vu16 *)MREG_SET_FORCE_POWER = FPWR_STS_ACTIVE;

    WWaitus(1500);

    *(vu16 *)MREG_IMR = MIMR_TXEND;

    *(vu16 *)MREG_CMD = MACCMD_START;

    *(vu16 *)MREG_TXQ_OPEN = TXQ_OPEN_TXQ0;
    *(vu16 *)MREG_TXQ0 = TXQ_ENABLE | (u16)SetMacTxAdrs(pTxFrm);
}

void IntrCarrierSuppresionSignal(void)
{
    LPTXFRM_MAC pTxFrm = (LPTXFRM_MAC)MAC_MEM_BASE;

    if (wlMan->Work.SigTest2) {
        pTxFrm->MacHeader.Tx.Status = 0;
        pTxFrm->MacHeader.Tx.rsv_RetryCount = 0;

        *(vu16 *)MREG_TXQ0 |= TXQ_ENABLE;
    } else {
        *(vu16 *)MREG_TXQ_CLOSE = TXQ_CLOSE_TXQ0;

        *(vu16 *)MREG_CMD = 0;

        *(vu16 *)MREG_IMR = MIMR_TXEND;
        *(vu16 *)MREG_ISR = 0xFFFF;

        *(vu16 *)MREG_TX_TEST_MODE = 0x0000;

        *(vu16 *)MREG_SET_FORCE_POWER = FPWR_STS_DISABLE;
    }
}

u16 DEV_TestRxReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt)
{
    WlDevTestRxReq *pReq = (WlDevTestRxReq *)pReqt;
    WlDevTestRxCfm *pCfm = (WlDevTestRxCfm *)pCfmt;
    LPWORK_PARAM pWork = &wlMan->Work;

    pCfm->header.length = CalcCfmMsgLength(WlDevTestRxCfm);

    if ((pWork->STA & STA_MAIN_MASK) != STA_IDLE) {
        return WL_CMDRES_STATE_WRONG;
    }

    switch (pReq->control) {
    case WL_CMDLABEL_TEST_RX_ON:
        if (pWork->STA != STA_IDLE) {
            return WL_CMDRES_STATE_WRONG;
        }

        WSetChannel(pReq->channel, TRUE);

        pWork->Mode = WL_CMDLABEL_MODE_TEST;
        WStart();
        WSetForcePowerState(FPWR_STS_ACTIVE);

        pWork->STA = STA_IDLE_TEST;
        break;

    case WL_CMDLABEL_TEST_RX_OFF:
        if (pWork->STA == STA_IDLE_TEST) {
            WSetForcePowerState(FPWR_STS_DISABLE);
            WStop();
        } else {
            return WL_CMDRES_STATE_WRONG;
        }

        pWork->STA = STA_IDLE;
        break;
    }

    return WL_CMDRES_SUCCESS;
}
