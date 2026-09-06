#include "nitro/spi/ARM7/pm.h"

PMLEDStatus PMi_LEDStatus = PM_LED_ON;

void PMi_SwitchUtilityProc(u32 procNumber)
{
    switch (procNumber) {
    case PM_UTIL_LED_ON:
        PM_SetLEDPattern(PM_LED_PATTERN_ON);
        PMi_SetLED(PM_LED_ON);
        break;
    case PM_UTIL_LED_BLINK_HIGH_SPEED:
        PM_SetLEDPattern(PM_LED_PATTERN_BLINK_HIGH);
        PMi_SetLED(PM_LED_BLINK_HIGH);
        break;
    case PM_UTIL_LED_BLINK_LOW_SPEED:
        PM_SetLEDPattern(PM_LED_PATTERN_BLINK_LOW);
        PMi_SetLED(PM_LED_BLINK_LOW);
        break;

    case PM_UTIL_LCD1_BACKLIGHT_ON:
        PMi_SetControl(PMIC_CTL_BKLT1);
        break;
    case PM_UTIL_LCD1_BACKLIGHT_OFF:
        PMi_ResetControl(PMIC_CTL_BKLT1);
        break;
    case PM_UTIL_LCD2_BACKLIGHT_ON:
        PMi_SetControl(PMIC_CTL_BKLT2);
        break;
    case PM_UTIL_LCD2_BACKLIGHT_OFF:
        PMi_ResetControl(PMIC_CTL_BKLT2);
        break;
    case PM_UTIL_LCD12_BACKLIGHT_ON:
        PMi_SetControl(PMIC_CTL_BKLT1 | PMIC_CTL_BKLT2);
        break;
    case PM_UTIL_LCD12_BACKLIGHT_OFF:
        PMi_ResetControl(PMIC_CTL_BKLT1 | PMIC_CTL_BKLT2);
        break;

    case PM_UTIL_SOUND_POWER_ON:
        PMi_SetControl(PMIC_CTL_SND_PWR);
        break;
    case PM_UTIL_SOUND_POWER_OFF:
        PMi_ResetControl(PMIC_CTL_SND_PWR);
        break;
    case PM_UTIL_SOUND_VOL_CTRL_ON:
        PMi_ResetControl(PMIC_CTL_SND_VOLCTRL);
        break;
    case PM_UTIL_SOUND_VOL_CTRL_OFF:
        PMi_SetControl(PMIC_CTL_SND_VOLCTRL);
        break;

    case PM_UTIL_FORCE_POWER_ON:
        PMi_ResetControl(PMIC_CTL_PWR_OFF);
        break;
    case PM_UTIL_FORCE_POWER_OFF:
        SND_BeginSleep();
        PMi_SetControl(PMIC_CTL_PWR_OFF);
        break;
    }
}

void PMi_SetLED(PMLEDStatus status)
{
    switch (status) {
    case PM_LED_ON:
        PMi_ResetControl(PMIC_CTL_LED_SW);
        break;
    case PM_LED_BLINK_HIGH:
        PMi_SetControl(PMIC_CTL_LED_SP | PMIC_CTL_LED_SW);
        break;
    case PM_LED_BLINK_LOW:
        PMi_ResetControl(PMIC_CTL_LED_SP);
        PMi_SetControl(PMIC_CTL_LED_SW);
        break;
    default:
        OS_Panic("Bad LED status");
        break;
    }

    PMi_LEDStatus = status;
}
