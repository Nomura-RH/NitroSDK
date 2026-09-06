#include "nitro/rtc/ARM7/control.h"

#include <nitro/exi/ARM7/genPort.h>
#include <nitro/os/common/alarm.h>
#include <nitro/os/common/interrupt.h>
#include <nitro/os/common/message.h>
#include <nitro/os/common/systemWork.h>
#include <nitro/os/common/thread.h>
#include <nitro/pxi/common/fifo.h>
#include <nitro/rtc/ARM7/instruction.h>
#include <nitro/rtc/common/fifo.h>
#include <nitro/rtc/common/type.h>

#define RTC_ALARM_CATCH_BY_INTR

static u16 rtcInitialized;
static RTCWork rtcWork;

static void RtcPxiCallback(PXIFifoTag tag, u32 data, BOOL err);
static void RtcReturnResult(u16 command, RTCPxiResult result);
static void RtcThread(void *arg);
#ifdef RTC_ALARM_CATCH_BY_INTR
static void RtcAlarmIntr(void);
#else
static void RtcPollingThread(void *arg);
static void RtcPollingAlarm(void *arg);
#endif
static void RtcInitialize(void);
static u32 RtcGetDayOfWeek(u32 year, u32 month, u32 day);
static u32 RtcBCD2HEX(u32 bcd);

void RTC_Init(u32 priority)
{
    if (rtcInitialized) {
        return;
    }
    rtcInitialized = TRUE;

    rtcWork.busy = TRUE;
    RtcInitialize();
    rtcWork.busy = FALSE;

    PXI_Init();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_RTC, RtcPxiCallback);

    OS_InitMessageQueue(&rtcWork.msgQueue, rtcWork.msgArray, RTC_MESSAGE_ARRAY_MAX);
    OS_CreateThread(&rtcWork.thread, RtcThread, NULL, (void *)(rtcWork.stack + RTC_THREAD_STACK_SIZE / sizeof(u64)), RTC_THREAD_STACK_SIZE, priority);
    OS_WakeupThreadDirect(&rtcWork.thread);
#ifdef RTC_ALARM_CATCH_BY_INTR
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);
    EXIi_SetBitRcnt0L(REG_EXI_RCNT0_L_DIR_SI_MASK, 0);
    EXIi_SetBitRcnt0L(REG_EXI_RCNT0_L_I_MASK, REG_EXI_RCNT0_L_I_MASK);

    OSIntrMode enabled = OS_DisableInterrupts();
    OS_SetIrqFunction(OS_IE_SIO, RtcAlarmIntr);
    OS_EnableIrqMask(OS_IE_SIO);
    OS_RestoreInterrupts(enabled);
#else
    OS_InitThreadQueue(&rtcWork.pollingQueue);
    OS_CreateThread(&rtcWork.polling, RtcPollingThread, 0, (void *)(rtcWork.pollingStack + (RTC_POLLING_STACK_SIZE / sizeof(u64))), RTC_POLLING_STACK_SIZE, priority - 1);
    OS_WakeupThreadDirect(&rtcWork.polling);

    if (!OS_IsAlarmAvailable()) {
        OS_InitAlarm();
    }

    OS_CreateAlarm(&rtcWork.pollingAlarm);
    OS_SetPeriodicAlarm(&rtcWork.pollingAlarm, OS_GetTick(), RTC_POLLING_SPAN_TICK, RtcPollingAlarm, NULL);
#endif
}

static void RtcPxiCallback(PXIFifoTag tag, u32 data, BOOL err)
{
    if (err) {
        return;
    }

    u16 command = (data & RTC_PXI_COMMAND_MASK) >> RTC_PXI_COMMAND_SHIFT;
    switch (command) {
    case RTC_PXI_COMMAND_RESET:
    case RTC_PXI_COMMAND_SET_HOUR_FORMAT:
    case RTC_PXI_COMMAND_READ_DATETIME:
    case RTC_PXI_COMMAND_READ_DATE:
    case RTC_PXI_COMMAND_READ_TIME:
    case RTC_PXI_COMMAND_READ_PULSE:
    case RTC_PXI_COMMAND_READ_ALARM1:
    case RTC_PXI_COMMAND_READ_ALARM2:
    case RTC_PXI_COMMAND_READ_STATUS1:
    case RTC_PXI_COMMAND_READ_STATUS2:
    case RTC_PXI_COMMAND_READ_ADJUST:
    case RTC_PXI_COMMAND_READ_FREE:
#ifndef SDK_FINALROM
    case RTC_PXI_COMMAND_WRITE_DATETIME:
    case RTC_PXI_COMMAND_WRITE_DATE:
    case RTC_PXI_COMMAND_WRITE_TIME:
#endif
    case RTC_PXI_COMMAND_WRITE_PULSE:
    case RTC_PXI_COMMAND_WRITE_ALARM1:
    case RTC_PXI_COMMAND_WRITE_ALARM2:
    case RTC_PXI_COMMAND_WRITE_STATUS1:
    case RTC_PXI_COMMAND_WRITE_STATUS2:
    case RTC_PXI_COMMAND_WRITE_ADJUST:
    case RTC_PXI_COMMAND_WRITE_FREE:
        if (rtcWork.busy) {
            RtcReturnResult(command, RTC_PXI_RESULT_BUSY);
        } else {
            rtcWork.busy = TRUE;
            rtcWork.command = command;
            if (!OS_SendMessage(&rtcWork.msgQueue, NULL, OS_MESSAGE_NOBLOCK)) {
                RtcReturnResult(command, RTC_PXI_RESULT_FATAL_ERROR);
            }
        }
        break;
    default:
        RtcReturnResult(command, RTC_PXI_RESULT_INVALID_COMMAND);
    }
}

