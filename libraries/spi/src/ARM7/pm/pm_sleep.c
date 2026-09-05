#include "nitro/spi/ARM7/pm.h"
#include <nitro/exi/ARM7/genPort.h>
#include <nitro/os/common/interrupt.h>

extern u16 PMi_TriggerBL;
extern u16 PMi_KeyPattern;
extern PMWork PMi_Work;

void PMi_DoSleep(void)
{
    BOOL prepIrq;
    OSIntrMode prepIntrMode;
    OSIrqMask prepIntrMask;

    u8 pmic_reg_backup;
    u16 rcnt_reg_backup;
    BOOL rcnt_reg_restore = FALSE;

    prepIrq = OS_DisableIrq();
    prepIntrMode = OS_DisableInterrupts();
    prepIntrMask = OS_DisableIrqMask((1 << OS_IRQ_TABLE_MAX) - 1);

    pmic_reg_backup = PMi_GetRegister(REG_PMIC_CTL_ADDR);
    PM_SetLEDPattern(PM_LED_PATTERN_BLINK_LOW);
    PMi_SetLED(PM_LED_BLINK_LOW);
    PMi_SetLED(PM_LED_BLINK_LOW);

    SND_BeginSleep();
    PMi_ResetControl(PMIC_CTL_SND_PWR);

    if (PMi_TriggerBL & PM_TRIGGER_KEY) {
        reg_PAD_KEYCNT = (u16)(REG_PAD_KEYCNT_INTR_MASK | PMi_KeyPattern);
        OS_EnableIrqMask(OS_IE_KEY);
    }

    if (PMi_TriggerBL & PM_TRIGGER_COVER_OPEN) {
        OS_EnableIrqMask(OS_IE_POWERMAN);
    }

    if (PMi_TriggerBL & PM_TRIGGER_RTC_ALARM) {
        rcnt_reg_backup = EXIi_GetRcnt0L();
        rcnt_reg_restore = TRUE;

        EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

        EXIi_SetBitRcnt0L(REG_EXI_RCNT0_L_DIR_SI_MASK, 0);
        EXIi_SetBitRcnt0L(REG_EXI_RCNT0_L_I_MASK, REG_EXI_RCNT0_L_I_MASK);

        OS_EnableIrqMask(OS_IE_SIO);
    }

    if (PMi_TriggerBL & PM_TRIGGER_CARD) {
        OS_EnableIrqMask(OS_IE_CARD_IREQ);
    }

    if (PMi_TriggerBL & PM_TRIGGER_CARTRIDGE) {
        OS_EnableIrqMask(OS_IE_CARTRIDGE);
    }

    OS_RestoreInterrupts(prepIntrMode);
    OS_EnableIrq();

    SVC_Sleep();

    PMi_SetRegister(REG_PMIC_CTL_ADDR, pmic_reg_backup);

    u32 b1 = PMi_TriggerBL & (1 << PM_BACKLIGHT_RECOVER_TOP_SHIFT) ? PM_UTIL_LCD2_BACKLIGHT_ON : PM_UTIL_LCD2_BACKLIGHT_OFF;
    u32 b2 = PMi_TriggerBL & (1 << PM_BACKLIGHT_RECOVER_BOTTOM_SHIFT) ? PM_UTIL_LCD1_BACKLIGHT_ON : PM_UTIL_LCD1_BACKLIGHT_OFF;
    PMi_SwitchUtilityProc(b1);
    PMi_SwitchUtilityProc(b2);

    if (rcnt_reg_restore) {
        EXIi_SetRcnt0L(rcnt_reg_backup);
    }

    PMi_SetControl(PMIC_CTL_SND_PWR);
    SND_EndSleep();

    PMi_Work.status = PM_STATUS_READY;

    PMi_SendPxiCommand(SPI_PXI_COMMAND_PM_SLEEP_END, 0, 0);

    OS_DisableInterrupts();
    OS_SetIrqMask(prepIntrMask);
    OS_RestoreInterrupts(prepIntrMode);
    OS_RestoreIrq(prepIrq);
}
