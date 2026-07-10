
#include "nitro/exi/ARM7/genPort.h"
#include "nitro/rtc/ARM7/gpio.h"
#include <nitro/rtc/ARM7/instruction.h>

static void RtcChangeAlarmFormat24to12(void *param1);
static void RtcChangeAlarmFormat12to24(void *param1);
static void RtcGpioTransfer(int param1, u32 param2, void *param3, u32 param4);

asm void RTC_SetHourFormat(u32 param1)
{
    stmdb sp!, {r4, lr}
    sub sp, sp, #8
    mov r4, r0
    and r4, r4, #1
    cmp r4, #1
    bne _03804FE4
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    add r2, sp, #0
    mov r0, #0x86
    mov r1, #0
    mov r3, #1
    bl RtcGpioTransfer
    ldrh r0, [sp]
    mov r1, r0, lsl #0x1e
    mov r1, r1, lsr #0x1f
    cmp r1, r4
    beq _03804FE4
    bic r1, r0, #2
    mov r0, r4, lsl #0x1f
    orr r1, r1, r0, lsr #0x1e
    mov r0, #0x8000
    strh r1, [sp]
    bl EXIi_SelectRcnt
    bl RTCi_GpioStart
    mov r0, #6
    mov r1, #0
    bl RTCi_GpioSendCommand
    add r0, sp, #0
    mov r1, #1
    bl RTCi_GpioSendData
    bl RTCi_GpioEnd
    mov r0, #0x86
    mov r1, #0x10
    add r2, sp, #4
    mov r3, #3
    bl RtcGpioTransfer
    cmp r4, #0
    add r0, sp, #4
    bne _03804F74
    bl RtcChangeAlarmFormat24to12
    b _03804F78

_03804F74:
    bl RtcChangeAlarmFormat12to24

_03804F78:
    bl RTCi_GpioStart
    mov r0, #6
    mov r1, #0x10
    bl RTCi_GpioSendCommand
    add r0, sp, #4
    mov r1, #3
    bl RTCi_GpioSendData
    bl RTCi_GpioEnd
    mov r0, #0x86
    mov r1, #0x50
    add r2, sp, #4
    mov r3, #3
    bl RtcGpioTransfer
    cmp r4, #0
    add r0, sp, #4
    bne _03804FC0
    bl RtcChangeAlarmFormat24to12
    b _03804FC4

_03804FC0:
    bl RtcChangeAlarmFormat12to24

_03804FC4:
    bl RTCi_GpioStart
    mov r0, #6
    mov r1, #0x50
    bl RTCi_GpioSendCommand
    add r0, sp, #4
    mov r1, #3
    bl RTCi_GpioSendData
    bl RTCi_GpioEnd

_03804FE4:
    add sp, sp, #8
    ldmia sp!, {r4, lr}
    bx lr
}

void RTC_ReadDateTime(RTCRawData *param1)
{
    EXIi_SelectRcnt(0x8000);
    RtcGpioTransfer(0x86, 0x20, param1, 7);
}

void RTC_WriteDateTime(RTCRawData *param1)
{
    EXIi_SelectRcnt(0x8000);
    RTCi_GpioStart();
    RTCi_GpioSendCommand(6, 0x20);
    RTCi_GpioSendData(param1, 7);
    RTCi_GpioEnd();
}

void RTC_ReadDate(void *param1)
{
    EXIi_SelectRcnt(0x8000);
    RtcGpioTransfer(0x86, 0x20, param1, 4);
}

void RTC_ReadTime(void *param1)
{
    EXIi_SelectRcnt(0x8000);
    RtcGpioTransfer(0x86, 0x60, param1, 3);
}

void RTC_WriteTime(void *param1)
{
    EXIi_SelectRcnt(0x8000);
    RTCi_GpioStart();
    RTCi_GpioSendCommand(6, 0x60);
    RTCi_GpioSendData(param1, 3);
    RTCi_GpioEnd();
}

asm BOOL RTC_ReadPulse(void *param1)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    mov r4, r0
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    add r2, sp, #0
    mov r0, #0x86
    mov r1, #0x40
    mov r3, #1
    bl RtcGpioTransfer
    ldrh r0, [sp]
    mov r0, r0, lsl #0x1c
    mov r0, r0, lsr #0x1c
    and r0, r0, #0xb
    cmp r0, #1
    movne r0, #0
    bne _03805140
    mov r2, r4
    mov r0, #0x86
    mov r1, #0x10
    mov r3, #1
    bl RtcGpioTransfer
    mov r0, #1

_03805140:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

