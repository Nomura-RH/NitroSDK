
#include "nitro/hw/ARM7/ioreg_SPI.h"
#include "nitro/os/common/printf.h"
#include <nitro/spi/common/pm_common.h>
#include <nitro/spi/ARM7/pm.h>

u16 PMi_TriggerBL = 0; // at 038094ce
u16 PMi_KeyPattern = 0; // at 038094cc
BOOL PMi_Initialized = FALSE; // at 038094d0
PMiWork PMi_Work = {}; // at 038094d4

PMLEDStatus PMi_LEDStatus = PM_LED_ON; // at 03806bf8
PMiBlinkPatternData PMi_BlinkPatternData[12] = { // at 03806bfc
    { 0, 0xaa000000, 8, 1, },
    { 0, 0xcc000000, 8, 1, },
    { 0, 0xe3800000, 0xc, 1 },
    { 0, 0xf0f00000, 0x10, 1 },
    { 0, 0xf83e0000, 0x14, 1 },
    { 0, 0xfc000000, 0xc, 1 },
    { 0, 0xff000000, 0x10, 1 },
    { 0, 0xffc00000, 0x14, 1 },
    { 0, 0xff000000, 0x20, 1 },
    { 0, 0xff00ff00, 0x20, 1 },
    { 0, 0xffffff00, 0x20, 1 },
    { 0, 0xc3000000, 0x28, 2 },
};

static void SPI_SendWait(u8 param1);

void PM_Init(void)
{
    PMi_Initialized = TRUE;
    PMi_Work.unk_20 = 0;

    for (int i = 0; i < 0x10; ++i) {
        PMi_Work.unk_0[i] = 0;
    }
}

asm void PM_AnalyzeCommand(u32 data)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    tst r0, #0x2000000
    beq _038019C8
    ldr r1, =PMi_Work
    mov r4, #0
    mov r3, r4

_038019B4:
    mov r2, r4, lsl #1
    add r4, r4, #1
    strh r3, [r1, r2]
    cmp r4, #0x10
    blt _038019B4