static void RtcReturnResult(u16 command, RTCPxiResult result)
{
    while (0 > PXI_SendWordByFifo(
               PXI_FIFO_TAG_RTC,
               RTC_PXI_RESULT_BIT_MASK
                   | ((command << RTC_PXI_COMMAND_SHIFT) & RTC_PXI_COMMAND_MASK)
                   | ((result << RTC_PXI_RESULT_SHIFT) & RTC_PXI_RESULT_MASK),
               0)) {
    }
}

static void RtcThread(void *arg)
{
    OSMessage msg;
    RTCRawData *prd = (RTCRawData *)(OS_GetSystemWork()->real_time_clock);

    while (TRUE) {
        OS_ReceiveMessage(&rtcWork.msgQueue, &msg, OS_MESSAGE_BLOCK);

        switch (rtcWork.command) {
        case RTC_PXI_COMMAND_RESET:
            RTC_Reset();
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_RESET, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_SET_HOUR_FORMAT:
            RTC_SetHourFormat(prd->a.status1.format);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_SET_HOUR_FORMAT, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_READ_DATETIME:
            RTC_ReadDateTime(prd);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_READ_DATETIME, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_READ_DATE:
            RTC_ReadDate(&prd->t.date);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_READ_DATE, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_READ_TIME:
            RTC_ReadTime(&prd->t.time);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_READ_TIME, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_READ_PULSE:
            if (!RTC_ReadPulse(&prd->a.pulse)) {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_READ_PULSE, RTC_PXI_RESULT_ILLEGAL_STATUS);
            } else {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_READ_PULSE, RTC_PXI_RESULT_SUCCESS);
            }
            break;

        case RTC_PXI_COMMAND_READ_ALARM1:
            if (!RTC_ReadAlarm1(&prd->a.alarm)) {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_READ_ALARM1, RTC_PXI_RESULT_ILLEGAL_STATUS);
            } else {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_READ_ALARM1, RTC_PXI_RESULT_SUCCESS);
            }
            break;

        case RTC_PXI_COMMAND_READ_ALARM2:
            if (!RTC_ReadAlarm2(&prd->a.alarm)) {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_READ_ALARM2, RTC_PXI_RESULT_ILLEGAL_STATUS);
            } else {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_READ_ALARM2, RTC_PXI_RESULT_SUCCESS);
            }
            break;

        case RTC_PXI_COMMAND_READ_STATUS1:
            RTC_ReadStatus1(&prd->a.status1);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_READ_STATUS1, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_READ_STATUS2:
            RTC_ReadStatus2(&prd->a.status2);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_READ_STATUS2, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_READ_ADJUST:
            RTC_ReadAdjust(&prd->a.adjust);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_READ_ADJUST, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_READ_FREE:
            RTC_ReadFree(&prd->a.free);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_READ_FREE, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_WRITE_DATETIME:
            RTC_WriteDateTime(prd);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_WRITE_DATETIME, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_WRITE_DATE:
            RTC_ReadTime(&prd->t.time);
            RTC_WriteDateTime(prd);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_WRITE_DATE, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_WRITE_TIME:
            RTC_WriteTime(&prd->t.time);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_WRITE_TIME, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_WRITE_PULSE:
            if (!RTC_WritePulse(&prd->a.pulse)) {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_WRITE_PULSE, RTC_PXI_RESULT_ILLEGAL_STATUS);
            } else {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_WRITE_PULSE, RTC_PXI_RESULT_SUCCESS);
            }
            break;

        case RTC_PXI_COMMAND_WRITE_ALARM1:
            if (!RTC_WriteAlarm1(&prd->a.alarm)) {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_WRITE_ALARM1, RTC_PXI_RESULT_ILLEGAL_STATUS);
            } else {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_WRITE_ALARM1, RTC_PXI_RESULT_SUCCESS);
            }
            break;

        case RTC_PXI_COMMAND_WRITE_ALARM2:
            if (!RTC_WriteAlarm2(&prd->a.alarm)) {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_WRITE_ALARM2, RTC_PXI_RESULT_ILLEGAL_STATUS);
            } else {
                rtcWork.busy = FALSE;
                RtcReturnResult(RTC_PXI_COMMAND_WRITE_ALARM2, RTC_PXI_RESULT_SUCCESS);
            }
            break;

        case RTC_PXI_COMMAND_WRITE_STATUS1:
            RTC_WriteStatus1(&prd->a.status1);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_WRITE_STATUS1, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_WRITE_STATUS2:
            RTC_WriteStatus2(&prd->a.status2);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_WRITE_STATUS2, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_WRITE_ADJUST:
            RTC_WriteAdjust(&prd->a.adjust);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_WRITE_ADJUST, RTC_PXI_RESULT_SUCCESS);
            break;

        case RTC_PXI_COMMAND_WRITE_FREE:
            RTC_WriteFree(&prd->a.free);
            rtcWork.busy = FALSE;
            RtcReturnResult(RTC_PXI_COMMAND_WRITE_FREE, RTC_PXI_RESULT_SUCCESS);
            break;

        default: {
            u16 comm = rtcWork.command;
            rtcWork.busy = FALSE;
            RtcReturnResult(comm, RTC_PXI_RESULT_INVALID_COMMAND);
        } break;
        }
    }
}

