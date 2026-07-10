
static struct {
    u32 unk_0;
    u32 unk_4;
    u32 unk_8;
    u32 unk_c;
} micIntroInfo;

static const struct {
    u32 unk_0;
    u32 unk_4;
} micIntrPrio[22] = {
    { 0x40, 6 },
    { 0x1000000, 0x18 },
    { 0x80000, 0x13 },
    { 0x100000, 0x14 },
    { 0x40000, 0x12 },
    { 8, 3 },
    { 0x10, 4 },
    { 0x20, 5 },
    { 0x100, 8 },
    { 0x200, 9 },
    { 0x400, 0xa },
    { 0x800, 0xb },
    { 0x1000, 0xc },
    { 0x2000, 0xd },
    { 2, 1 },
    { 4, 2 },
    { 1, 0 },
    { 0x10000, 0x10 },
    { 0x400000, 0x16 },
    { 0x80, 7 },
    { 0x20000, 0x11 },
    { 0x800000, 0x17 },
};

asm void MIC_SetIrqFunction(u32 param1, void (*param2)(void))
{
    ldr r2, =OS_IRQTable
    mov r3, #0

_03802BE8:
    tst r0, #1
    strne r1, [r2, r3, lsl #2]
    add r3, r3, #1
    cmp r3, #0x19
    mov r0, r0, lsr #1
    blt _03802BE8
    bx lr
}

asm void MIC_EnableMultipleInterrupt(void)
{
    stmdb sp!, {r3, lr}
    ldr r3, =HW_INTR_VECTOR_BUF
    ldr r0, =MIC_IrqHandler
    ldr r2, [r3]
    cmp r2, r0
    beq _03802C54
    ldr r0, =micIntroInfo
    mov r1, #0
    str r1, [r0]
    sub r1, r3, #0x17c
    str r1, [r0, #4]
    mov r1, #0x40
    str r1, [r0, #8]
    str r2, [r0, #0xc]
    bl OS_DisableInterrupts
    ldr r2, =MIC_IrqHandler
    ldr r1, =HW_INTR_VECTOR_BUF
    str r2, [r1]
    bl OS_RestoreInterrupts

_03802C54:
    ldmia sp!, {r3, lr}
    bx lr
}

asm void MIC_DisableMultipleInterrupt(void)
{
    stmdb sp!, {r3, lr}
    ldr r1, =HW_INTR_VECTOR_BUF
    ldr r0, =MIC_IrqHandler
    ldr r1, [r1]
    cmp r1, r0
    bne _03802C98
    bl OS_DisableInterrupts
    ldr r1, =micIntroInfo
    ldr r2, =HW_INTR_VECTOR_BUF
    ldr r1, [r1, #0xc]
    str r1, [r2]
    bl OS_RestoreInterrupts

_03802C98:
    ldmia sp!, {r3, lr}
    bx lr
}

asm void MIC_IrqHandler(void)
{
    mov r12, #0x4000000
    add r1, r12, #0x208
    ldrh r0, [r1]
    tst r0, r0
    beq _03802CC4
    b _03802CC8

_03802CC4:
    bx lr

_03802CC8:
    ldr r3, [r12, #0x210]
    ldr r1, [r12, #0x214]
    ands r2, r1, r3
    beq _03802CDC
    b _03802CE0

_03802CDC:
    bx lr

_03802CE0:
    ldr r0, =0x1df3fff
    tst r2, r0
    beq _03802CF0
    b _03802CF4

_03802CF0:
    str r2, [r12, #0x214]

_03802CF4:
    beq _03802CFC
    b _03802D00

_03802CFC:
    bx lr

_03802D00:
    stmdb sp!, {lr}
    mrs r0, spsr
    stmdb sp!, {r0}
    stmdb sp, {sp, lr}^
    sub sp, sp, #8
    mov r0, #0x9f
    msr cpsr_c, r0
    ldr r1, =OSi_ThreadInfo
    ldrh r0, [r1, #2]
    add r0, r0, #1
    strh r0, [r1, #2]
    ldr r1, =micIntroInfo
    cmp r0, #1
    beq _03802D3C
    b _03802D40

_03802D3C:
    mov r0, sp

_03802D40:
    beq _03802D48
    b _03802D4C
    
_03802D48:
    ldr sp, [r1, #4]

_03802D4C:
    beq _03802D54
    b _03802D58

_03802D54:
    str r0, [r1, #4]

_03802D58:
    stmdb sp!, {r3}
    ldr r1, =micIntrPrio
    ldr r0, [r1]
    tst r0, r2
    bne _03802D70
    b _03802D74

_03802D70:
    str r0, [r12, #0x214]

_03802D74:
    bne _03802D7C
    b _03802D80

_03802D7C:
    ldr r0, [r1, #4]

_03802D80:
    bne _03802D88
    b _03802D8C

_03802D88:
    ldr r3, =OS_IRQTable

_03802D8C:
    bne _03802D94
    b _03802D98

_03802D94:
    ldr r0, [r3, r0, lsl #2]

_03802D98:
    bne _03802E08
    mov r3, #1

_03802DA0:
    ldr r0, [r1, r3, lsl #3]
    tst r0, r2
    beq _03802DB0
    b _03802DB4

_03802DB0:
    add r3, r3, #1

_03802DB4:
    beq _03802DA0
    str r0, [r12, #0x214]
    add r0, r1, r3, lsl #3
    ldr r2, [r0, #4]
    ldr r3, =OS_IRQTable
    ldr r0, [r3, r2, lsl #2]
    ldr r2, =OSi_ThreadInfo
    ldrh r3, [r2, #2]
    cmp r3, #1
    beq _03802DE0
    b _03802DE4

_03802DE0:
    ldr r2, [r1]

_03802DE4:
    beq _03802DEC
    b _03802DF0

_03802DEC:
    str r2, [r12, #0x210]

_03802DF0:
    beq _03802DF8
    b _03802DFC

_03802DF8:
    mov r2, #0x1f

_03802DFC:
    beq _03802E04
    b _03802E08

_03802E04:
    msr cpsr_c, r2

_03802E08:
    ldr r1, [r12, #0x210]
    stmdb sp!, {r1}
    add lr, pc, #0
    bx r0

_03802E18:
    mov r0, #0x9f
    msr cpsr_c, r0
    mov r12, #0x4000000
    ldmia sp!, {r0}
    ldr r1, [r12, #0x210]
    eor r2, r0, r1
    and r1, r2, r1
    and r0, r2, r0
    ldmia sp!, {r3}
    orr r3, r3, r1
    bic r3, r3, r0
    str r3, [r12, #0x210]
    ldr r2, =OSi_ThreadInfo
    ldr r3, =micIntroInfo
    ldrh r0, [r2, #2]
    subs r1, r0, #1
    strh r1, [r2, #2]
    beq _03802E64
    b _03802E68

_03802E64:
    mov r0, sp

_03802E68:
    beq _03802E70
    b _03802E74

_03802E70:
    ldr sp, [r3, #4]

_03802E74:
    beq _03802E7C
    b _03802E80

_03802E7C:
    str r0, [r3, #4]

_03802E80:
    mov r0, #0x92
    msr cpsr_c, r0
    ldmia sp, {sp, lr}^
    mov r0, r0
    add sp, sp, #8
    ldmia sp!, {r0}
    msr spsr_cf, r0
    tst r1, r1
    beq _03802EA8
    b _03802EAC

_03802EA8:
    ldr r0, =OS_IrqHandler_ThreadSwitch
    
_03802EAC:
    beq _03802EB4
    b _03802EB8

_03802EB4:
    add lr, pc, #0

_03802EB8:
    beq _03802EC0

_03802EBC:
    b _03802EC4

_03802EC0:
    bx r0

_03802EC4:
    ldmia sp!, {pc}
}
