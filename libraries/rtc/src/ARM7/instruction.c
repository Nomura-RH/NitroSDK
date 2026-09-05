#include "nitro/rtc/ARM7/instruction.h"

#include "nitro/rtc/ARM7/gpio.h"
#include <nitro/exi/ARM7/genPort.h>

static void RtcChangeAlarmFormat24to12(RTCRawAlarm *alarm);
static void RtcChangeAlarmFormat12to24(RTCRawAlarm *alarm);
static void RtcGpioTransfer(u16 inst, u16 param, void *buf, u32 size);

void RTC_Reset(void)
{
    RTCRawStatus1 stat;

    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    stat.reset = 1;
    RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_STAT1, &stat, 1);
}

void RTC_SetHourFormat(u16 format)
{
    RTCRawStatus1 stat1;
    RTCRawAlarm alarm;

    format %= 2;
    if (format != 1) {
        return;
    }

    RTC_ReadStatus1(&stat1);

    if (stat1.format != format) {
        stat1.format = format;
        RTC_WriteStatus1(&stat1);

        RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_ALARM1, &alarm, 3);

        if (format == 0) {
            RtcChangeAlarmFormat24to12(&alarm);
        } else {
            RtcChangeAlarmFormat12to24(&alarm);
        }

        RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_ALARM1, &alarm, 3);

        RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_ALARM2, &alarm, 3);

        if (format == 0) {
            RtcChangeAlarmFormat24to12(&alarm);
        } else {
            RtcChangeAlarmFormat12to24(&alarm);
        }

        RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_ALARM2, &alarm, 3);
    }
}

void RTC_ReadDateTime(RTCRawData *data)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_FULL, data, 7);
}

void RTC_WriteDateTime(const RTCRawData *data)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_FULL, data, 7);
}

void RTC_ReadDate(RTCRawDate *date)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_FULL, date, 4);
}

void RTC_ReadTime(RTCRawTime *time)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_TIME, time, 3);
}

void RTC_WriteTime(const RTCRawTime *time)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_TIME, time, 3);
}

BOOL RTC_ReadPulse(RTCRawPulse *pulse)
{
    RTCRawStatus2 stat2;
    RTC_ReadStatus2(&stat2);

    if ((stat2.intr_mode & RTC_INTERRUPT_MASK_PULSE) == RTC_INTERRUPT_MODE_PULSE) {
        RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_PULSE, pulse, 1);
        return TRUE;
    }
    return FALSE;
}

BOOL RTC_WritePulse(const RTCRawPulse *pulse)
{
    RTCRawStatus2 stat2;
    RTC_ReadStatus2(&stat2);

    if ((stat2.intr_mode & RTC_INTERRUPT_MASK_PULSE) == RTC_INTERRUPT_MODE_PULSE) {
        RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_PULSE, pulse, 1);
        return TRUE;
    }
    return FALSE;
}

BOOL RTC_ReadAlarm1(RTCRawAlarm *alarm)
{
    RTCRawStatus2 stat2;
    RTC_ReadStatus2(&stat2);

    if (stat2.intr_mode == RTC_INTERRUPT_MODE_ALARM) {
        RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_ALARM1, alarm, 3);
        return TRUE;
    }
    return FALSE;
}

BOOL RTC_WriteAlarm1(const RTCRawAlarm *alarm)
{
    RTCRawStatus2 stat2;
    RTC_ReadStatus2(&stat2);

    if (stat2.intr_mode == RTC_INTERRUPT_MODE_ALARM) {
        RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_ALARM1, alarm, 3);
        return TRUE;
    }
    return FALSE;
}

BOOL RTC_ReadAlarm2(RTCRawAlarm *alarm)
{
    RTCRawStatus2 stat2;
    RTC_ReadStatus2(&stat2);

    if (stat2.intr2_mode) {
        RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_ALARM2, alarm, 3);
        return TRUE;
    }
    return FALSE;
}

BOOL RTC_WriteAlarm2(const RTCRawAlarm *alarm)
{
    RTCRawStatus2 stat2;
    RTC_ReadStatus2(&stat2);

    if (stat2.intr2_mode) {
        RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_ALARM2, alarm, 3);
        return TRUE;
    }
    return FALSE;
}

void RTC_ReadStatus1(RTCRawStatus1 *stat)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_STAT1, stat, 1);
}

void RTC_WriteStatus1(const RTCRawStatus1 *stat)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_STAT1, stat, 1);
}

void RTC_ReadStatus2(RTCRawStatus2 *stat)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_STAT2, stat, 1);
}

void RTC_WriteStatus2(const RTCRawStatus2 *stat)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_STAT2, stat, 1);
}

void RTC_ReadAdjust(RTCRawAdjust *adjust)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_ADJUST, adjust, 1);
}

void RTC_WriteAdjust(const RTCRawAdjust *adjust)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_ADJUST, adjust, 1);
}

void RTC_ReadFree(RTCRawFree *free)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_READ, RTC_INSTRUCTION_FREE, free, 1);
}

void RTC_WriteFree(const RTCRawFree *free)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);

    RtcGpioTransfer(RTC_INSTRUCTION_WRITE, RTC_INSTRUCTION_FREE, free, 1);
}

static void RtcChangeAlarmFormat24to12(RTCRawAlarm *alarm)
{
    switch (alarm->hour) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x10:
    case 0x11:
        alarm->afternoon = 0;
        break;
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x22:
    case 0x23:
        alarm->afternoon = 1;
        alarm->hour -= 0x12;
        break;
    case 0x20:
    case 0x21:
        alarm->afternoon = 1;
        alarm->hour -= 0x18;
        break;
    default:
        alarm->afternoon = 0;
        alarm->hour = 0x00;
    }
}

static void RtcChangeAlarmFormat12to24(RTCRawAlarm *alarm)
{
    switch (alarm->hour) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x10:
    case 0x11:
        if (alarm->afternoon) {
            alarm->hour += 0x12;
        }
        break;
    case 0x08:
    case 0x09:
        if (alarm->afternoon) {
            alarm->hour += 0x18;
        }
        break;
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
        alarm->afternoon = 1;
        break;
    default:
        alarm->afternoon = 0;
        alarm->hour = 0x00;
    }
}

static void RtcGpioTransfer(u16 inst, u16 param, void *buf, u32 size)
{
    RTCi_GpioStart();
    RTCi_GpioSendCommand(inst, param);
    switch (inst) {
    case RTC_INSTRUCTION_READ:
        RTCi_GpioReceiveData(buf, size);
        break;
    case RTC_INSTRUCTION_WRITE:
        RTCi_GpioSendData(buf, size);
        break;
    }
    RTCi_GpioEnd();
}
