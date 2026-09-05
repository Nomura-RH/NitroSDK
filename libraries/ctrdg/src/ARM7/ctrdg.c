#include "nitro/ctrdg/ARM7/ctrdg.h"

#include "nitro/ctrdg.h"
#include "nitro/ctrdg/common/ctrdg_common.h"
#include <nitro/hw/ARM7/ioreg_OS.h>
#include <nitro/mi/exMemory.h>
#include <nitro/os/common/alarm.h>
#include <nitro/os/common/interrupt.h>
#include <nitro/os/common/spinLock.h>
#include <nitro/os/common/systemCall.h>
#include <nitro/os/common/tick.h>
#include <nitro/pxi/common/fifo.h>

#include "ctrdg_work.h"

typedef struct CTRDGWorkSp {
    union {
        volatile u32 cmd32[CTRDG_PXI_PACKET_MAX];
        volatile u16 cmd16[CTRDG_PXI_PACKET_MAX * 2];
    };
    volatile enum {
        CTRDG_STATUS_READY = 0,
        CTRDG_STATUS_INIT_MODULE_INFO
    } status;
    volatile u16 count;
    volatile u16 pad[1];
} CTRDGWorkSp;

static CTRDGWorkSp ctw_sp;
VibSwitch current_vib = VIB_STOP;

#ifdef SDK_COMPONENT_VIB
static u16 lock_id;
static OSAlarm pulse_edge_alarm;

#define ALARM_RETRYINTERVAL    (OS_MilliSecondsToTicks(1))
#define SPINWAIT_RETRYINTERVAL 33000
#endif

static void CTRDGi_CallbackForInitModuleInfo(PXIFifoTag tag, u32 data, BOOL err);
static void CTRDGi_CallbackForPulledOut(PXIFifoTag tag, u32 data, BOOL err);
static BOOL CTRDGi_IsNinLogoOfAgb(u16 *logop);
static void CTRDGi_TerminateARM7(void);
static void CTRDGi_CallbackForSetPhi(PXIFifoTag tag, u32 data, BOOL err);
#ifdef SDK_COMPONENT_VIB
static void CTRDGi_CallbackForCTREx(PXIFifoTag tag, u32 data, BOOL err);
static void CTRDGi_VibOnOff(u32 sw);
static inline u32 get_vib_tick(CTRDGPulseVib *ctrdg_ex_data);
static inline u32 get_stop_tick(CTRDGPulseVib *ctrdg_ex_data);
#endif

void CTRDG_Init(void)
{
    static BOOL isInitialized;
#ifdef SDK_COMPONENT_VIB
    OS_InitTick();
    OS_InitAlarm();

    OS_CreateAlarm(&pulse_edge_alarm);
#endif
    if (isInitialized) {
        return;
    }
    isInitialized = TRUE;

    CTRDGi_InitCommon();

#ifdef SDK_COMPONENT_VIB
    s32 ret = OS_GetLockID();

    if (ret == OS_LOCK_ID_ERROR) {
        return;
    }
    lock_id = ret;
#endif

    PXI_Init();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CTRDG, CTRDGi_CallbackForInitModuleInfo);

    CTRDGi_InitModuleInfo();

    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CTRDG, CTRDGi_CallbackForPulledOut);
#ifdef SDK_COMPONENT_VIB
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CTRDG_Ex, CTRDGi_CallbackForCTREx);
#endif
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CTRDG_PHI, CTRDGi_CallbackForSetPhi);
}

void CTRDGi_InitModuleInfo(void)
{
#ifndef SDK_SMALL_BUILD
    OSIrqMask lastIE;
    BOOL lastIME;
    static BOOL isInitialized;

    CTRDGModuleInfo *cip = CTRDGi_GetModuleInfoAddr();

    if (isInitialized) {
        return;
    }
    isInitialized = TRUE;

    if (!(reg_OS_PAUSE & REG_OS_PAUSE_CHK_MASK)) {
        return;
    }

    lastIE = OS_SetIrqMask(OS_IE_MPFIFO_RECV);
    lastIME = OS_EnableIrq();

    while (ctw_sp.status != CTRDG_STATUS_INIT_MODULE_INFO) {
        SVC_WaitByLoop(0x100);
    }

    CTRDGHeader *hdr = (CTRDGHeader *)(HW_MAIN_MEM + ((ctw_sp.cmd32[0] & CTRDG_PXI_COMMAND_PARAM_MASK) >> CTRDG_PXI_COMMAND_PARAM_SHIFT) * 32);
    cip->isAgbCartridge = CTRDGi_IsNinLogoOfAgb((u16 *)(hdr->nintendoLogo));

    CTRDGi_SendtoPxi(CTRDG_PXI_COMMAND_INIT_MODULE_INFO);

    OS_RestoreIrq(lastIME);
    OS_SetIrqMask(lastIE);
#endif
}