asm BOOL RTC_WritePulse(void *param1)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    mov r4, r0
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    add r2, sp, #0
    mov r0, #0x86
    mov r1, #0x40
    mov r3, #1
    bl RtcGpioTransfer
    ldrh r0, [sp]
    mov r0, r0, lsl #0x1c
    mov r0, r0, lsr #0x1c
    and r0, r0, #0xb
    cmp r0, #1
    movne r0, #0
    bne _038051B4
    bl RTCi_GpioStart
    mov r0, #6
    mov r1, #0x10
    bl RTCi_GpioSendCommand
    mov r0, r4
    mov r1, #1
    bl RTCi_GpioSendData
    bl RTCi_GpioEnd
    mov r0, #1

_038051B4:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

asm BOOL RTC_ReadAlarm1(void *param1)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    mov r4, r0
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    add r2, sp, #0
    mov r0, #0x86
    mov r1, #0x40
    mov r3, #1
    bl RtcGpioTransfer
    ldrh r0, [sp]
    mov r0, r0, lsl #0x1c
    mov r0, r0, lsr #0x1c
    cmp r0, #4
    movne r0, #0
    bne _03805218
    mov r2, r4
    mov r0, #0x86
    mov r1, #0x10
    mov r3, #3
    bl RtcGpioTransfer
    mov r0, #1

_03805218:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

asm BOOL RTC_WriteAlarm1(void *param1)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    mov r4, r0
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    add r2, sp, #0
    mov r0, #0x86
    mov r1, #0x40
    mov r3, #1
    bl RtcGpioTransfer
    ldrh r0, [sp]
    mov r0, r0, lsl #0x1c
    mov r0, r0, lsr #0x1c
    cmp r0, #4
    movne r0, #0
    bne _03805288
    bl RTCi_GpioStart
    mov r0, #6
    mov r1, #0x10
    bl RTCi_GpioSendCommand
    mov r0, r4
    mov r1, #3
    bl RTCi_GpioSendData
    bl RTCi_GpioEnd
    mov r0, #1

_03805288:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

asm BOOL RTC_ReadAlarm2(void *param1)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    mov r4, r0
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    add r2, sp, #0
    mov r0, #0x86
    mov r1, #0x40
    mov r3, #1
    bl RtcGpioTransfer
    ldrh r0, [sp]
    mov r0, r0, lsl #0x19
    movs r0, r0, lsr #0x1f
    moveq r0, #0
    beq _038052E8
    mov r2, r4
    mov r0, #0x86
    mov r1, #0x50
    mov r3, #3
    bl RtcGpioTransfer
    mov r0, #1

_038052E8:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

asm BOOL RTC_WriteAlarm2(void *param1)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    mov r4, r0
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    add r2, sp, #0
    mov r0, #0x86
    mov r1, #0x40
    mov r3, #1
    bl RtcGpioTransfer
    ldrh r0, [sp]
    mov r0, r0, lsl #0x19
    movs r0, r0, lsr #0x1f
    moveq r0, #0
    beq _03805354
    bl RTCi_GpioStart
    mov r0, #6
    mov r1, #0x50
    bl RTCi_GpioSendCommand
    mov r0, r4
    mov r1, #3
    bl RTCi_GpioSendData
    bl RTCi_GpioEnd
    mov r0, #1

_03805354:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

void RTC_ReadStatus1(RTCRawStatus1 *param1)
{
    EXIi_SelectRcnt(0x8000);
    RtcGpioTransfer(0x86, 0, param1, 1);
}

void RTC_WriteStatus1(RTCRawStatus1 *param1)
{
    EXIi_SelectRcnt(0x8000);
    RTCi_GpioStart();
    RTCi_GpioSendCommand(6, 0);
    RTCi_GpioSendData(param1, 1);
    RTCi_GpioEnd();
}

void RTC_ReadStatus2(RTCRawStatus2 *param1)
{
    EXIi_SelectRcnt(0x8000);
    RtcGpioTransfer(0x86, 0x40, param1, 1);
}

void RTC_WriteStatus2(RTCRawStatus2 *param1)
{
    EXIi_SelectRcnt(0x8000);
    RTCi_GpioStart();
    RTCi_GpioSendCommand(6, 0x40);
    RTCi_GpioSendData(param1, 1);
    RTCi_GpioEnd();
}

void RTC_ReadAdjust(void *param1)
{
    EXIi_SelectRcnt(0x8000);
    RtcGpioTransfer(0x86, 0x30, param1, 1);
}

void RTC_WriteAdjust(void *param1)
{
    EXIi_SelectRcnt(0x8000);
    RTCi_GpioStart();
    RTCi_GpioSendCommand(6, 0x30);
    RTCi_GpioSendData(param1, 1);
    RTCi_GpioEnd();
}

void RTC_ReadFree(void *param1)
{
    EXIi_SelectRcnt(0x8000);
    RtcGpioTransfer(0x86, 0x70, param1, 1);
}

