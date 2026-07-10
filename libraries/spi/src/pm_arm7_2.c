
extern void _u32_div_f();

PMLEDPattern PMi_BlinkPatternNo; // at 03809504
static u32 PMi_BlinkCounter;

asm void PM_SelfBlinkProc(void)
{
    stmdb sp!, {r4, r5, r6, lr}
    ldr r1, =PMi_BlinkCounter
    ldr r3, [r1, #4]
    cmp r3, #0
    bne _038021F0
    mov r2, #1
    mov r3, r2
    mov r0, #3
    mov r1, #0x66
    bl SPIi_SetEntry
    cmp r0, #0
    beq _038022C0
    mov r0, #1
    bl PM_SetLEDPattern
    b _038022C0

_038021F0:
    cmp r3, #4
    bge _0380221C
    ldr r0, =PMi_LEDStatus
    ldr r0, [r0]
    cmp r3, r0
    beq _038022C0
    mov r0, #3
    mov r1, #0x66
    mov r2, #1
    bl SPIi_SetEntry
    b _038022C0

_0380221C:
    sub r2, r3, #4
    mov r0, #0xc
    mul r4, r2, r0
    ldr r5, =PMi_BlinkPatternData
    ldr r0, [r1]
    add r6, r5, r4
    ldrh r1, [r6, #0xa]
    bl _u32_div_f
    mov r1, #0
    mov r3, r1, lsr r0
    ldr r12, [r6, #4]
    mov r2, #0x80000000
    rsb r1, r0, #0x20
    orr r3, r3, r2, lsl r1
    sub r1, r0, #0x20
    and r12, r12, r2, lsr r0
    orr r3, r3, r2, lsr r1
    ldr r0, [r5, r4]
    cmp r12, #0
    and r0, r0, r3
    cmpeq r0, #0
    movne r3, #1
    ldrh r2, [r6, #8]
    ldrh r1, [r6, #0xa]
    ldr r0, =PMi_BlinkCounter
    mul r1, r2, r1
    ldr r2, [r0]
    moveq r3, #2
    add r2, r2, #1
    cmp r2, r1
    str r2, [r0]
    movcs r1, #0
    strcs r1, [r0]
    ldr r0, =PMi_LEDStatus
    ldr r0, [r0]
    cmp r3, r0
    beq _038022C0
    mov r0, #3

_038022B4:
    mov r1, #0x66
    mov r2, #1
    bl SPIi_SetEntry

_038022C0:
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

asm void PM_SetLEDPattern(PMLEDPattern pattern)
{
    cmp r0, #0xf
    ldrle r1, =PMi_BlinkCounter
    movle r2, #0
    strle r0, [r1, #4]
    strle r2, [r1, #0]
    bx lr
}

PMLEDPattern PM_GetLEDPattern(void)
{
    return PMi_BlinkPatternNo;
}