static void CTRDGi_CallbackForInitModuleInfo(PXIFifoTag tag, u32 data, BOOL err)
{
    if ((data & CTRDG_PXI_COMMAND_MASK) == CTRDG_PXI_COMMAND_INIT_MODULE_INFO) {
        ctw_sp.cmd32[0] = data;
        ctw_sp.status = CTRDG_STATUS_INIT_MODULE_INFO;
    } else {
#ifndef SDK_FINALROM
        OS_Panic("illegal Cartridge pxi command.");
#else
        OS_Panic("");
#endif
    }
}

static void CTRDGi_CallbackForPulledOut(PXIFifoTag tag, u32 data, BOOL err)
{
    if ((data & CTRDG_PXI_COMMAND_MASK) == CTRDG_PXI_COMMAND_TERMINATE) {
        CTRDGi_TerminateARM7();
    } else {
#ifndef SDK_FINALROM
        OS_Panic("illegal Cartridge pxi command.");
#else
        OS_Panic("");
#endif
    }
}

#ifdef SDK_COMPONENT_VIB
static void CTRDGi_CallbackForCTREx(PXIFifoTag tag, u32 data, BOOL err)
{
    CTRDG_VibPulseEdgeUpdate((void *)data);
}

void CTRDG_VibPulseEdgeUpdate(void *ctrdg_ex_data)
{
    BOOL is_locked_by_other;

    if (ctrdg_ex_data && (((CTRDGPulseVib *)ctrdg_ex_data)->current_pos == 0)) {
        ((CTRDGPulseVib *)ctrdg_ex_data)->current_count++;
        if (((CTRDGPulseVib *)ctrdg_ex_data)->repeat_num != 0 && ((CTRDGPulseVib *)ctrdg_ex_data)->current_count > ((CTRDGPulseVib *)ctrdg_ex_data)->repeat_num) {
            ctrdg_ex_data = NULL;
        }
    }

    if (!ctrdg_ex_data || !(((CTRDGPulseVib *)ctrdg_ex_data)->is_enable)) {
        OSIntrMode irq = OS_DisableInterrupts();
        if (current_vib == VIB_START) {
            BOOL finished_stop_process = FALSE;
            while (!finished_stop_process) {
                is_locked_by_other = (OS_ReadOwnerOfLockCartridge() & OS_SUBP_LOCKED_FLAG);
                if (is_locked_by_other || (OS_TryLockCartridge(lock_id) == OS_LOCK_SUCCESS)) {
                    CTRDGi_VibOnOff(VIB_STOP);
                    finished_stop_process = TRUE;
                    if (!is_locked_by_other) {
                        OS_UnlockCartridge(lock_id);
                    }
                } else {
                    OS_SpinWait(SPINWAIT_RETRYINTERVAL);
                    continue;
                }
            }
        }
        OS_CancelAlarm(&pulse_edge_alarm);
        OS_RestoreInterrupts(irq);
    } else if (ctrdg_ex_data) {
        is_locked_by_other = (OS_ReadOwnerOfLockCartridge() & OS_SUBP_LOCKED_FLAG);
        if (is_locked_by_other || (OS_TryLockCartridge(lock_id) == OS_LOCK_SUCCESS)) {
            if (((CTRDGPulseVib *)ctrdg_ex_data)->current_pos == ((CTRDGPulseVib *)ctrdg_ex_data)->rest_pos) {
                CTRDGi_VibOnOff(VIB_STOP);
                OS_SetAlarm(&pulse_edge_alarm, ((CTRDGPulseVib *)ctrdg_ex_data)->rest_tick, CTRDG_VibPulseEdgeUpdate, (CTRDGPulseVib *)ctrdg_ex_data);
                ((CTRDGPulseVib *)ctrdg_ex_data)->current_pos = 0;
            } else if (((CTRDGPulseVib *)ctrdg_ex_data)->current_pos & 1) {
                CTRDGi_VibOnOff(VIB_STOP);
                OS_SetAlarm(&pulse_edge_alarm, get_stop_tick((CTRDGPulseVib *)ctrdg_ex_data), CTRDG_VibPulseEdgeUpdate, (CTRDGPulseVib *)ctrdg_ex_data);
                ++((CTRDGPulseVib *)ctrdg_ex_data)->current_pos;
            } else {
                CTRDGi_VibOnOff(VIB_START);
                OS_SetAlarm(&pulse_edge_alarm, get_vib_tick((CTRDGPulseVib *)ctrdg_ex_data), CTRDG_VibPulseEdgeUpdate, (CTRDGPulseVib *)ctrdg_ex_data);
                ++((CTRDGPulseVib *)ctrdg_ex_data)->current_pos;
            }

            if (!is_locked_by_other) {
                OS_UnlockCartridge(lock_id);
            }
        } else {
            OS_SetAlarm(&pulse_edge_alarm, ALARM_RETRYINTERVAL, CTRDG_VibPulseEdgeUpdate, (CTRDGPulseVib *)ctrdg_ex_data);
        }
    }
}