void RTC_WriteFree(void *param1)
{
    EXIi_SelectRcnt(0x8000);
    RTCi_GpioStart();
    RTCi_GpioSendCommand(6, 0x70);
    RTCi_GpioSendData(param1, 1);
    RTCi_GpioEnd();
}


static asm void RtcChangeAlarmFormat24to12(void *param1)
{
    ldr r1, [r0]
    mov r2, r1, lsl #0x12
    mov r2, r2, lsr #0x1a
    cmp r2, #0x23

    addls pc, pc, r2, lsl #2
    b _038055EC
    b _03805598
    b _03805598
    b _03805598
    b _03805598
    b _03805598
    b _03805598
    b _03805598
    b _03805598
    b _03805598
    b _03805598
    b _038055EC
    b _038055EC
    b _038055EC
    b _038055EC
    b _038055EC
    b _038055EC
    b _03805598
    b _03805598
    b _038055A4
    b _038055A4
    b _038055A4
    b _038055A4
    b _038055A4
    b _038055A4
    b _038055A4
    b _038055A4
    b _038055EC
    b _038055EC
    b _038055EC
    b _038055EC
    b _038055EC
    b _038055EC
    b _038055C8
    b _038055C8
    b _038055A4
    b _038055A4

_03805598:
    bic r1, r1, #0x4000
    str r1, [r0]
    bx lr

_038055A4:
    orr r2, r1, #0x4000
    mov r1, r2, lsl #0x12
    mov r1, r1, lsr #0x1a
    sub r1, r1, #0x12
    bic r2, r2, #0x3f00
    mov r1, r1, lsl #0x1a
    orr r1, r2, r1, lsr #0x12
    str r1, [r0]
    bx lr

_038055C8:
    orr r2, r1, #0x4000
    mov r1, r2, lsl #0x12
    mov r1, r1, lsr #0x1a
    sub r1, r1, #0x18
    bic r2, r2, #0x3f00
    mov r1, r1, lsl #0x1a
    orr r1, r2, r1, lsr #0x12
    str r1, [r0]
    bx lr

_038055EC:
    ldr r1, [r0]
    bic r1, r1, #0x4000
    bic r1, r1, #0x3f00
    str r1, [r0]
    bx lr
}

static asm void RtcChangeAlarmFormat12to24(void *param1)
{
    ldr r2, [r0]
    mov r1, r2, lsl #0x12
    mov r3, r1, lsr #0x1a
    cmp r3, #0x23

    addls pc, pc, r3, lsl #2
    b _038056FC
    b _038056A8
    b _038056A8
    b _038056A8
    b _038056A8
    b _038056A8
    b _038056A8
    b _038056A8
    b _038056A8
    b _038056CC
    b _038056CC
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056A8
    b _038056A8
    b _038056F0
    b _038056F0
    b _038056F0
    b _038056F0
    b _038056F0
    b _038056F0
    b _038056F0
    b _038056F0
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056FC
    b _038056F0
    b _038056F0
    b _038056F0
    b _038056F0

_038056A8:
    mov r1, r2, lsl #0x11
    movs r1, r1, lsr #0x1f
    bxeq lr
    add r1, r3, #0x12
    bic r2, r2, #0x3f00
    mov r1, r1, lsl #0x1a
    orr r1, r2, r1, lsr #0x12
    str r1, [r0]
    bx lr

_038056CC:
    mov r1, r2, lsl #0x11
    movs r1, r1, lsr #0x1f
    bxeq lr
    add r1, r3, #0x18
    bic r2, r2, #0x3f00
    mov r1, r1, lsl #0x1a
    orr r1, r2, r1, lsr #0x12
    str r1, [r0]
    bx lr

_038056F0:
    orr r1, r2, #0x4000
    str r1, [r0]
    bx lr

_038056FC:
    ldr r1, [r0]
    bic r1, r1, #0x4000
    bic r1, r1, #0x3f00
    str r1, [r0]
    bx lr
}

static asm void RtcGpioTransfer(int param1, u32 param2, void *param3, u32 param4)
{
    stmdb sp!, {r3, r4, r5, r6, r7, lr}
    mov r7, r0
    mov r6, r1
    mov r5, r2
    mov r4, r3
    bl RTCi_GpioStart
    mov r0, r7
    mov r1, r6
    bl RTCi_GpioSendCommand
    cmp r7, #6
    beq _03805754
    cmp r7, #0x86
    bne _03805760
    mov r0, r5
    mov r1, r4
    bl RTCi_GpioReceiveData
    b _03805760

_03805754:
    mov r0, r5
    mov r1, r4
    bl RTCi_GpioSendData

_03805760:
    bl RTCi_GpioEnd
    ldmia sp!, {r3, r4, r5, r6, r7, lr}
    bx lr
}

