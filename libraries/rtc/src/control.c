
#include <nitro/exi/ARM7/genPort.h>
#include <nitro/os/common/interrupt.h>
#include <nitro/os/common/systemWork.h>
#include <nitro/rtc/ARM7/instruction.h>
#include <nitro/rtc/common/type.h>
#include <nitro/os/common/message.h>
#include <nitro/os/common/thread.h>
#include <nitro/pxi/common/fifo.h>

#define RTC_MESSAGE_QUEUE_LENGTH 4
#define RTC_THREAD_STACK_SIZE 0x100

static u16 rtcInitialized;
static struct {
    OSMessageQueue unk_0;
    OSMessage unk_20[RTC_MESSAGE_QUEUE_LENGTH];
    OSThread unk_30;
    // thread stack
    u8 unk_d4[RTC_THREAD_STACK_SIZE];
    BOOL unk_1d4;
    u16 unk_1d8;
    u16 unk_1da;
    OSThread unk_1dc;
    OSThreadQueue unk_280;
    u8 unk_288[256];
    OSAlarm unk_388;
} rtcWork;

static void RtcPxiCallback(PXIFifoTag tag, u32 data, BOOL err);
static void RtcReturnResult(u32 param0, u32 param1);
static void RtcThread(void *arg);
static void RtcAlarmIntr(void);
static void RtcInitialize(void);
static u32 RtcGetDayOfWeek(u32 year, u32 month, u32 day);
static int RtcBCD2HEX(u32 param0);

// not actual signature
extern int _u32_div_f(void);

void RTC_Init(u32 param1)
{
    if (rtcInitialized) {
        return;
    }
    rtcInitialized = 1;

    rtcWork.unk_1d4 = TRUE;
    RtcInitialize();
    rtcWork.unk_1d4 = FALSE;

    PXI_Init();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_RTC, RtcPxiCallback);

    OS_InitMessageQueue(&(rtcWork.unk_0), rtcWork.unk_20, RTC_MESSAGE_QUEUE_LENGTH);
    OS_CreateThread(&(rtcWork.unk_30), RtcThread, NULL, (void *)(rtcWork.unk_d4 + RTC_THREAD_STACK_SIZE), RTC_THREAD_STACK_SIZE, param1);
    OS_WakeupThreadDirect(&(rtcWork.unk_30));

    {
        OSIntrMode enabled;

        EXIi_SelectRcnt(EXI_GPIOIF_GPIO);
        EXIi_SetBitRcnt0L(REG_EXI_RCNT0_L_DIR_SI_MASK, 0);
        EXIi_SetBitRcnt0L(REG_EXI_RCNT0_L_I_MASK, REG_EXI_RCNT0_L_I_MASK);

        enabled = OS_DisableInterrupts();
        (void)OS_SetIrqFunction(OS_IE_SIO, RtcAlarmIntr);
        (void)OS_EnableIrqMask(OS_IE_SIO);
        (void)OS_RestoreInterrupts(enabled);
    }
}

