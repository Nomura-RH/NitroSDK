#include "nitro/pad/ARM7/xyButton.h"
#include <nitro/exi/ARM7/genPort.h>
#include <nitro/hw/ARM7/ioreg_EXI.h>
#include <nitro/hw/common/mmap_shared.h>
#include <nitro/os/common/alarm.h>
#include <nitro/os/common/tick.h>

#define PAD_XYBUTTON_PERIOD OS_MicroSecondsToTicks(4000)

static OSAlarm PADi_XYButtonAlarm;

BOOL PADi_XYButtonAvailable = FALSE;

static void PADi_XYButton_Callback(void *arg);

BOOL PAD_InitXYButton(void)
{
    if (!(OS_IsTickAvailable() && OS_IsAlarmAvailable())) {
        SDK_WARNING(TRUE, "PAD Service are not starting...\n");
        return FALSE;
    }

    if (PADi_XYButtonAvailable) {
        return FALSE;
    }

#if defined(SDK_TEG)
    OSIntrMode enabled = OS_DisableInterrupts();
    EXIi_SetBitRcnt0H(REG_EXI_RCNT0_H_DIR_R0_MASK | REG_EXI_RCNT0_H_DIR_R1_MASK | REG_EXI_RCNT0_H_DIR_R2_MASK | REG_EXI_RCNT0_H_DIR_R3_MASK, 0);
    OS_RestoreInterrupts(enabled);
#endif

    OS_CreateAlarm(&PADi_XYButtonAlarm);
    OS_SetPeriodicAlarm(&PADi_XYButtonAlarm, OS_GetTick() + PAD_XYBUTTON_PERIOD, PAD_XYBUTTON_PERIOD, &PADi_XYButton_Callback, 0);

    PADi_XYButtonAvailable = TRUE;
    return TRUE;
}

static void PADi_XYButton_Callback(void *arg)
{
    u16 fold = 0;

    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);
    u16 r = EXIi_GetRcnt0H();

    if (r & EXI_GPIO_PADFOLD) {
        fold = PAD_DETECT_FOLD_MASK;
    }

    *(vu16 *)HW_BUTTON_XY_BUF = ((r & (EXI_GPIO_PADX | EXI_GPIO_PADY | EXI_GPIO_PADDEBUG)) << 10) | fold;
}
