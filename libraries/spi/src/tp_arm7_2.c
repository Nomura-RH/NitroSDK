
static u16 last_touch_flg;
static void SPI_DummyWait2(void);

static asm u32 TPi_DetectTouch(void)
{
    stmdb sp!, {r3, lr}
    mov r0, #0x8000
    bl EXIi_SelectRcnt
    ldr r2, =REG_SPICNT_ADDR

_0380146C:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _0380146C
    ldr r1, =0x8a01
    ldr r0, =REG_SPID_ADDR
    strh r1, [r2]
    mov r1, #0x84
    strh r1, [r0]
    sub r1, r0, #2

_03801490:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03801490
    bl SPI_DummyWait2
    ldr r1, =0x8201
    ldr r0, =REG_SPICNT_ADDR
    strh r1, [r0]
    bl SPI_DummyWait2
    ldr r0, =last_touch_flg
    ldrh r0, [r0]
    cmp r0, #0
    bne _038014D8
    ldr r0, =REG_RCNT0_H_ADDR
    ldrh r0, [r0]
    tst r0, #0x40
    moveq r0, #1
    movne r0, #0
    b _03801530

_038014D8:
    ldr r2, =REG_RCNT0_H_ADDR
    ldrh r0, [r2]
    tst r0, #0x40
    moveq r0, #1
    beq _03801530
    ldr r1, =0x8a01
    mov r0, #0x84
    strh r1, [r2, #0x8a]
    strh r0, [r2, #0x8c]

_038014FC:
    ldrh r0, [r2, #0x8a]
    tst r0, #0x80
    bne _038014FC
    bl SPI_DummyWait2
    ldr r1, =0x8201
    ldr r0, =REG_SPICNT_ADDR
    strh r1, [r0]
    bl SPI_DummyWait2
    ldr r0, =REG_RCNT0_H_ADDR
    ldrh r0, [r0]
    tst r0, #0x40
    movne r0, #0
    moveq r0, #2

_03801530:
    ldmia sp!, {r3, lr}
    bx lr
}

static void SPI_DummyWait2(void)
{
    reg_SPI_SPID = 0;
    while (reg_SPI_SPICNT & 0x80);
}

static asm u32 TPi_DetectPos(u16 *param1, u32 param2, u32 param3, u16 *param4)
{
    stmdb sp!, {r4, r5, r6, r7, r8, r9, lr}
    sub sp, sp, #0x14
    cmp r2, #0
    ldr r2, =REG_SPICNT_ADDR
    mov r5, r3
    moveq r3, #0xd1
    moveq r4, #1
    mov r7, r0
    mov r6, r1
    movne r3, #0x91
    movne r4, #2

_038015A0:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _038015A0
    ldr r1, =0x8a01
    ldr r0, =REG_SPID_ADDR
    strh r1, [r2]
    and r2, r3, #0xff
    strh r2, [r0]
    sub r1, r0, #2

_038015C4:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _038015C4
    mov r0, r2, lsl #0x10
    ldr r2, =REG_SPICNT_ADDR
    mov r12, #0
    ldr r8, =0x7ff8
    add r1, sp, #0
    mov r0, r0, lsr #0x10
    mov r3, r12

_038015EC:
    strh r3, [r2, #2]

_038015F0:
    ldrh r9, [r2]
    tst r9, #0x80
    bne _038015F0
    ldrh r9, [r2, #2]
    and r9, r9, #0xff
    mov lr, r9, lsl #0x10
    mov r9, lr, lsr #8
    str r9, [r1, r12, lsl #2]
    strh r0, [r2, #2]

_03801614:
    ldrh r9, [r2]
    tst r9, #0x80
    bne _03801614
    ldrh lr, [r2, #2]
    ldr r9, [r1, r12, lsl #2]
    and lr, lr, #0xff
    mov lr, lr, lsl #0x10
    orr r9, r9, lr, lsr #0x10
    and lr, r9, r8
    mov lr, lr, asr #3
    str lr, [r1, r12, lsl #2]
    add r12, r12, #1
    cmp r12, #5
    blt _038015EC
    ldr r0, =0x8201
    strh r0, [r2]
    bl SPI_DummyWait2
    mov r8, #0
    mov r9, r8
    add r2, sp, #0

_03801664:
    ldr r1, [r2, r9, lsl #2]
    add r3, r9, #1
    b _03801688

_03801670:
    ldr r0, [r2, r3, lsl #2]
    add r3, r3, #1
    subs r0, r1, r0
    rsbmi r0, r0, #0
    cmp r0, r8
    movgt r8, r0

_03801688:
    cmp r3, #5
    blt _03801670
    add r9, r9, #1
    cmp r9, #4
    blt _03801664
    strh r8, [r5]
    mov r8, #0
    add r0, sp, #0
    b _03801720

_038016AC:
    ldr r3, [r0, r8, lsl #2]
    add r9, r8, #1
    b _03801714

_038016B8:
    ldr r2, [r0, r9, lsl #2]
    subs r1, r3, r2
    rsbmi r1, r1, #0
    cmp r1, r6
    bgt _03801710
    add r1, r9, #1
    b _03801708

_038016D4:
    ldr r5, [r0, r1, lsl #2]
    subs r12, r3, r5
    rsbmi r12, r12, #0
    cmp r12, r6
    bgt _03801704
    add r0, r2, r3, lsl #1
    add r0, r5, r0
    mov r0, r0, asr #2
    bic r0, r0, #7
    strh r0, [r7]
    mov r0, #0
    b _03801744

_03801704:
    add r1, r1, #1

_03801708:
    cmp r1, #5
    blt _038016D4

_03801710:
    add r9, r9, #1

_03801714:
    cmp r9, #4
    blt _038016B8
    add r8, r8, #1

_03801720:
    cmp r8, #3
    blt _038016AC
    ldr r2, [sp]
    ldr r1, [sp, 0x10]
    mov r0, r4
    add r1, r2, r1
    mov r1, r1, asr #1
    bic r1, r1, #7
    strh r1, [r7]

_03801744:
    add sp, sp, #0x14
    ldmia sp!, {r4, r5, r6, r7, r8, r9, lr}
    bx lr
}

asm void TP_ExecSampling(SPITpData *param1, u32 param2, u16 *param3)
{
    stmdb sp!, {r3, r4, r5, r6, r7, lr}
    sub sp, sp, #8
    movs r6, r1
    mov r5, r2
    mov r1, #0
    mov r7, r0
    strh r1, [r5, #0]
    rsbmi r6, r6, #0
    bl TPi_DetectTouch
    movs r4, r0
    bne _038017CC
    ldr r1, [r7]
    mov r0, #0x1000
    rsb r0, r0, #0
    and r1, r1, r0
    ldr r0, =0xff000fff
    and r0, r1, r0
    bic r1, r0, #0x1000000
    bic r0, r1, #0x6000000
    str r1, [r7]
    orr r1, r0, #0x6000000
    str r1, [r7]
    ldr r0, =last_touch_flg
    mov r1, #0
    strh r1, [r0]
    b _0380193C

_038017CC:
    add r0, sp, #4
    add r3, sp, #2
    mov r1, r6
    mov r2, #0
    bl TPi_DetectPos
    ldr r1, [r7]
    mov r0, r0, lsl #0x1e
    bic r1, r1, #0x6000000
    orr r12, r1, r0, lsr #5
    str r12, [r7]
    mov r1, #0x1000
    rsb r1, r1, #0
    ldrh r2, [sp, #4]
    and r12, r12, r1
    and r1, r2, r1, lsr #0x14
    orr r12, r12, r1
    add r0, sp, #4
    add r3, sp, #0
    mov r1, r6
    mov r2, #1
    str r12, [r7]
    bl TPi_DetectPos
    cmp r0, #2
    bne _0380184C
    ldr r1, [r7]
    mov r0, r1, lsl #5
    mov r0, r0, lsr #0x1e
    orr r0, r0, #2
    bic r1, r1, #0x6000000
    mov r0, r0, lsl #0x1e
    orr r0, r1, r0, lsr #5
    str r0, [r7]

_0380184C:
    ldrh r1, [sp, #4]
    ldr r2, [r7]
    ldr r0, =0xff000fff
    mov r1, r1, lsl #0x14
    and r0, r2, r0
    orr r2, r0, r1, lsr #8
    ldr r1, =0x8a01
    ldr r0, =REG_SPICNT_ADDR
    str r2, [r7]
    strh r1, [r0]
    mov r6, #0

_03801878:
    bl SPI_DummyWait2
    add r6, r6, #1
    cmp r6, #0xc
    blt _03801878

_03801888:
    ldr r1, =0x8201
    ldr r0, =REG_SPICNT_ADDR
    strh r1, [r0]
    bl SPI_DummyWait2
    cmp r4, #2
    ldreq r0, [r7]
    biceq r0, r0, #0x6000000
    orreq r0, r0, #0x6000000
    streq r0, [r7]
    bl TPi_DetectTouch
    cmp r0, #0
    beq _0380191C
    cmp r0, #1
    beq _038018EC
    cmp r0, #2
    bne _03801938
    ldr r1, [r7]
    ldr r0, =last_touch_flg
    orr r2, r1, #0x1000000
    bic r1, r2, #0x6000000
    orr r1, r1, #0x6000000
    str r1, [r7]
    mov r1, #0
    strh r1, [r0]
    b _0380193C

_038018EC:
    ldr r1, [r7]
    ldr r0, =last_touch_flg
    orr r1, r1, #0x1000000
    str r1, [r7]
    mov r2, #1
    ldrh r1, [sp]
    ldrh r3, [sp, #2]
    strh r2, [r0]
    cmp r3, r1
    movcc r3, r1
    strh r3, [r5]
    b _0380193C

_0380191C:
    ldr r1, [r7]
    ldr r0, =last_touch_flg
    bic r1, r1, #0x1000000
    str r1, [r7]
    mov r1, #0
    strh r1, [r0]
    b _0380193C

_03801938:
    bl OS_Terminate

_0380193C:
    add sp, sp, #8
    ldmia sp!, {r3, r4, r5, r6, r7, lr}
    bx lr
}
