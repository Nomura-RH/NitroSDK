#include <nitro/card/pullOut.h>
#include <nitro/mb/mb.h>

#include "card_rom.h"

static BOOL detectPullOut = FALSE;

static void CARDi_CallbackForPulledOut(PXIFifoTag tag, u32 data, BOOL err);
static void CARDi_TerminateARM7(void);
static void CARDi_SendtoPxi(u32 data, u32 wait);

void CARD_InitPulledOutCallback(void)
{
    static BOOL isInitialized;
    if (isInitialized) {
        return;
    }
    isInitialized = TRUE;

    PXI_Init();

    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_CARD, PXI_PROC_ARM9)) {
    }

    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CARD, CARDi_CallbackForPulledOut);
}

static void CARDi_CallbackForPulledOut(PXIFifoTag tag, u32 data, BOOL err)
{
    u32 command = data & CARD_PXI_COMMAND_MASK;

    if (command == CARD_PXI_COMMAND_TERMINATE) {
        CARDi_TerminateARM7();
    } else {
#ifndef SDK_FINALROM
        OS_Panic("illegal card pxi command.");
#else
        OS_Panic("");
#endif
    }
}

#define CARD_USE_IREQ_BUF  0x027FFE1F
#define CARD_USE_IREQ_MASK 0x80

BOOL CARD_IsPulledOut(void)
{
    if (!detectPullOut) {
        if (*(vu8 *)CARD_USE_IREQ_BUF & CARD_USE_IREQ_MASK) {
            CARD_CompareCardID();
        } else {
            CARD_IsCardIreqLo();
        }
    }

    return detectPullOut;
}

BOOL CARD_CompareCardID(void)
{
    BOOL retval = TRUE;
    s32 lockID = OS_GetLockID();
    if (lockID != OS_LOCK_ID_ERROR) {
        if (OS_TryLockCard(lockID) == OS_LOCK_SUCCESS) {
            vu32 iplCardID = *(vu32 *)((*(u16 *)HW_CHECK_DEBUGGER_SW == 0) ? HW_RED_RESERVED : HW_BOOT_CHECK_INFO_BUF);
            u32 cardID = CARDi_ReadRomID();

            retval = (cardID == (u32)iplCardID);

            OS_UnlockCard(lockID);
        }
        OS_ReleaseLockID(lockID);
    }

    detectPullOut = !retval;

    return retval;
}

BOOL CARD_IsCardIreqLo(void)
{
    BOOL retval = TRUE;

    if (reg_OS_IF & OS_IE_CARD_IREQ) {
        retval = FALSE;
        detectPullOut = TRUE;
    }

    return retval;
}

#define CARD_POLLING_INTERVAL 10
#define CARDi_COUNT_NOT_SET   0xFFFFFFFF

static BOOL isCardPullOut = FALSE;

void CARD_CheckPullOut_Polling(void)
{
    static u32 nextCount = CARDi_COUNT_NOT_SET;
    static BOOL skipCheck = FALSE;

    if (isCardPullOut || *(u16 *)HW_WM_BOOT_BUF == MB_TYPE_MULTIBOOT) {
        return;
    }

    if (nextCount == CARDi_COUNT_NOT_SET) {
        nextCount = OS_GetVBlankCount() + CARD_POLLING_INTERVAL;
        return;
    }

    if (OS_GetVBlankCount() < nextCount) {
        return;
    }
    nextCount = OS_GetVBlankCount() + CARD_POLLING_INTERVAL;

    static BOOL isFirstCheck = TRUE;

    if (CARD_IsPulledOut()) {
        isCardPullOut = TRUE;
        if ((((const CARDRomHeader *)CARD_GetRomHeader())->game_code == 0) && isFirstCheck) {
            return;
        }
    }
    isFirstCheck = FALSE;

    if (isCardPullOut) {
        CARDi_SendtoPxi(CARD_PXI_COMMAND_PULLED_OUT, 100);
    }
}

static void CARDi_TerminateARM7(void)
{
    MI_StopDma(0);
    MI_StopDma(1);
    MI_StopDma(2);
    MI_StopDma(3);

    CTRDG_VibPulseEdgeUpdate(NULL);

    OSIntrMode bak_psr = OS_DisableInterrupts();

    SND_BeginSleep();

    WVR_Shutdown();

    OS_RestoreInterrupts(bak_psr);

    OS_Terminate();
}

static void CARDi_SendtoPxi(u32 data, u32 wait)
{
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_CARD, data, FALSE) != PXI_FIFO_SUCCESS) {
        SVC_WaitByLoop(wait);
    }
}