static asm void RtcPxiCallback(PXIFifoTag tag, u32 data, BOOL err)
{
    stmdb sp!, {r4, lr}
    cmp r2, #0
    bne _038048B0
    and r0, r1, #0x7f00
    mov r0, r0, lsl #8
    mov r4, r0, lsr #0x10
    cmp r4, #0x29

    addls pc, pc, r4, lsl #2
    b _038048A4
    b _0380484C
    b _0380484C
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _038048A4
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C
    b _0380484C

_0380484C:
    ldr r0, =rtcWork - 4
    ldr r1, [r0, #0x1d8]
    cmp r1, #0
    beq _0380486C
    mov r0, r4
    mov r1, #3
    bl RtcReturnResult
    b _038048B0

_0380486C:
    ldr r3, =rtcWork + 0xfc
    mov r1, #1
    str r1, [r0, #0x1d8]
    mov r1, #0
    ldr r0, =rtcWork.unk_0
    mov r2, r1
    strh r4, [r3, #0xdc]
    bl OS_SendMessage
    cmp r0, #0
    bne _038048B0
    mov r0, r4
    mov r1, #4
    bl RtcReturnResult
    b _038048B0

_038048A4:
    mov r0, r4
    mov r1, #1
    bl RtcReturnResult

_038048B0:
    ldmia sp!, {r4, lr}
    bx lr
}

static asm void RtcReturnResult(u32 param0, u32 param1)
{
    stmdb sp!, {r4, r5, r6, lr}
    mov r0, r0, lsl #8
    and r0, r0, #0x7f00
    orr r2, r0, #0x8000
    and r0, r1, #0xff
    orr r6, r2, r0
    mov r5, #5
    mov r4, #0

_038048E4:
    mov r0, r5
    mov r1, r6
    mov r2, r4
    bl PXI_SendWordByFifo
    cmp r0, #0
    blt _038048E4
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

static asm void RtcThread(void *arg)
{
    stmdb sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11, lr}
    ldr r9, =0x027ffde8
    mov r8, #0
    ldr r4, =rtcWork - 4
    mov r7, r8
    mov r6, r8
    mov r5, r8
    mov r10, r8
    mov r11, r8

_03804928:
    ldr r0, =rtcWork.unk_0
    add r1, sp, #0
    mov r2, #1
    bl OS_ReceiveMessage
    ldr r0, =rtcWork + 0xfc
    ldrh r0, [r0, #0xdc]
    cmp r0, #0x29

    addls pc, pc, r0, lsl #2
    b _03804D64
    b _038049F4
    b _03804A0C
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804A34
    b _03804A54
    b _03804A74
    b _03804A94
    b _03804AD0
    b _03804B0C
    b _03804B48
    b _03804B68
    b _03804B88
    b _03804BA8
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804D64
    b _03804BC8
    b _03804BE8
    b _03804C10
    b _03804C30
    b _03804C6C
    b _03804CA8
    b _03804CE4
    b _03804D04
    b _03804D24
    b _03804D44

_038049F4:
    bl RTC_Reset
    mov r0, #0
    mov r1, r0
    str r0, [r4, #0x1d8]
    bl RtcReturnResult
    b _03804928

_03804A0C:
    ldrh r0, [r9]
    mov r0, r0, lsl #0x1e
    mov r0, r0, lsr #0x1f
    bl RTC_SetHourFormat
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #1
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804A34:
    mov r0, r9
    bl RTC_ReadDateTime
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x10
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804A54:
    mov r0, r9
    bl RTC_ReadDate
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x11
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804A74:
    add r0, r9, #4
    bl RTC_ReadTime
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x12
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804A94:
    add r0, r9, #4
    bl RTC_ReadPulse
    cmp r0, #0
    bne _03804AB8
    mov r0, #0x13
    mov r1, #2
    str r8, [r4, #0x1d8]
    bl RtcReturnResult
    b _03804928

_03804AB8:
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x13
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804AD0:
    add r0, r9, #4
    bl RTC_ReadAlarm1
    cmp r0, #0
    bne _03804AF4
    mov r0, #0x14
    mov r1, #2
    str r7, [r4, #0x1d8]
    bl RtcReturnResult
    b _03804928

_03804AF4:
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x14
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804B0C:
    add r0, r9, #4
    bl RTC_ReadAlarm2
    cmp r0, #0
    bne _03804B30
    mov r0, #0x15
    mov r1, #2
    str r6, [r4, #0x1d8]
    bl RtcReturnResult
    b _03804928

_03804B30:
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x15
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804B48:
    mov r0, r9
    bl RTC_ReadStatus1
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x16
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804B68:
    add r0, r9, #2
    bl RTC_ReadStatus2
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x17
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804B88:
    add r0, r9, #4
    bl RTC_ReadAdjust
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x18
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804BA8:
    add r0, r9, #4
    bl RTC_ReadFree
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x19
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804BC8:
    mov r0, r9
    bl RTC_WriteDateTime
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x20
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804BE8:
    add r0, r9, #4
    bl RTC_ReadTime
    mov r0, r9
    bl RTC_WriteDateTime
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x21
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804C10:
    add r0, r9, #4
    bl RTC_WriteTime
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x22
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804C30:
    add r0, r9, #4
    bl RTC_WritePulse
    cmp r0, #0
    bne _03804C54
    mov r0, #0x23
    mov r1, #2
    str r5, [r4, #0x1d8]
    bl RtcReturnResult
    b _03804928

_03804C54:
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x23
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804C6C:
    add r0, r9, #4
    bl RTC_WriteAlarm1
    cmp r0, #0
    bne _03804C90
    mov r0, #0x24
    mov r1, #2
    str r10, [r4, #0x1d8]
    bl RtcReturnResult
    b _03804928

_03804C90:
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x24
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804CA8:
    add r0, r9, #4
    bl RTC_WriteAlarm2
    cmp r0, #0
    bne _03804CCC
    mov r0, #0x25
    mov r1, #2
    str r11, [r4, #0x1d8]
    bl RtcReturnResult
    b _03804928

_03804CCC:
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x25
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804CE4:
    mov r0, r9
    bl RTC_WriteStatus1
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x26
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804D04:
    add r0, r9, #2
    bl RTC_WriteStatus2
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x27
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804D24:
    add r0, r9, #4
    bl RTC_WriteAdjust
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x28
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804D44:
    add r0, r9, #4
    bl RTC_WriteFree
    mov r0, #0
    str r0, [r4, #0x1d8]
    mov r0, #0x29
    mov r1, #0
    bl RtcReturnResult
    b _03804928

_03804D64:
    mov r1, #0
    str r1, [r4, #0x1d8]
    mov r1, #1
    bl RtcReturnResult
    b _03804928
}

static asm void RtcAlarmIntr(void)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    add r0, sp, #2
    bl RTC_ReadStatus1
    ldrh r0, [sp, #2]
    mov r1, r0, lsl #0x1b
    movs r1, r1, lsr #0x1f
    bne _03804DB4
    mov r0, r0, lsl #0x1a
    movs r0, r0, lsr #0x1f
    beq _03804E0C

_03804DB4:
    add r0, sp, #0
    bl RTC_ReadStatus2
    ldrh r0, [sp, #2]
    mov r4, #0
    mov r0, r0, lsl #0x1b
    movs r0, r0, lsr #0x1f
    ldrneh r0, [sp]
    orrne r4, r4, #1
    bicne r0, r0, #0xf
    strneh r0, [sp]
    ldrh r0, [sp, #2]
    mov r0, r0, lsl #0x1a
    movs r0, r0, lsr #0x1f
    ldrneh r0, [sp]
    orrne r4, r4, #2
    bicne r0, r0, #0x40
    strneh r0, [sp]
    add r0, sp, #0
    bl RTC_WriteStatus2
    mov r1, r4
    mov r0, #0x30
    bl RtcReturnResult

_03804E0C:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

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

    {
        RTCRawData *pData = (RTCRawData *)(OS_GetSystemWork()->real_time_clock);
        u32 week;

        RTC_ReadDateTime(pData);
        week = RtcGetDayOfWeek((u32)(2000 + RtcBCD2HEX(pData->t.date.year)),
                RtcBCD2HEX(pData->t.date.month), RtcBCD2HEX(pData->t.date.day));

        if (pData->t.date.week != week) {
            pData->t.date.week = week;
            RTC_WriteDateTime(pData);
        }
    }

    RTC_SetHourFormat(1);
}

static u32 RtcGetDayOfWeek(u32 year, u32 month, u32 day)
{
    if (month == 1 || month == 2) {
        --year;
        month += 12;
    }
    return (u32)((year + year / 4 - year / 100 + year / 400 + (13 * month + 8) / 5 + day) % 7);
}

static asm int RtcBCD2HEX(u32 param0)
{
    stmdb sp!, {r4, lr}
    mov r12, #0
    mov r2, r12
    b _03804E44

_03804E28:
    mov r1, r2, lsl #2
    mov r1, r0, lsr r1
    and r1, r1, #0xf
    cmp r1, #0xa
    movcs r0, #0
    bcs _03804E80
    add r2, r2, #1

_03804E44:
    cmp r2, #8
    blt _03804E28
    mov r4, #0
    mov lr, #1
    mov r2, #0xa

_03804E58:
    mov r1, r4, lsl #2
    mov r1, r0, lsr r1
    and r3, r1, #0xf
    mul r1, lr, r2
    mla r12, lr, r3, r12
    add r4, r4, #1
    mov lr, r1
    cmp r4, #8
    blt _03804E58
    mov r0, r12

_03804E80:
    ldmia sp!, {r4, lr}
    bx lr
}

asm void RTC_Reset(void)
{
    stmdb sp!, {r3, lr}
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    ldrh r0, [sp]
    bic r0, r0, #1
    orr r0, r0, #1
    strh r0, [sp]
    bl RTCi_GpioStart
    mov r0, #6
    mov r1, #0
    bl RTCi_GpioSendCommand
    add r0, sp, #0
    mov r1, #1
    bl RTCi_GpioSendData
    bl RTCi_GpioEnd
    ldmia sp!, {r3, lr}
    bx lr
}