static void CTRDGi_VibOnOff(u32 sw)
{
    current_vib = sw;
    *(vu16 *)VIB_ADDRESS = (u16)sw;
}

static inline u32 get_vib_tick(CTRDGPulseVib *ctrdg_ex_data)
{
    return ctrdg_ex_data->vib_tick[ctrdg_ex_data->current_pos >> 1];
}

static inline u32 get_stop_tick(CTRDGPulseVib *ctrdg_ex_data)
{
    return ctrdg_ex_data->stop_tick[ctrdg_ex_data->current_pos >> 1];
}
#endif

#define AGB_MASK1_OFFSET      0x9C
#define AGB_MASK2_OFFSET      0x9E
#define AGB_TITLE_NAME_OFFSET 0xA0

static BOOL CTRDGi_IsNinLogoOfAgb(u16 *logop)
{
    CTRDGHeader *chp = (CTRDGHeader *)HW_CTRDG_ROM;
    u16 *logo_orgp = (u16 *)chp->nintendoLogo;
    u16 lockID;
    BOOL retval = TRUE;

    lockID = OS_GetLockID();

    OS_LockCartridge(lockID);

    for (int i = 0; i < (AGB_TITLE_NAME_OFFSET - 4) / 2; i++) {
        u16 cmpMask = 0xFFFF;
        if (i == (AGB_MASK1_OFFSET - 4) / 2) {
            cmpMask ^= 0x84;
        } else if (i == (AGB_MASK2_OFFSET - 4) / 2) {
            cmpMask ^= 3;
        }

        if ((logop[i] & cmpMask) != logo_orgp[i]) {
            retval = FALSE;
            break;
        }
    }

    OS_UnLockCartridge(lockID);

    OS_ReleaseLockID(lockID);

    return retval;
}

#define CTRDGi_COUNT_NOT_SET   0xFFFFFFFF
#define CTRDG_POLLING_INTERVAL 10

void CTRDG_CheckPullOut_Polling(void)
{
    static u32 nextCount = CTRDGi_COUNT_NOT_SET;
    static BOOL isCartridgePullOut = FALSE;
    static BOOL isFirstCheck = TRUE;
    static BOOL skipCheck = FALSE;

#ifndef SDK_SMALL_BUILD
    if (nextCount == CTRDGi_COUNT_NOT_SET) {
        nextCount = OS_GetVBlankCount() + CTRDG_POLLING_INTERVAL;
        return;
    }

    if (skipCheck || isCartridgePullOut || OS_GetVBlankCount() < nextCount) {
        return;
    }
    nextCount = OS_GetVBlankCount() + CTRDG_POLLING_INTERVAL;

    isCartridgePullOut = CTRDG_IsPulledOut();

    if (!CTRDG_IsExisting()) {
        if (isFirstCheck) {
            skipCheck = TRUE;
            return;
        }
        isCartridgePullOut = TRUE;
    }
    isFirstCheck = FALSE;

    if (isCartridgePullOut) {
        while (PXI_SendWordByFifo(PXI_FIFO_TAG_CTRDG, CTRDG_PXI_COMMAND_PULLED_OUT, FALSE) != PXI_FIFO_SUCCESS) {
            OS_Sleep(100);
        }
    }
#endif
}

static void CTRDGi_TerminateARM7(void)
{
    CTRDG_VibPulseEdgeUpdate(NULL);
    SND_BeginSleep();
    WVR_Shutdown();
    OS_Terminate();
}

static void CTRDGi_CallbackForSetPhi(PXIFifoTag tag, u32 data, BOOL err)
{
    if ((data & CTRDG_PXI_COMMAND_MASK) == CTRDG_PXI_COMMAND_SET_PHI) {
        CTRDGPhiClock param = (data & CTRDG_PXI_COMMAND_PARAM_MASK) >> CTRDG_PXI_COMMAND_PARAM_SHIFT;

        MIi_SetPhiClock(param);

        u32 data = CTRDG_PXI_COMMAND_SET_PHI_RESULT;
        while (PXI_SendWordByFifo(PXI_FIFO_TAG_CTRDG_PHI, data, FALSE) != PXI_FIFO_SUCCESS) {
            SVC_WaitByLoop(1);
        }
    } else {
#ifndef SDK_FINALROM
        OS_Panic("illegal Cartridge pxi command.");
#else
        OS_Panic("");
#endif
    }
}
