
#include <nitro/wm/ARM7/wm_sp.h>

#include <nitro/wram_begin.h>

asm void WMSP_ReturnResult2Wm9(void)
{
    stmdb sp!, {r3, r4, r5, r6, r7, lr}
    mov r7, r0
    mov r6, #0x100
    mov r5, #0xa
    mov r4, #0
    b _038069E0

_038069D8:
    mov r0, r6
    bl SVC_WaitByLoop

_038069E0:
    mov r0, r5
    mov r1, r7
    mov r2, r4
    bl PXI_SendWordByFifo
    cmp r0, #0
    blt _038069D8
    ldr r0, =wmspW.unk_1528
    bl OS_UnlockMutex
    ldmia sp!, {r3, r4, r5, r6, r7, lr}
    bx lr
}

asm void WMSP_GetBuffer4Callback2Wm9(void)
{
    stmdb sp!, {r3, r4, r5, lr}
    ldr r0, =wmspW.unk_1528
    bl OS_LockMutex
    ldr r4, =0x027fff96
    mov r5, #0x100
    b _03806A2C

_03806A24:
    mov r0, r4
    bl SVC_WaitByLoop

_03806A2C:
    ldrh r1, [r4]
    tst r1, #1
    bne _03806A24
    ldr r0, =wmspW + 0x1000
    orr r1, r1, #1
    strh r1, [r4]
    ldr r0, [r0, #0x54c]
    ldr r0, [r0, #8]
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

asm u32 WMSP_GetAllowedChannel(u32 param1)
{
    stmdb sp!, {r4, r5, r6, lr}
    ldr r1, =0x00001fff
    and r0, r0, r1
    mov r0, r0, lsl #0x10
    movs r2, r0, lsr #0x10
    moveq r0, #0
    beq _03806B60
    mov r1, #0
    mov r0, #1
    b _03806A94

_03806A88:
    tst r2, r0, lsl r1
    bne _03806A9C
    add r1, r1, #1

_03806A94:
    cmp r1, #0x10
    blt _03806A88

_03806A9C:
    mov r0, #0xf
    mov r3, #1
    b _03806AB4

_03806AA8:
    tst r2, r3, lsl r0
    bne _03806ABC
    sub r0, r0, #1

_03806AB4:
    cmp r0, #0
    bne _03806AA8

_03806ABC:
    sub r6, r0, r1
    cmp r6, #5
    movlt r0, #1
    movlt r0, r0, lsl r1
    movlt r0, r0, lsl #0x10
    movlt r0, r0, lsr #0x10
    blt _03806B60
    add r3, r0, r1
    add r3, r3, r3, lsr #0x1f
    mov r4, r3, asr #1
    mov r5, #0
    mov lr, #1
    b _03806B14

_03806AF0:
    mov r12, r5, lsr #0x1f
    rsb r3, r12, r5, lsl #0x1f
    add r3, r12, r3, ror #0x1f
    mov r3, r3, lsl #1
    sub r3, r3, #1
    mla r4, r5, r3, r4
    tst r2, lr, lsl r4
    bne _03806B1C
    add r5, r5, #1

_03806B14:
    cmp r5, r6
    ble _03806AF0

_03806B1C:
    sub r2, r0, r4
    cmp r2, #5
    subge r2, r4, r1
    cmpge r2, #5
    bge _03806B48
    mov r2, #1
    mov r0, r2, lsl r0
    orr r0, r0, r2, lsl r1
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    b _03806B60

_03806B48:
    mov r3, #1
    mov r2, r3, lsl r4
    orr r0, r2, r3, lsl r0
    orr r0, r0, r3, lsl r1
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10

_03806B60:
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

#include <nitro/wram_end.h>

