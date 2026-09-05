#include "nitro/rtc/ARM7/gpio.h"

#include <nitro/hw/ARM7/ioreg_EXI.h>
#include <nitro/hw/ARM7/mmap_global.h>

#include <nitro/code32.h>

asm void RTCi_GpioStart(void)
{
    mov r12, #HW_REG_BASE
    add r12, r12, #REG_RCNT1_OFFSET
    ldrh r0, [r12]
    bic r0, r0, #RTC_GPIO_MASK
    orr r0, r0, #RTC_GPIO_DIRECTION_SEND | RTC_GPIO_CS_LO | RTC_GPIO_CLOCK_HI
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TDS

@loop_tds:
    subs r3, r3, #1
    bne @loop_tds

    bic r0, r0, #RTC_GPIO_CS_MASK
    orr r0, r0, #RTC_GPIO_CS_HI
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TCSH

@loop_tcsh:
    subs r3, r3, #1
    bne @loop_tcsh
    bx lr
}

asm void RTCi_GpioEnd(void)
{
    mov r12, #HW_REG_BASE
    add r12, r12, #REG_RCNT1_OFFSET
    mov r3, #RTC_WAIT_LOOP_COUNT_TCSS

@loop_tcss:
    subs r3, r3, #1
    bne @loop_tcss

    ldrh r0, [r12]
    bic r0, r0, #RTC_GPIO_CS_MASK
    orr r0, r0, #RTC_GPIO_CS_LO
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TDH

@loop_tdh:
    subs r3, r3, #1
    bne @loop_tdh
    bx lr
}

asm void RTCi_GpioSendCommand(u16 command, u16 parameter)
{
    mov r12, #HW_REG_BASE
    add r12, r12, #REG_RCNT1_OFFSET
    orr r1, r0, r1
    ldrh r0, [r12]
    bic r0, r0, #RTC_GPIO_MASK
    orr r0, r0, #RTC_GPIO_DIRECTION_SEND | RTC_GPIO_CS_HI
    mov r2, #0

@loop01:
    bic r0, r0, #RTC_GPIO_CLOCK_MASK | RTC_GPIO_DATA_MASK
    orr r0, r0, #RTC_GPIO_CLOCK_LO
    mov r3, #1
    tst r3, r1, lsr r2
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    bne @movne1
    b @movne2
@movne1:
    mov r3, #RTC_GPIO_DATA_HI
@movne2:
    beq @moveq1
    b @moveq2
@moveq1:
    mov r3, #RTC_GPIO_DATA_LO
@moveq2:
#else
    movne r3, #RTC_GPIO_DATA_HI
    moveq r3, #RTC_GPIO_DATA_LO
#endif
    orr r0, r0, r3
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TSCK

@loop_tsck_lo:
    subs r3, r3, #1
    bne @loop_tsck_lo

    bic r0, r0, #RTC_GPIO_CLOCK_MASK
    orr r0, r0, #RTC_GPIO_CLOCK_HI
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TSCK

@loop_tsck_hi:
    subs r3, r3, #1
    bne @loop_tsck_hi

    add r2, r2, #1
    cmp r2, #8
    bne @loop01
    bx lr
}


asm void RTCi_GpioSendData(const void *pData, u32 size)
{
    mov r12, #HW_REG_BASE
    add r12, r12, #REG_RCNT1_OFFSET

@loop01:
    stmdb sp!, {r0, r1}
    tst r0, #1
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @ldreqh1
    b @ldreqh2
@ldreqh1:
    ldrh r1, [r0]
@ldreqh2:
    bne @ldrneh1
    b @ldrneh2
@ldrneh1:
    ldrh r1, [r0, #-1]
@ldrneh2:
    bne @movne1
    b @movne2
@movne1:
    mov r1, r1, lsr #8
@movne2:
#else
    ldreqh r1, [r0]
    ldrneh r1, [r0, #-1]
    movne r1, r1, lsr #8
#endif
    ldrh r0, [r12]
    bic r0, r0, #RTC_GPIO_MASK
    orr r0, r0, #RTC_GPIO_DIRECTION_SEND | RTC_GPIO_CS_HI
    mov r2, #0

@loop02:
    bic r0, r0, #RTC_GPIO_CLOCK_MASK | RTC_GPIO_DATA_MASK
    orr r0, r0, #RTC_GPIO_CLOCK_LO
    mov r3, #1
    tst r3, r1, lsr r2
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    bne @movne3
    b @movne4
@movne3:
    mov r3, #RTC_GPIO_DATA_HI
@movne4:
    beq @moveq1
    b @moveq2
@moveq1:
    mov r3, #RTC_GPIO_DATA_LO
@moveq2:
#else
    movne r3, #RTC_GPIO_DATA_HI
    moveq r3, #RTC_GPIO_DATA_LO
#endif
    orr r0, r0, r3
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TSCK

@loop_tsck_lo:
    subs r3, r3, #1
    bne @loop_tsck_lo

    bic r0, r0, #RTC_GPIO_CLOCK_MASK
    orr r0, r0, #RTC_GPIO_CLOCK_HI
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TSCK

@loop_tsck_hi:
    subs r3, r3, #1
    bne @loop_tsck_hi

    add r2, r2, #1
    cmp r2, #8
    bne @loop02

    ldmia sp!, {r0, r1}
    add r0, r0, #1
    subs r1, r1, #1
    bne @loop01

    bx lr
}

asm void RTCi_GpioReceiveData(void *pData, u32 size)
{
    mov r12, #HW_REG_BASE
    add r12, r12, #REG_RCNT1_OFFSET

@loop01:
    stmdb sp!, {r0, r1}
    ldrh r0, [r12]
    bic r0, r0, #RTC_GPIO_MASK
    orr r0, r0, #RTC_GPIO_DIRECTION_RECV | RTC_GPIO_CS_HI
    mov r2, #0
    mov r1, #0

@loop02:
    bic r0, r0, #RTC_GPIO_CLOCK_MASK | RTC_GPIO_DATA_MASK
    orr r0, r0, #RTC_GPIO_CLOCK_LO
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TSCK

@loop_tsck_lo:
    subs r3, r3, #1
    bne @loop_tsck_lo

    ldrh r0, [r12]
    and r3, r0, #RTC_GPIO_DATA_MASK
    cmp r3, #RTC_GPIO_DATA_HI
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @moveq1
    b @moveq2
@moveq1:
    mov r3, #0x80
@moveq2:
    bne @movne1
    b @movne2
@movne1:
    mov r3, #0
@movne2:
#else
    moveq r3, #0x80
    movne r3, #0
#endif
    orr r2, r3, r2, lsr #1
    bic r0, r0, #RTC_GPIO_CLOCK_MASK
    orr r0, r0, #RTC_GPIO_CLOCK_HI
    strh r0, [r12]
    mov r3, #RTC_WAIT_LOOP_COUNT_TSCK

@loop_tsck_hi:
    subs r3, r3, #1
    bne @loop_tsck_hi

    add r1, r1, #1
    cmp r1, #8
    bne @loop02

    ldmia sp!, {r0, r1}
    tst r0, #1
    beq @even

@odd:
    ldrh r3, [r0, #-1]
    bic r3, r3, #0xFF00
    mov r2, r2, lsl #0x8
    orr r3, r2, r3
    strh r3, [r0, #-1]
    b @judge_seq

@even:
    ldrh r3, [r0]
    bic r3, r3, #0xFF
    orr r3, r3, r2
    strh r3, [r0]

@judge_seq:
    add r0, r0, #1
    subs r1, r1, #1
    bne @loop01

    bx lr
}

#include <nitro/codereset.h>
