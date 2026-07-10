
#include <../include/card_rom.h>
#include <nitro/card/pullOut.h>

static struct {
    u32 unk_0;
    u32 unk_4;
} Unk_03806bf0 = { 0xffffffff, 1 };

static struct {
    u32 unk_0;
    u32 unk_4;
    u32 unk_8;
    u32 unk_c;
} Unk_03808f40;

static void CARDi_CallbackForPulledOut(PXIFifoTag tag, u32 data, BOOL err);

asm void CARD_InitPulledOutCallback(void)
{
    stmdb sp!, {r3, r4, r5, lr}
    ldr r0, =rom_stat + 0x220
    ldr r1, [r0, #8]
    cmp r1, #0
    bne _03800630
    mov r1, #1
    str r1, [r0, #8]
    bl PXI_Init
    mov r5, #0xe
    mov r4, #0

_03800610:
    mov r0, r5
    mov r1, r4
    bl PXI_IsCallbackReady
    cmp r0, #0
    beq _03800610
    ldr r1, =CARDi_CallbackForPulledOut
    mov r0, #0xe
    bl PXI_SetFifoRecvCallback

_03800630:
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

static asm void CARDi_CallbackForPulledOut(PXIFifoTag tag, u32 data, BOOL err)
{
    stmdb sp!, {r4, lr}
    and r0, r1, #0x3f
    cmp r0, #1
    bne _03800698
    mov r0, #0
    bl MI_StopDma
    mov r0, #1
    bl MI_StopDma
    mov r0, #2
    bl MI_StopDma
    mov r0, #3
    bl MI_StopDma
    mov r0, #0
    bl CTRDG_VibPulseEdgeUpdate
    bl OS_DisableInterrupts
    mov r4, r0
    bl SND_BeginSleep
    bl WVR_Shutdown
    mov r0, r4
    bl OS_RestoreInterrupts
    bl OS_Terminate
    b _0380069C

_03800698:
    bl OS_Terminate

_0380069C:
    ldmia sp!, {r4, lr}
    bx lr
}

asm BOOL CARD_IsPulledOut(void)
{
    stmdb sp!, {r3, lr}
    ldr r0, =Unk_03808f40
    ldr r0, [r0, #0xc]
    cmp r0, #0
    bne _038006D4
    ldr r0, =0x027ffe1f
    ldrb r0, [r0]
    tst r0, #0x80
    beq _038006D0
    bl CARD_CompareCardID
    b _038006D4

_038006D0:
    bl CARD_IsCardIreqLo

_038006D4:
    ldr r0, =Unk_03808f40
    ldr r0, [r0, #0xc]
    ldmia sp!, {r3, lr}
    bx lr
}

asm BOOL CARD_CompareCardID(void)
{
    stmdb sp!, {r3, r4, r5, lr}
    mov r5, #1
    bl OS_GetLockID
    mov r4, r0
    mvn r0, #2
    cmp r4, r0
    beq _03800764
    mov r0, r4, lsl #0x10
    mov r0, r0, lsr #0x10
    bl OS_TryLockCard
    cmp r0, #0
    bne _03800758
    ldr r1, =0x027ffc10
    ldrh r0, [r1]
    cmp r0, #0
    subeq r0, r1, #0x410
    subne r0, r1, #0x10
    ldr r0, [r0]
    str r0, [sp]
    bl CARDi_ReadRomID
    ldr r1, [sp]
    cmp r0, r1
    mov r0, r4, lsl #0x10
    moveq r5, #1
    mov r0, r0, lsr #0x10
    movne r5, #0
    bl OS_UnlockCard

_03800758:
    mov r0, r4, lsl #0x10
    mov r0, r0, lsr #0x10
    bl OS_ReleaseLockID

_03800764:
    ldr r1, =Unk_03808f40
    cmp r5, #0
    moveq r2, #1
    movne r2, #0
    str r2, [r1, #0xc]
    mov r0, r5
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

BOOL asm CARD_IsCardIreqLo(void)
{
    ldr r0, =REG_IF_ADDR
    mov r2, #1
    ldr r1, [r0]
    mov r0, r2
    tst r1, #0x100000
    ldrne r1, =Unk_03808f40
    movne r0, #0
    strne r2, [r1, #0xc]
    bx lr
}

asm void CARD_CheckPullOut_Polling(void)
{
    stmdb sp!, {r3, r4, r5, r6, r7, lr}
    ldr r0, =Unk_03808f40
    ldr r0, [r0, #4]
    cmp r0, #0
    bne _0380089C
    ldr r2, =0x027ffc40
    ldrh r0, [r2]
    cmp r0, #2
    beq _0380089C
    ldr r1, =Unk_03806bf0
    mvn r0, #0
    ldr r3, [r1]
    cmp r3, r0
    ldreq r0, [r2, #-4]
    addeq r0, r0, #0xa
    streq r0, [r1]
    beq _0380089C
    ldr r0, [r2, #-4]!
    cmp r0, r3
    bcc _0380089C
    ldr r0, [r2]
    add r0, r0, #0xa
    str r0, [r1]
    bl CARD_IsPulledOut
    cmp r0, #0
    beq _0380084C
    ldr r0, =Unk_03808f40
    mov r1, #1
    str r1, [r0, #4]
    bl CARD_GetRomHeader
    ldr r0, [r0, #0xc]
    cmp r0, #0
    bne _0380084C
    ldr r0, =Unk_03806bf0
    ldr r0, [r0, #4]
    cmp r0, #0
    bne _0380089C

_0380084C:
    ldr r0, =Unk_03808f40
    ldr r1, =Unk_03806bf0
    ldr r0, [r0, #4]
    mov r2, #0
    str r2, [r1, #4]
    cmp r0, #0
    beq _0380089C
    mov r7, #0x64
    mov r6, #0xe
    mov r5, #0x11
    mov r4, r2
    b _03800884

_0380087C:
    mov r0, r7
    bl SVC_WaitByLoop

_03800884:
    mov r0, r6
    mov r1, r5
    mov r2, r4
    bl PXI_SendWordByFifo
    cmp r0, #0
    bne _0380087C

_0380089C:
    ldmia sp!, {r3, r4, r5, r6, r7, lr}
    bx lr
}
