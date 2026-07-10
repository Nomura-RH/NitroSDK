#include <nitro/exi/ARM7/genPort.h>
#include <nitro/hw/ARM7/ioreg_EXI.h>
#include <nitro/hw/common/mmap_shared.h>
#include <nitro/os/common/alarm.h>
#include <nitro/os/common/tick.h>

BOOL PADi_XYButtonAvailable;
static OSAlarm sAlarm;

static void XYButtonAlarmHandler(void *);

BOOL PAD_InitXYButton(void) {
    OSTick tick;

    if (!OS_IsTickAvailable() || !OS_IsAlarmAvailable()) {
        return FALSE;
    } else if (PADi_XYButtonAvailable) {
        return FALSE;
    } else {
        OS_CreateAlarm(&sAlarm);
        tick = OS_GetTick();
        OS_SetPeriodicAlarm(&sAlarm, tick + 0x82e, 0x82e, (OSAlarmHandler)XYButtonAlarmHandler, NULL);
        PADi_XYButtonAvailable = TRUE;
        return TRUE;
    }
}

static void XYButtonAlarmHandler(void *dummy)
{
    u16 r4 = 0, r1;
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);
    r1 = EXIi_GetRcnt0H();
    if (r1 & REG_EXI_RCNT0_H_DATA_R7_MASK) {
        r4 = 0x8000;
    }
    *(vu16 *)HW_BUTTON_XY_BUF = (u16)(r4 | ((r1 & REG_EXI_RCNT0_H_FIELD(0, 0, 0, 0, 1, 0, 1, 1)) << 10));
}
