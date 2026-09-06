#include "nitro/spi/ARM7/pm.h"

typedef struct PMiBlinkPatternData {
    u64 pattern;
    u16 patternSize;
    u16 patternResolution;
} PMiBlinkPatternData;

PMiBlinkPatternData PMi_BlinkPatternData[12] = {
    { 0xAA00000000000000, 8, 1 },
    { 0xCC00000000000000, 8, 1 },
    { 0xE380000000000000, 12, 1 },
    { 0xF0F0000000000000, 16, 1 },
    { 0xF83E000000000000, 20, 1 },
    { 0xFC00000000000000, 12, 1 },
    { 0xFF00000000000000, 16, 1 },
    { 0xFFC0000000000000, 20, 1 },
    { 0xFF00000000000000, 32, 1 },
    { 0xFF00FF0000000000, 32, 1 },
    { 0xFFFFFF0000000000, 32, 1 },
    { 0xC300000000000000, 40, 2 },
};

static u32 PMi_BlinkCounter = 0;
static PMLEDPattern PMi_BlinkPatternNo = PM_LED_PATTERN_NONE;
extern PMLEDStatus PMi_LEDStatus;

#define PMi_SKIP_PATTERN 4

void PM_SelfBlinkProc(void)
{
    PMLEDStatus nextStatus = PMi_LEDStatus;

    if (PMi_BlinkPatternNo == PM_LED_PATTERN_NONE) {
        if (SPIi_SetEntry(SPI_DEVICE_TYPE_PM, SPI_PXI_COMMAND_PM_SELF_BLINK, 1, PM_LED_PATTERN_ON)) {
            PM_SetLEDPattern(PM_LED_PATTERN_ON);
        }
        return;
    } else if (PMi_BlinkPatternNo < PMi_SKIP_PATTERN) {
        if (PMi_BlinkPatternNo != PMi_LEDStatus) {
            SPIi_SetEntry(SPI_DEVICE_TYPE_PM, SPI_PXI_COMMAND_PM_SELF_BLINK, 1, PMi_BlinkPatternNo);
        }
        return;
    }

    PMiBlinkPatternData *p = &PMi_BlinkPatternData[PMi_BlinkPatternNo - PMi_SKIP_PATTERN];
    if (p->pattern & (0x8000000000000000 >> (PMi_BlinkCounter / p->patternResolution))) {
        nextStatus = PM_LED_ON;
    } else {
        nextStatus = PM_LED_BLINK_LOW;
    }

    PMi_BlinkCounter++;
    if (PMi_BlinkCounter >= p->patternSize * p->patternResolution) {
        PMi_BlinkCounter = 0;
    }

    if (nextStatus != PMi_LEDStatus) {
        SPIi_SetEntry(SPI_DEVICE_TYPE_PM, SPI_PXI_COMMAND_PM_SELF_BLINK, 1, nextStatus);
    }
}

void PM_SetLEDPattern(PMLEDPattern pattern)
{
    if (pattern > PM_LED_PATTERN_MAX) {
        return;
    }

    PMi_BlinkPatternNo = pattern;
    PMi_BlinkCounter = 0;
}

PMLEDPattern PM_GetLEDPattern(void)
{
    return PMi_BlinkPatternNo;
}