#ifdef RTC_ALARM_CATCH_BY_INTR
static void RtcAlarmIntr(void)
{
    RTCRawStatus1 stat1;
    RTCRawStatus2 stat2;

    RTC_ReadStatus1(&stat1);
    if (stat1.intr1 || stat1.intr2) {
        RTC_ReadStatus2(&stat2);
        u32 intr_num = 0;

        if (stat1.intr1) {
            intr_num |= 1;
            stat2.intr_mode = RTC_INTERRUPT_MODE_NONE;
        }
        if (stat1.intr2) {
            intr_num |= 2;
            stat2.intr2_mode = 0;
        }

        RTC_WriteStatus2(&stat2);

        RtcReturnResult(RTC_PXI_COMMAND_INTERRUPT, intr_num);
    }
}
#else
static void RtcPollingThread(void *arg)
{
    RTCRawStatus1 stat1, old;
    RTCRawStatus2 stat2;
    u32 intr_num;

    old.intr1 = 0;
    old.intr2 = 0;
    while (TRUE) {
        OS_SleepThread(&rtcWork.pollingQueue);

        if (!rtcWork.busy) {
            RTC_ReadStatus1(&stat1);
            if (stat1.intr1 - old.intr1 == 1 || stat1.intr2 - old.intr2 == 1) {
                RTC_ReadStatus2(&stat2);
                intr_num = 0;

                if (stat1.intr1) {
                    intr_num |= 1;
                    stat2.intr_mode = RTC_INTERRUPT_MODE_NONE;
                }
                if (stat1.intr2) {
                    intr_num |= 2;
                    stat2.intr2_mode = 0;
                }

                RTC_WriteStatus2(&stat2);

                RtcReturnResult(RTC_PXI_COMMAND_INTERRUPT, intr_num);
            }

            old = stat1;
        }
    }
}

static void RtcPollingAlarm(void *arg)
{
    OS_WakeupThread(&rtcWork.pollingQueue);
}
#endif

static void RtcInitialize(void)
{
    RTCRawStatus1 stat1;
    RTCRawStatus2 stat2;

    RTC_ReadStatus1(&stat1);
    RTC_ReadStatus2(&stat2);
    if (stat1.poc || stat1.bld || stat2.test) {
        stat1.reset = 1;
        RTC_WriteStatus1(&stat1);
    }

    if (stat1.intr1 || stat1.intr2) {
        stat2.intr_mode = RTC_INTERRUPT_MODE_NONE;
        stat2.intr2_mode = 0;
        RTC_WriteStatus2(&stat2);
    }

    RTCRawData *pData = (RTCRawData *)(OS_GetSystemWork()->real_time_clock);

    RTC_ReadDateTime(pData);
    u32 week = RtcGetDayOfWeek(2000 + RtcBCD2HEX(pData->t.date.year), RtcBCD2HEX(pData->t.date.month), RtcBCD2HEX(pData->t.date.day));
    if (pData->t.date.week != week) {
        pData->t.date.week = week;
        RTC_WriteDateTime(pData);
    }

    RTC_SetHourFormat(1);
}

static u32 RtcGetDayOfWeek(u32 year, u32 month, u32 day)
{
    if (month == 1 || month == 2) {
        year--;
        month += 12;
    }

    return (year + year / 4 - year / 100 + year / 400 + (13 * month + 8) / 5 + day) % 7;
}

static u32 RtcBCD2HEX(u32 bcd)
{
    u32 hex = 0;
    s32 i;
    s32 w;

    for (i = 0; i < 8; i++) {
        if (((bcd >> (i * 4)) & 0xF) >= 0xA) {
            return hex;
        }
    }

    for (i = 0, w = 1; i < 8; i++, w *= 10) {
        hex += ((bcd >> (i * 4)) & 0xF) * w;
    }

    return hex;
}
