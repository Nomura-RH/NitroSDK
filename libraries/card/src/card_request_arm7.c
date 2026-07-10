
#include "../include/card_common.h"
#include "../include/card_rom.h"
#include "../include/card_spi.h"

asm void CARDi_OnFifoRecv(PXIFifoTag tag, u32 data, BOOL err)
{
    stmdb sp!, {r3, lr}
    cmp r0, #0xb
    bne _038003A8
    cmp r2, #0
    beq _038003A8
    ldr r0, =cardi_common
    ldr r2, [r0, #8]
    cmp r2, #0
    streq r1, [r0, #4]
    ldr r2, [r0, #4]
    cmp r2, #0xf

    addls pc, pc, r2, lsl #2
    b _03800374
    b _03800344
    b _03800374
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368
    b _03800368

_03800344:
    ldr r2, [r0, #8]
    cmp r2, #0
    beq _03800374
    cmp r2, #1
    ldreq r2, [r0, #0xfc]
    streq r1, [r0, #0]
    orreq r1, r2, #0x10
    streq r1, [r0, #0xfc]
    b _03800374

_03800368:
    ldr r1, [r0, #0xfc]
    orr r1, r1, #0x10
    str r1, [r0, #0xfc]

_03800374:
    ldr r1, [r0, #0xfc]
    tst r1, #0x10
    ldreq r1, [r0, #8]
    addeq r1, r1, #1
    streq r1, [r0, #8]
    beq _038003A8
    ldr r1, [r0, #0xfc]
    mov r2, #0
    str r2, [r0, #8]
    tst r1, #4
    ldrne r0, [r0, #0xec]
    addeq r0, r0, #0x48
    bl OS_WakeupThreadDirect

_038003A8:
    ldmia sp!, {r3, lr}
    bx lr
}

asm void CARDi_TaskThread(void *arg)
{
    stmdb sp!, {r4, r5, r6, r7, r8, lr}
    ldr r4, =cardi_common

_038003BC:
    mov r5, #0
    bl OS_DisableInterrupts
    mov r6, r0
    add r8, r4, #0x48
    mov r7, r5

_038003D0:
    ldr r0, [r4, #0xfc]
    tst r0, #4
    bne _03800408
    ldr r0, [r4, #0xfc]
    tst r0, #0x10
    beq _03800414
    ldr r0, [r4, #0xfc]
    mov r5, #1
    orr r0, r0, #4
    str r0, [r4, #0xfc]
    ldr r0, [r4, #0xfc]
    bic r0, r0, #0x10
    str r0, [r4, #0xfc]
    b _03800424

_03800408:
    ldr r0, [r4, #0xfc]
    tst r0, #8
    bne _03800424

_03800414:
    mov r0, r7
    str r8, [r4, #0xec]
    bl OS_SleepThread
    b _038003D0

_03800424:
    mov r0, r6
    bl OS_RestoreInterrupts
    cmp r5, #0
    beq _038005D0
    ldr r0, [r4]
    mov r1, #0
    str r1, [r0]
    ldr r3, [r4]
    ldr r2, [r4, #4]
    ldr r1, [r3, #0x58]
    mov r0, #1
    tst r1, r0, lsl r2
    moveq r0, #3
    streq r0, [r3]
    beq _03800574
    cmp r2, #0xf

    addls pc, pc, r2, lsl #2
    b _0380056C
    b _03800574
    b _03800574
    b _038004AC
    b _038004B4
    b _038004C4
    b _0380056C
    b _038004D0
    b _038004E4
    b _038004F8
    b _0380050C
    b _0380056C
    b _03800520
    b _03800540
    b _03800548
    b _0380055C
    b _03800530

_038004AC:
    bl CARDi_InitStatusRegister
    b _03800574

_038004B4:
    bl CARDi_ReadRomIDCore
    ldr r1, [r4]
    str r0, [r1, #8]
    b _03800574

_038004C4:
    mov r0, #3
    str r0, [r3]
    b _03800574

_038004D0:
    ldr r0, [r3, #0xc]
    ldr r1, [r3, #0x10]
    ldr r2, [r3, #0x14]
    bl CARDi_ReadBackupCore
    b _03800574

_038004E4:
    ldr r0, [r3, #0x10]
    ldr r1, [r3, #0xc]
    ldr r2, [r3, #0x14]
    bl CARDi_WriteBackupCore
    b _03800574

_038004F8:
    ldr r0, [r3, #0x10]
    ldr r1, [r3, #0xc]
    ldr r2, [r3, #0x14]
    bl CARDi_ProgramBackupCore
    b _03800574

_0380050C:
    ldr r0, [r3, #0x10]
    ldr r1, [r3, #0xc]
    ldr r2, [r3, #0x14]
    bl CARDi_VerifyBackupCore
    b _03800574

_03800520:
    ldr r0, [r3, #0x10]
    ldr r1, [r3, #0x14]
    bl CARDi_EraseBackupSectorCore
    b _03800574

_03800530:
    ldr r0, [r3, #0x10]
    ldr r1, [r3, #0x14]
    bl CARDi_EraseBackupSubSectorCore
    b _03800574

_03800540:
    bl CARDi_EraseChipCore
    b _03800574

_03800548:
    bl CARDi_CommandReadStatus
    ldr r1, [r4]
    ldr r1, [r1, #0x10]
    strb r0, [r1]
    b _03800574

_0380055C:
    ldr r0, [r3, #0xc]
    ldrb r0, [r0]
    bl CARDi_SetWriteProtectCore
    b _03800574

_0380056C:
    mov r0, #3
    str r0, [r3]

_03800574:
    mov r6, #0xb
    mov r5, #1

_0380057C:
    mov r0, r6
    mov r1, r5
    mov r2, r5
    bl PXI_SendWordByFifo
    cmp r0, #0
    blt _0380057C
    bl OS_DisableInterrupts
    ldr r1, [r4, #0xfc]
    mov r5, r0
    bic r0, r1, #0x4c
    str r0, [r4, #0xfc]
    add r0, r4, #0xf4
    bl OS_WakeupThread
    ldr r0, [r4, #0xfc]
    tst r0, #0x10
    beq _038005C4
    add r0, r4, #0x48
    bl OS_WakeupThreadDirect

_038005C4:
    mov r0, r5
    bl OS_RestoreInterrupts
    b _038003BC

_038005D0:
    ldr r1, [r4, #0x44]
    mov r0, r4
    mov lr, pc
    bx r1
    b _038003BC
}
