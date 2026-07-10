#include <nitro/os.h>
#include <nitro/nvram.h>

extern void WVR_ShelterExtWram(void);
extern u16 WMSP_GetAllowedChannel(u16 bitField);

static u32 GetRomValidLanguage(void);
static void VBlankIntr(void);

asm void NitroSpMain(void)
{
    stmdb sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11, lr}
    sub sp, sp, #0x210
    bl WVR_ShelterExtWram
    bl OS_Init
    bl OS_InitThread
    add r2, sp, #4
    mov r0, #0x20
    mov r1, #2
    bl NVRAM_ReadDataBytes
    ldr r0, [sp, #4]
    add r2, sp, #0x10
    mov r0, r0, lsl #3
    str r0, [sp, #4]
    mov r1, #0x100
    bl NVRAM_ReadDataBytes
    ldr r0, [sp, #4]
    mov r1, #0x100
    add r0, r0, #0x100
    add r2, sp, #0x110
    bl NVRAM_ReadDataBytes
    mov r0, #0x1d
    mov r1, #1
    add r2, sp, #0
    mov r6, #0
    bl NVRAM_ReadDataBytes
    ldrb r0, [sp]
    cmp r0, #0xff
    moveq r0, r6
    beq _037F8080
    tst r0, #0x50
    movne r0, #1
    moveq r0, r6

_037F8080:
    cmp r0, #0
    beq _037F8154
    bl GetRomValidLanguage
    mov r8, r0
    mov r11, #1
    and r7, r8, #0x40
    mov r10, #0
    add r5, sp, #0x10
    mov r4, r11
    b _037F8148

_037F80A8:
    ldr r0, =0x0000ffff
    add r9, r5, r10, lsl #8
    mov r1, r9
    mov r2, #0x70
    bl SVC_GetCRC16
    mov r2, r9
    ldrh r1, [r2, #0x72]
    cmp r0, r1
    bne _037F813C
    ldrh r0, [r2, #0x70]
    cmp r0, #0x80
    bcs _037F813C
    ldr r0, =0x0000ffff
    mov r2, #0x8a
    add r1, r9, #0x74
    bl SVC_GetCRC16
    mov r2, r9
    ldrh r1, [r2, #0xfe]
    cmp r0, r1
    bne _037F813C
    ldrh r1, [r2, #0x76]
    ldrb r0, [r2, #0x75]
    tst r1, r4, lsl r0
    beq _037F813C
    tst r8, r1
    ldrneh r1, [r9, #0x64]
    andne r0, r0, #7
    bicne r1, r1, #7
    orrne r0, r1, r0
    strneh r0, [r9, #0x64]
    add r0, r5, r10, lsl #8
    ldrh r0, [r0, #0x76]
    mvn r0, r0
    tst r7, r0
    movne r6, #3
    bne _037F81F4
    orr r6, r6, r11, lsl r10

_037F813C:
    add r0, r10, #1
    mov r0, r0, lsl #0x10
    mov r10, r0, lsr #0x10

_037F8148:
    cmp r10, #2
    bcc _037F80A8
    b _037F81B8

_037F8154:
    bl GetRomValidLanguage
    tst r0, #0x40
    movne r6, #3
    bne _037F81F4
    ldr r8, =0x0000ffff
    mov r9, #0
    add r7, sp, #0x10
    mov r4, #1
    mov r5, #0x70

_037F8178:
    mov r0, r8
    mov r2, r5
    add r1, r7, r9, lsl #8
    bl SVC_GetCRC16
    add r2, r7, r9, lsl #8
    ldrh r1, [r2, #0x72]
    cmp r0, r1
    bne _037F81A4
    ldrh r0, [r2, #0x70]
    cmp r0, #0x80
    orrcc r6, r6, r4, lsl r9

_037F81A4:
    add r0, r9, #1
    mov r0, r0, lsl #0x10
    mov r9, r0, lsr #0x10
    cmp r9, #2
    bcc _037F8178

_037F81B8:
    cmp r6, #1
    cmpne r6, #2
    beq _037F81F4
    cmp r6, #3
    bne _037F81F0
    ldrh r1, [sp, #0x80]
    add r0, sp, #0x100
    add r1, r1, #1
    ldrh r0, [r0, #0x80]
    and r1, r1, #0x7f
    cmp r1, r0
    moveq r6, #2
    movne r6, #1
    b _037F81F4

_037F81F0:
    mov r6, #0

_037F81F4:
    cmp r6, #3
    blt _037F8210
    ldr r1, =0x027ffc80
    mvn r0, #0
    mov r2, #0x74
    bl MIi_CpuClear32
    b _037F82CC

_037F8210:
    cmp r6, #0
    beq _037F82BC
    ldr r0, =0xffffff2a
    mov r1, r6, lsl #8
    add r0, sp, r0
    ldrb r0, [r0, r6, lsl #8]
    cmp r0, #0xa
    bcs _037F825C
    add r0, sp, #0x10
    mov r3, #0xa
    mov r2, #0
    add r1, r0, r1
    b _037F8250

_037F8244:
    add r0, r1, r3, lsl #1
    strh r2, [r0, #-0xfc]
    sub r3, r3, #1

_037F8250:
    ldrb r0, [r1, #-0xe6]
    cmp r3, r0
    bgt _037F8244

_037F825C:
    ldr r0, =0xffffff60
    mov r1, r6, lsl #8
    add r0, sp, r0
    ldrb r0, [r0, r6, lsl #8]
    cmp r0, #0x1a
    bcs _037F82A0
    add r0, sp, #0x10
    mov r3, #0x1a
    mov r2, #0
    add r1, r0, r1
    b _037F8294

_037F8288:
    add r0, r1, r3, lsl #1
    strh r2, [r0, #-0xe6]
    sub r3, r3, #1

_037F8294:
    ldrb r0, [r1, #-0xb0]
    cmp r3, r0
    bgt _037F8288

_037F82A0:
    ldr r1, =0x027ffc80
    add r2, sp, #0x10
    sub r0, r6, #1
    add r0, r2, r0, lsl #8
    mov r2, #0x74
    bl MIi_CpuCopy32
    b _037F82CC

_037F82BC:
    ldr r1, =0x027ffc80
    mov r0, #0
    mov r2, #0x74
    bl MIi_CpuClear32

_037F82CC:
    add r2, sp, #8
    mov r0, #0x36
    mov r1, #6
    bl NVRAM_ReadDataBytes
    ldr r4, =0x027ffc80
    add r0, sp, #8
    add r1, r4, #0x74
    mov r2, #6
    bl MI_CpuCopy8
    add r2, sp, #2
    mov r0, #0x3c
    mov r1, #2
    bl NVRAM_ReadDataBytes
    ldrh r0, [sp, #2]
    mov r0, r0, lsl #0xf
    mov r0, r0, lsr #0x10
    bl WMSP_GetAllowedChannel
    strh r0, [r4, #0x7a]
    bl PXI_Init
    mov r0, #8
    bl OS_GetArenaHi
    mov r4, r0
    mov r0, #8
    bl OS_GetArenaLo
    mov r1, r0
    mov r2, r4
    mov r0, #8
    mov r3, #1
    bl OS_InitAlloc
    mov r4, r0
    mov r0, #8
    bl OS_GetArenaHi
    sub r2, r0, r4
    mov r0, r4
    mov r1, #0
    bl MI_CpuFill8
    mov r1, r4
    mov r0, #8
    bl OS_SetArenaLo
    mov r0, #8
    bl OS_GetArenaHi
    mov r4, r0
    mov r0, #8
    bl OS_GetArenaLo
    mov r1, r0
    mov r2, r4
    mov r0, #8
    bl OS_CreateHeap
    movs r4, r0
    bpl _037F8398
    bl OS_Terminate

_037F8398:
    mov r1, r4
    mov r0, #8
    bl OS_SetCurrentHeap
    mov r1, r4
    mov r0, #8
    bl OS_CheckHeap
    cmp r0, #0x2100
    bcs _037F83BC
    bl OS_Terminate

_037F83BC:
    mov r0, #6
    bl SND_Init
    bl PAD_InitXYButton
    ldr r1, =VBlankIntr
    mov r0, #1
    bl OS_SetIrqFunction
    mov r0, #1
    bl OS_EnableIrqMask
    ldr r3, =REG_DISPSTAT_ADDR
    mov r0, #1
    ldrh r1, [r3]
    ldrh r1, [r3]
    add r2, r3, #0x204
    orr r1, r1, #8
    strh r1, [r3]
    ldrh r1, [r2]
    strh r0, [r2]
    bl OS_EnableInterrupts
    mvn r0, #0
    bl FS_Init
    mov r0, #0xf
    bl CARD_SetThreadPriority
    mov r0, #0xc
    bl RTC_Init
    mov r0, r4
    bl WVR_Init
    mov r0, #2
    bl SPI_Init
    mov r4, #0

_037F8430:
    bl SVC_Halt
    bl OS_IsResetOccurred
    cmp r0, #0
    beq _037F844C
    mov r0, r4
    bl CTRDG_VibPulseEdgeUpdate
    bl OS_ResetSystem

_037F844C:
    bl CTRDG_CheckPullOut_Polling
    bl CARD_CheckPullOut_Polling
    b _037F8430
}

static asm u32 GetRomValidLanguage(void)
{
    ldr r1, =0x027ffe1d
    mov r0, #0
    ldrb r1, [r1]
    cmp r1, #0x80
    orreq r0, r0, #0x40
    moveq r0, r0, lsl #0x10
    moveq r0, r0, lsr #0x10
    bxeq lr
    cmp r1, #0x40
    orreq r0, r0, #0x80
    moveq r0, r0, lsl #0x10
    moveq r0, r0, lsr #0x10
    bx lr
}

static asm void VBlankIntr(void)
{
    stmdb sp!, {r3, lr}
    ldr r0, =PMi_Initialized
    ldr r0, [r0]
    cmp r0, #0
    beq _037F84D8
    bl PM_SelfBlinkProc

_037F84D8:
    ldmia sp!, {r3, lr}
    bx lr
}