_038019C8:
    ldr r1, =PMi_Work
    and r2, r0, #0xf0000
    mov r2, r2, lsr #0x10
    mov r2, r2, lsl #1
    strh r0, [r1, r2]
    tst r0, #0x1000000
    beq _03801B40
    ldr r1, =PMi_Work - 8
    ldrh r2, [r1, #8]
    and r0, r2, #0xff00
    mov r0, r0, lsl #8
    mov r4, r0, lsr #0x10
    sub r0, r4, #0x60
    cmp r0, #7

    addls pc, pc, r0, lsl #2
    b _03801B34
    b _03801A28
    b _03801A38
    b _03801B34
    b _03801AD0
    b _03801A6C
    b _03801AA0
    b _03801B04
    b _03801B1C

_03801A28:
    mov r0, #0x60
    mov r1, #0
    bl SPIi_ReturnResult
    b _03801B40

_03801A38:
    ldrh r12, [r1, #0xa]
    mov r1, r4
    and r3, r2, #0xff
    mov r0, #3
    mov r2, #2
    str r12, [sp]
    bl SPIi_SetEntry
    cmp r0, #0
    bne _03801B40
    mov r0, r4
    mov r1, #4
    bl SPIi_ReturnResult
    b _03801B40

_03801A6C:
    ldrh r12, [r1, #0xa]
    mov r1, r4
    and r3, r2, #0xff
    mov r0, #3
    mov r2, #2
    str r12, [sp]
    bl SPIi_SetEntry
    cmp r0, #0
    bne _03801B40
    mov r0, r4
    mov r1, #4
    bl SPIi_ReturnResult
    b _03801B40

_03801AA0:
    mov r0, r2, lsl #0x10
    mov r1, r4
    mov r3, r0, lsr #0x10
    mov r0, #3
    mov r2, #1
    bl SPIi_SetEntry
    cmp r0, #0
    bne _03801B40
    mov r0, r4
    mov r1, #4
    bl SPIi_ReturnResult
    b _03801B40

_03801AD0:
    ldrh r0, [r1, #0xa]
    mov r2, r2, lsl #0x18
    mov r1, r4
    orr r3, r0, r2, lsr #8
    mov r0, #3
    mov r2, #1
    bl SPIi_SetEntry
    cmp r0, #0
    bne _03801B40
    mov r0, r4
    mov r1, #4
    bl SPIi_ReturnResult
    b _03801B40

_03801B04:
    and r0, r2, #0xff
    bl PM_SetLEDPattern
    mov r0, #0x66
    mov r1, #0
    bl SPIi_ReturnResult
    b _03801B40

_03801B1C:
    bl PM_GetLEDPattern
    mov r0, r0, lsl #0x10
    mov r1, r0, lsr #0x10
    mov r0, #0x67
    bl SPIi_ReturnResult
    b _03801B40

_03801B34:
    mov r0, r4
    mov r1, #1
    bl SPIi_ReturnResult
    
_03801B40:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

asm void PM_ExecuteProcess(SPIMessage *param1)
{
    stmdb sp!, {r3, r4, r5, lr}
    mov r4, r0
    bl OS_DisableInterrupts
    mov r5, r0
    mov r0, #3
    bl SPIi_CheckException
    cmp r0, #0
    bne _03801B94
    mov r0, r5
    bl OS_RestoreInterrupts
    ldr r0, [r4, #4]
    mov r1, #4
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    b _03801CB8

_03801B94:
    mov r0, #3
    bl SPIi_GetException
    mov r0, r5
    bl OS_RestoreInterrupts
    ldr r1, [r4, #4]
    sub r0, r1, #0x61
    cmp r0, #5

    addls pc, pc, r0, lsl #2
    b _03801CA0
    b _03801BD0
    b _03801CA0
    b _03801C6C
    b _03801BF4
    b _03801C30
    b _03801C94

_03801BD0:
    ldr r0, =PMi_Work - 8
    mov r1, #1
    str r1, [r0, #0x28]
    ldr r1, [r4, #8]
    strh r1, [r0, #2]
    ldr r1, [r4, #0xc]
    strh r1, [r0]
    bl PMi_DoSleep
    b _03801CB0

_03801BF4:
    ldr r2, =PMi_Work - 8
    mov r0, #4
    str r0, [r2, #0x28]
    ldr r0, [r4, #8]
    str r0, [r2, #0x30]
    ldr r3, [r4, #0xc]
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    and r1, r3, #0xff
    str r3, [r2, #0x2c]
    bl PMi_SetRegister
    mov r0, #0x64
    mov r1, #0
    bl SPIi_ReturnResult
    b _03801CB0

_03801C30:
    ldr r1, =PMi_Work - 8
    mov r0, #3
    str r0, [r1, #0x28]
    ldr r2, [r4, #8]
    mov r0, r2, lsl #0x10
    mov r4, r0, lsr #0x10
    mov r0, r4
    str r2, [r1, #0x30]
    bl PMi_GetRegister
    add r1, r4, #0x70
    mov r2, r1, lsl #0x10
    mov r1, r0
    mov r0, r2, lsr #0x10
    bl SPIi_ReturnResult
    b _03801CB0

_03801C6C:
    ldr r1, =PMi_Work - 8
    mov r0, #2
    str r0, [r1, #0x28]
    ldr r0, [r4, #8]
    str r0, [r1, #0x2c]
    bl PMi_SwitchUtilityProc
    mov r0, #0x63
    mov r1, #0
    bl SPIi_ReturnResult
    b _03801CB0

_03801C94:
    ldr r0, [r4, #8]
    bl PMi_SetLED
    b _03801CB0

_03801CA0:
    mov r0, r1, lsl #0x10
    mov r0, r0, lsr #0x10
    mov r1, #1
    bl SPIi_ReturnResult

_03801CB0:
    mov r0, #3
    bl SPIi_ReleaseException
_03801CB8:
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

asm void PMi_SendPxiCommand(u32 param1, u32 param2, u16 param3)
{
    stmdb sp!, {r4, r5, r6, lr}
    and r1, r1, #0x3f0000
    and r3, r0, #0x3c00000
    mov r1, r1, lsl #0x10
    mov r0, r2, lsl #0x10
    orr r1, r1, r3, lsl #0x16
    orr r6, r1, r0, lsr #0x10
    mov r5, #8
    mov r4, #0

_03801CE8:
    mov r0, r5
    mov r1, r6
    mov r2, r4
    bl PXI_SendWordByFifo
    cmp r0, #0
    bne _03801CE8
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

void PMi_SetRegister(u16 param1, u32 param2)
{
    while (reg_SPI_SPICNT & 0x80);
    reg_SPI_SPICNT = 0x8202;
    reg_SPI_SPICNT = 0x8802;
    SPI_SendWait(param1);
    reg_SPI_SPICNT = 0x8002;
    reg_SPI_SPID = param2 & 0xff;
}

static void SPI_SendWait(u8 param1)
{
    reg_SPI_SPID = param1 & 0xff;

    while (reg_SPI_SPICNT & 0x80);
}

u8 PMi_GetRegister(u8 param1)
{
    while (reg_SPI_SPICNT & 0x80);

    reg_SPI_SPICNT = 0x8202;
    reg_SPI_SPICNT = 0x8802;
    SPI_SendWait(param1 | 0x80);
    reg_SPI_SPICNT = 0x8002;
    reg_SPI_SPID = 0;

    while (reg_SPI_SPICNT & 0x80);
    
    return reg_SPI_SPID;
}

void PMi_SetControl(u8 ctrl)
{
    u8 v0;
    v0 = PMi_GetRegister(0);
    v0 |= ctrl;
    PMi_SetRegister(0, v0);
}

void PMi_ResetControl(u8 ctrl)
{
    u8 v0;
    v0 = PMi_GetRegister(0);
    v0 &= ~ctrl;
    PMi_SetRegister(0, v0);
}

asm void PMi_SwitchUtilityProc(u32 param1)
{
    stmdb sp!, {r3, lr}
    cmp r0, #0xf

    addls pc, pc, r0, lsl #2
    b _03801F64
    b _03801F64
    b _03801E98
    b _03801EAC
    b _03801EC0
    b _03801ED4
    b _03801EE0
    b _03801EEC
    b _03801EF8
    b _03801F04
    b _03801F10
    b _03801F1C
    b _03801F28
    b _03801F34
    b _03801F40
    b _03801F58
    b _03801F4C

_03801E98:
    mov r0, #1
    bl PM_SetLEDPattern
    mov r0, #1
    bl PMi_SetLED
    b _03801F64

_03801EAC:
    mov r0, #3
    bl PM_SetLEDPattern
    mov r0, #3
    bl PMi_SetLED
    b _03801F64

_03801EC0:
    mov r0, #2
    bl PM_SetLEDPattern
    mov r0, #2
    bl PMi_SetLED
    b _03801F64

_03801ED4:
    mov r0, #4
    bl PMi_SetControl
    b _03801F64

_03801EE0:
    mov r0, #4
    bl PMi_ResetControl
    b _03801F64

_03801EEC:
    mov r0, #8
    bl PMi_SetControl
    b _03801F64

_03801EF8:
    mov r0, #8
    bl PMi_ResetControl
    b _03801F64

_03801F04:
    mov r0, #0xc
    bl PMi_SetControl
    b _03801F64

_03801F10:
    mov r0, #0xc
    bl PMi_ResetControl
    b _03801F64

_03801F1C:
    mov r0, #1
    bl PMi_SetControl
    b _03801F64

_03801F28:
    mov r0, #1
    bl PMi_ResetControl
    b _03801F64

_03801F34:
    mov r0, #2
    bl PMi_ResetControl
    b _03801F64

_03801F40:
    mov r0, #2
    bl PMi_SetControl
    b _03801F64

_03801F4C:
    mov r0, #0x40
    bl PMi_ResetControl
    b _03801F64

_03801F58:
    bl SND_BeginSleep
    mov r0, #0x40
    bl PMi_SetControl

_03801F64:
    ldmia sp!, {r3, lr}
    bx lr
}

asm u32 PMi_SetLED(PMLEDStatus status)
{
    stmdb sp!, {r4, lr}
    mov r4, r0
    cmp r4, #1
    beq _03801F90
    cmp r4, #2
    beq _03801FA8
    cmp r4, #3
    beq _03801F9C
    b _03801FBC

_03801F90:
    mov r0, #0x10
    bl PMi_ResetControl
    b _03801FC0

_03801F9C:
    mov r0, #0x30
    bl PMi_SetControl
    b _03801FC0

_03801FA8:
    mov r0, #0x20
    bl PMi_ResetControl
    mov r0, #0x10
    bl PMi_SetControl
    b _03801FC0

_03801FBC:
    bl OS_Terminate

_03801FC0:
    ldr r0, =PMi_LEDStatus
    str r4, [r0]
    ldmia sp!, {r4, lr}
    bx lr
}

asm u16 PMi_DoSleep(void)
{
    stmdb sp!, {r3, r4, r5, r6, r7, r8, r9, lr}
    ldr r0, =REG_IME_ADDR
    mov r8, #0
    ldrh r9, [r0]
    strh r8, [r0]
    bl OS_DisableInterrupts
    mov r4, r0
    mvn r0, #0xfe000000
    bl OS_DisableIrqMask
    mov r5, r0
    mov r0, r8
    bl PMi_GetRegister
    mov r6, r0
    mov r0, #2
    bl PM_SetLEDPattern
    mov r0, #2
    bl PMi_SetLED
    mov r0, #2
    bl PMi_SetLED
    bl SND_BeginSleep
    mov r0, #1
    bl PMi_ResetControl
    ldr r0, =PMi_TriggerBL
    ldrh r0, [r0]
    tst r0, #1
    beq _03802058
    ldr r0, =PMi_KeyPattern
    ldr r1, =REG_KEYCNT_ADDR
    ldrh r2, [r0]
    mov r0, #0x1000
    orr r2, r2, #0x4000
    strh r2, [r1]
    bl OS_EnableIrqMask

_03802058:
    ldr r0, =PMi_TriggerBL
    ldrh r0, [r0]
    tst r0, #4
    beq _03802070
    mov r0, #0x400000
    bl OS_EnableIrqMask

_03802070:
    ldr r0, =PMi_TriggerBL
    ldrh r0, [r0]
    tst r0, #2
    beq _038020B4
    ldr r1, =REG_RCNT0_L_ADDR
    mov r0, #0x8000
    ldrh r7, [r1]
    mov r8, #1
    bl EXIi_SelectRcnt
    mov r0, #0x40
    mov r1, #0
    bl EXIi_SetBitRcnt0L
    mov r0, #0x100
    mov r1, r0
    bl EXIi_SetBitRcnt0L
    mov r0, #0x80
    bl OS_EnableIrqMask

_038020B4:
    ldr r0, =PMi_TriggerBL
    ldrh r0, [r0]
    tst r0, #8
    beq _038020CC
    mov r0, #0x100000
    bl OS_EnableIrqMask

_038020CC:
    ldr r0, =PMi_TriggerBL
    ldrh r0, [r0]
    tst r0, #0x10
    beq _038020E4
    mov r0, #0x2000
    bl OS_EnableIrqMask

_038020E4:
    mov r0, r4
    bl OS_RestoreInterrupts
    ldr r2, =REG_IME_ADDR
    mov r0, #1
    ldrh r1, [r2]
    strh r0, [r2]
    bl SVC_Sleep
    mov r1, r6
    mov r0, #0
    bl PMi_SetRegister
    ldr r0, =PMi_TriggerBL
    ldrh r1, [r0]
    tst r1, #0x20
    movne r0, #6
    moveq r0, #7
    tst r1, #0x40
    movne r6, #4
    moveq r6, #5
    bl PMi_SwitchUtilityProc
    mov r0, r6
    bl PMi_SwitchUtilityProc
    cmp r8, #0
    ldrne r0, =REG_RCNT0_L_ADDR
    strneh r7, [r0]
    mov r0, #1
    bl PMi_SetControl
    bl SND_EndSleep
    mov r1, #0
    ldr r3, =PMi_Work
    mov r2, r1
    mov r0, #0x62
    str r1, [r3, #0x20]
    bl PMi_SendPxiCommand
    bl OS_DisableInterrupts
    mov r0, r5
    bl OS_SetIrqMask
    mov r0, r4
    bl OS_RestoreInterrupts
    ldr r1, =REG_IME_ADDR
    ldrh r0, [r1]
    strh r9, [r1]
    ldmia sp!, {r3, r4, r5, r6, r7, r8, r9, lr}
    bx lr
}
