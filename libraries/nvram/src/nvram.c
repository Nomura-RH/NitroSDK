
#include "nitro/spi/ARM7/spi.h"
#include "nitro/spi/common/type.h"
#include <nitro/nvram.h>
#include <nitro/hw/ARM7/ioreg.h>

static struct {
    u16 unk_0[0x10];
} nvramw;

static BOOL NvramCheckReadyToRead(void);
static BOOL NvramCheckReadyToWrite(void);
static void SPI_DummyWait(void);

asm void NVRAM_Init(void)
{
    ldr r0, =nvramw
    mov r3, #0
    mov r2, r3

_03803814:
    mov r1, r3, lsl #1
    add r3, r3, #1
    strh r2, [r0, r1]
    cmp r3, #0x10
    blt _03803814
    bx lr
}

void NVRAM_AnalyzeCommand(u32 data)
{
    if (data & SPI_PXI_START_BIT) {
        s32 i;

        for (i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; ++i) {
            nvramw.unk_0[i] = 0;
        }
    }

    nvramw.unk_0[(data & SPI_PXI_INDEX_MASK) >> SPI_PXI_INDEX_SHIFT] = (u16)((data & SPI_PXI_DATA_MASK) >> SPI_PXI_DATA_SHIFT);

    if (data & SPI_PXI_END_BIT) {
        u16 command;
        u32 wu32;
        u32 buf;
        u32 addr;
        u32 size;

        command = (u16)((nvramw.unk_0[0] & 0xff00) >> 8);

        switch (command) {
        case SPI_PXI_COMMAND_NVRAM_RDSR:
        case SPI_PXI_COMMAND_NVRAM_RSI:
            wu32 = (((u32)(nvramw.unk_0[0] & 0x00ff) << 24) | ((u32)(nvramw.unk_0[1]) << 8) | ((u32)(nvramw.unk_0[2] & 0xff00) >> 8));
            if (wu32 < HW_MAIN_MEM || wu32 >= HW_MAIN_MEM_EX_END) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            buf = wu32;
            break;

        case SPI_PXI_COMMAND_NVRAM_FAST_READ:
        case SPI_PXI_COMMAND_NVRAM_READ:
            wu32 = (((u32)nvramw.unk_0[4] << 16) | ((u32)nvramw.unk_0[5]));
            if (wu32 < HW_MAIN_MEM || wu32 >= HW_MAIN_MEM_EX_END) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            buf = wu32;
            addr = (((u32)(nvramw.unk_0[0] & 0xff) << 16) | ((u32)nvramw.unk_0[1]));
            size = (((u32)nvramw.unk_0[2] << 16) | ((u32)nvramw.unk_0[3]));
            break;

        case SPI_PXI_COMMAND_NVRAM_PW:
        case SPI_PXI_COMMAND_NVRAM_PP:
            wu32 = (((u32)nvramw.unk_0[3] << 16) | ((u32)nvramw.unk_0[4]));
            if (wu32 < HW_MAIN_MEM || wu32 >= HW_MAIN_MEM_EX_END) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            buf = wu32;
            addr = (((u32)(nvramw.unk_0[0] & 0xff) << 16) | ((u32)nvramw.unk_0[1]));
            size = (u32)nvramw.unk_0[2];
            break;

        case SPI_PXI_COMMAND_NVRAM_PE:
        case SPI_PXI_COMMAND_NVRAM_SE:
            addr = (((u32)(nvramw.unk_0[0] & 0xff) << 16) | ((u32)nvramw.unk_0[1]));
            break;
        }

        if (!SPIi_SetEntry(SPI_DEVICE_TYPE_NVRAM, (u32)command, 3, addr, size, buf)) {
            SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
        }
    }
}

asm void NVRAM_ExecuteProcess(SPIMessage *param1)
{
    stmdb sp!, {r3, r4, r5, lr}
    mov r4, r0
    bl OS_DisableInterrupts
    mov r5, r0
    mov r0, #1
    bl SPIi_CheckException
    cmp r0, #0
    bne _03803A18
    mov r0, r5
    bl OS_RestoreInterrupts
    ldr r0, [r4, #4]
    mov r1, #4
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    b _03803CA4

_03803A18:
    mov r0, #1
    bl SPIi_GetException
    mov r0, r5
    bl OS_RestoreInterrupts
    ldr r0, [r4, #4]
    sub r0, r0, #0x20
    cmp r0, #0xd

    addls pc, pc, r0, lsl #2
    b _03803C68
    b _03803A74
    b _03803A7C
    b _03803A84
    b _03803A90
    b _03803AD0
    b _03803B10
    b _03803B58
    b _03803BA0
    b _03803DB8
    b _03803C10
    b _03803C18
    b _03803C20
    b _03803C54
    b _03803C60

_03803A74:
    bl NVRAM_WriteEnable
    b _03803C88

_03803A7C:
    bl NVRAM_WriteDisable
    b _03803C88

_03803A84:
    ldr r0, [r4, #0x10]
    bl NVRAM_ReadStatusRegister
    b _03803C88

_03803A90:
    bl NvramCheckReadyToRead
    cmp r0, #0
    bne _03803ABC
    ldr r0, [r4, #4]
    mov r1, #3
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException
    b _03803CA4

_03803ABC:
    ldr r0, [r4, #8]
    ldr r1, [r4, #0xc]
    ldr r2, [r4, #0x10]
    bl NVRAM_ReadDataBytes
    b _03803C88

_03803AD0:
    bl NvramCheckReadyToRead
    cmp r0, #0
    bne _03803AFC
    ldr r0, [r4, #4]
    mov r1, #3
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException
    b _03803CA4

_03803AFC:
    ldr r0, [r4, #8]
    ldr r1, [r4, #0xc]
    ldr r2, [r4, #0x10]
    bl NVRAM_ReadDataBytesAtHigherSpeed
    b _03803C88

_03803B10:
    bl NvramCheckReadyToWrite
    cmp r0, #0
    bne _03803B3C
    ldr r0, [r4, #4]
    mov r1, #3
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException
    b _03803CA4

_03803B3C:
    ldr r1, [r4, #0xc]
    ldr r0, [r4, #8]
    mov r1, r1, lsl #0x10
    ldr r2, [r4, #0x10]
    mov r1, r1, lsr #0x10
    bl NVRAM_PageWrite
    b _03803C88

_03803B58:
    bl NvramCheckReadyToWrite
    cmp r0, #0
    bne _03803B84
    ldr r0, [r4, #4]
    mov r1, #3
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException
    b _03803CA4

_03803B84:
    ldr r1, [r4, #0xc]
    ldr r0, [r4, #8]
    mov r1, r1, lsl #0x10
    ldr r2, [r4, #0x10]
    mov r1, r1, lsr #0x10
    bl NVRAM_PageProgram
    b _03803C88

_03803BA0:
    bl NvramCheckReadyToWrite
    cmp r0, #0
    bne _03803BCC
    ldr r0, [r4, #4]
    mov r1, #3
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException
    b _03803CA4

_03803BCC:
    ldr r0, [r4, #8]
    bl NVRAM_PageErase
    b _03803C88

_03803DB8:
    bl NvramCheckReadyToWrite
    cmp r0, #0
    bne _03803C04
    ldr r0, [r4, #4]
    mov r1, #3
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException
    b _03803CA4

_03803C04:
    ldr r0, [r4, #8]
    bl NVRAM_SectorErase
    b _03803C88

_03803C10:
    bl NVRAM_DeepPowerDown
    b _03803C88

_03803C18:
    bl NVRAM_ReleaseFromDeepPowerDown
    b _03803C88

_03803C20:
    bl NvramCheckReadyToWrite
    cmp r0, #0
    bne _03803C4C
    ldr r0, [r4, #4]
    mov r1, #3
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException
    b _03803CA4

_03803C4C:
    bl NVRAM_ChipErase
    b _03803C88

_03803C54:
    ldr r0, [r4, #0x10]
    bl NVRAM_ReadSiliconId
    b _03803C88

_03803C60:
    bl NVRAM_SoftwareReset
    b _03803C88

_03803C68:
    mov r0, #1
    bl SPIi_ReleaseException
    ldr r0, [r4, #4]
    mov r1, #1
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    b _03803CA4

_03803C88:
    ldr r0, [r4, #4]
    mov r1, #0
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #1
    bl SPIi_ReleaseException

_03803CA4:
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

static asm BOOL NvramCheckReadyToRead(void)
{
    stmdb sp!, {r3, lr}
    add r0, sp, #0
    bl NVRAM_ReadStatusRegister
    ldrh r0, [sp]
    tst r0, #1
    moveq r0, #1
    movne r0, #0
    ldmia sp!, {r3, lr}
    bx lr
}

static asm BOOL NvramCheckReadyToWrite(void)
{
    stmdb sp!, {r3, lr}
    add r0, sp, #0
    bl NVRAM_ReadStatusRegister
    ldrh r0, [sp, #0]
    tst r0, #1
    movne r0, #0
    bne _03803CF8
    tst r0, #2
    movne r0, #1
    moveq r0, #0

_03803CF8:
    ldmia sp!, {r3, lr}
    bx lr
}

asm void NVRAM_WriteEnable(void)
{
    ldr r2, =REG_SPICNT_ADDR

_03803D04:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03803D04
    ldr r0, =REG_SPID_ADDR
    mov r1, #0x8100
    strh r1, [r2]
    mov r1, #6
    strh r1, [r0]
    sub r1, r0, #2

_03803D28:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03803D28
    bx lr
}

static asm void sub_03803d40(u32 param1)
{
    ldr r1, =REG_SPID_ADDR
    and r0, r0, #0xff
    strh r0, [r1]
    sub r1, r1, #2

_03803D50:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03803D50
    bx lr
}

asm void NVRAM_WriteDisable(void)
{
    ldr r2, =REG_SPICNT_ADDR

_03803D68:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03803D68
    ldr r0, =REG_SPID_ADDR
    mov r1, #0x8100
    strh r1, [r2]
    mov r1, #4
    strh r1, [r0]
    sub r1, r0, #2

_03803D8C:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03803D8C
    bx lr
}

asm void NVRAM_ReadStatusRegister(u8 *param1)
{
    ldr r3, =REG_SPICNT_ADDR

_03803DA8:
    ldrh r1, [r3]
    tst r1, #0x80
    bne _03803DA8
    ldr r1, =REG_SPID_ADDR
    mov r2, #0x8900
    strh r2, [r3]
    mov r2, #5
    strh r2, [r1]
    sub r3, r1, #2

_03803DCC:
    ldrh r1, [r3]
    tst r1, #0x80
    bne _03803DCC
    ldr r1, =REG_SPID_ADDR
    mov r2, #0x8100
    strh r2, [r3]
    mov r2, #0
    strh r2, [r1]
    sub r2, r1, #2

_03803DF0:
    ldrh r1, [r2]
    tst r1, #0x80
    bne _03803DF0
    ldr r1, =REG_SPID_ADDR
    ldrh r1, [r1]
    strb r1, [r0]

    bx lr
}

asm BOOL NVRAM_ReadDataBytes(u32 address, u32 size, u8 *pData)
{
    stmdb sp!, {r3, r4, r5, lr}
    sub sp, sp, #8
    mov r5, r2
    cmp r1, #1
    bcc _03803F04
    and r2, r0, #0xff00
    mov r2, r2, lsr #8
    and r3, r0, #0xff0000
    strh r2, [sp, #2]
    mov r3, r3, lsr #0x10
    and r0, r0, #0xff
    ldr r2, =REG_SPICNT_ADDR
    strh r3, [sp]
    strh r0, [sp, #4]

_03803E4C:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03803E4C
    ldr r4, =REG_SPID_ADDR
    mov r0, #0x8900
    strh r0, [r2]
    mov r0, #3
    strh r0, [r4]
    mov r12, #0
    add r2, sp, #0
    sub r3, r4, #2

_03803E78:
    ldrh r0, [r3]
    tst r0, #0x80
    bne _03803E78
    mov r0, r12, lsl #1
    ldrh r0, [r2, r0]
    add r12, r12, #1
    and r0, r0, #0xff
    strh r0, [r4]
    cmp r12, #3
    blt _03803E78
    ldr r2, =REG_SPICNT_ADDR

_03803EA4:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03803EA4
    mov r4, #0
    ldr r2, =REG_SPICNT_ADDR
    sub r0, r1, #1
    mov r3, r4
    b _03803EE0

_03803EC4:
    strh r3, [r2, #2]

_03803EC8:
    ldrh r1, [r2]
    tst r1, #0x80
    bne _03803EC8
    ldrh r1, [r2, #2]
    strb r1, [r5, r4]
    add r4, r4, #1

_03803EE0:
    cmp r4, r0
    bcc _03803EC4
    ldr r0, =REG_SPICNT_ADDR
    mov r1, #0x8100
    strh r1, [r0]
    bl SPI_DummyWait
    ldr r0, =REG_SPID_ADDR
    ldrh r0, [r0]
    strb r0, [r5, r4]

_03803F04:
    add sp, sp, #8
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

static void SPI_DummyWait(void)
{
    reg_SPI_SPID = 0;
    while (reg_SPI_SPICNT & 0x80);
}

asm void NVRAM_ReadDataBytesAtHigherSpeed(u32 param1, u32 param2, u32 param3)
{
    stmdb sp!, {r3, r4, r5, lr}
    sub sp, sp, #8
    mov r5, r2
    cmp r1, #1
    bcc _03804048
    and r2, r0, #0xff00
    mov r2, r2, lsr #8
    and r3, r0, #0xff0000
    strh r2, [sp, #2]
    mov r3, r3, lsr #0x10
    and r0, r0, #0xff
    ldr r2, =REG_SPICNT_ADDR
    strh r3, [sp]
    strh r0, [sp, #4]

_03803F74:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03803F74
    ldr r4, =REG_SPID_ADDR
    mov r0, #0x8900
    strh r0, [r2]
    mov r0, #0xb
    strh r0, [r4]
    mov r12, #0
    add r2, sp, #0
    sub r3, r4, #2

_03803FA0:
    ldrh r0, [r3]
    tst r0, #0x80
    bne _03803FA0
    mov r0, r12, lsl #1
    ldrh r0, [r2, r0]
    add r12, r12, #1
    and r0, r0, #0xff
    strh r0, [r4]
    cmp r12, #3
    blt _03803FA0
    ldr r2, =REG_SPICNT_ADDR

_03803FCC:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03803FCC
    ldr r0, =REG_SPID_ADDR
    mov r2, #0
    strh r2, [r0]
    sub r2, r0, #2

_03803FE8:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03803FE8
    mov r4, #0
    ldr r2, =REG_SPICNT_ADDR
    sub r0, r1, #1
    mov r3, r4
    b _03804024

_03804008:
    strh r3, [r2, #2]

_0380400C:
    ldrh r1, [r2]
    tst r1, #0x80
    bne _0380400C
    ldrh r1, [r2, #2]
    strb r1, [r5, r4]
    add r4, r4, #1

_03804024:
    cmp r4, r0
    bcc _03804008
    ldr r0, =REG_SPICNT_ADDR
    mov r1, #0x8100
    strh r1, [r0]
    bl SPI_DummyWait
    ldr r0, =REG_SPID_ADDR
    ldrh r0, [r0]
    strb r0, [r5, r4]

_03804048:
    add sp, sp, #8
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

asm void NVRAM_PageWrite(u32 param1, u16 param2, u32 param3)
{
    stmdb sp!, {r4, lr}
    sub sp, sp, #8
    cmp r1, #1
    bcc _03804154
    add r3, r0, r1
    sub r3, r3, #1
    mov r3, r3, lsr #8
    cmp r3, r0, lsr #8
    and r3, r0, #0xff00
    mov r3, r3, lsr #8
    andhi r1, r0, #0xff
    and r12, r0, #0xff0000
    strh r3, [sp, #2]
    rsbhi r1, r1, #0x100
    mov r12, r12, lsr #0x10
    and r0, r0, #0xff
    movhi r1, r1, lsl #0x10
    ldr r3, =REG_SPICNT_ADDR
    strh r12, [sp]
    strh r0, [sp, #4]
    movhi r1, r1, lsr #0x10

_038040B0:
    ldrh r0, [r3, #0]
    tst r0, #0x80
    bne _038040B0
    ldr lr, =REG_SPID_ADDR
    mov r0, #0x8900
    strh r0, [r3]
    mov r0, #0xa
    strh r0, [lr]
    mov r4, #0
    add r3, sp, #0
    sub r12, lr, #2

_038040DC:
    ldrh r0, [r12]
    tst r0, #0x80
    bne _038040DC
    mov r0, r4, lsl #1
    ldrh r0, [r3, r0]
    add r4, r4, #1
    and r0, r0, #0xff
    strh r0, [lr]
    cmp r4, #3
    blt _038040DC
    sub r3, r1, #1
    ldr r1, =REG_SPICNT_ADDR
    mov r4, #0
    b _0380412C

_03804114:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03804114
    ldrb r0, [r2, r4]
    add r4, r4, #1
    strh r0, [r1, #2]

_0380412C:
    cmp r4, r3
    blt _03804114
    ldr r1, =REG_SPICNT_ADDR

_03804138:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03804138
    mov r0, #0x8100
    strh r0, [r1]
    ldrb r0, [r2, r4]
    bl sub_03803d40

_03804154:
    add sp, sp, #8
    ldmia sp!, {r4, lr}
    bx lr
}

asm void NVRAM_PageProgram(u32 param1, u16 param2, u32 param3)
{
    stmdb sp!, {r4, lr}
    sub sp, sp, #8
    cmp r1, #1
    bcc _03804260
    add r3, r0, r1
    sub r3, r3, #1
    mov r3, r3, lsr #8
    cmp r3, r0, lsr #8
    and r3, r0, #0xff00
    mov r3, r3, lsr #8
    andhi r1, r0, #0xff
    and r12, r0, #0xff0000
    strh r3, [sp, #2]
    rsbhi r1, r1, #0x100
    mov r12, r12, lsr #0x10
    and r0, r0, #0xff
    movhi r1, r1, lsl #0x10
    ldr r3, =REG_SPICNT_ADDR
    strh r12, [sp]
    strh r0, [sp, #4]
    movhi r1, r1, lsr #0x10

_038041BC:
    ldrh r0, [r3]
    tst r0, #0x80
    bne _038041BC
    ldr lr, =REG_SPID_ADDR
    mov r0, #0x8900
    strh r0, [r3]
    mov r0, #2
    strh r0, [lr]
    mov r4, #0
    add r3, sp, #0
    sub r12, lr, #2

_038041E8:
    ldrh r0, [r12]
    tst r0, #0x80
    bne _038041E8
    mov r0, r4, lsl #1
    ldrh r0, [r3, r0]
    add r4, r4, #1
    and r0, r0, #0xff
    strh r0, [lr]
    cmp r4, #3
    blt _038041E8
    sub r3, r1, #1
    ldr r1, =REG_SPICNT_ADDR
    mov r4, #0
    b _03804238

_03804220:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03804220
    ldrb r0, [r2, r4]
    add r4, r4, #1
    strh r0, [r1, #2]

_03804238:
    cmp r4, r3
    blt _03804220
    ldr r1, =REG_SPICNT_ADDR

_03804244:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03804244
    mov r0, #0x8100
    strh r0, [r1]
    ldrb r0, [r2, r4]
    bl sub_03803d40

_03804260:
    add sp, sp, #8
    ldmia sp!, {r4, lr}
    bx lr
}

asm void NVRAM_PageErase(u32 param1)
{
    stmdb sp!, {r3, r4, r5, lr}
    and r1, r0, #0xff0000
    mov r1, r1, lsr #0x10
    and r2, r0, #0xff00
    mov r2, r2, lsl #8
    ldr r3, =REG_SPICNT_ADDR
    mov r1, r1, lsl #0x10
    and r5, r0, #0xff
    mov r0, r1, lsr #0x10
    mov r4, r2, lsr #0x10

_0380429C:
    ldrh r1, [r3]
    tst r1, #0x80
    bne _0380429C
    ldr r1, =REG_SPID_ADDR
    mov r2, #0x8900
    strh r2, [r3]
    mov r2, #0xdb
    strh r2, [r1]
    sub r2, r1, #2

_038042C0:
    ldrh r1, [r2]
    tst r1, #0x80
    bne _038042C0
    bl sub_03803d40
    mov r0, r4
    bl sub_03803d40
    ldr r1, =REG_SPICNT_ADDR
    mov r2, #0x8100
    mov r0, r5
    strh r2, [r1]
    bl sub_03803d40
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

asm void NVRAM_SectorErase(u32 param1)
{
    stmdb sp!, {r3, r4, r5, lr}
    and r1, r0, #0xff0000
    mov r1, r1, lsr #0x10
    and r2, r0, #0xff00
    mov r2, r2, lsl #8
    ldr r3, =REG_SPICNT_ADDR
    mov r1, r1, lsl #0x10
    and r5, r0, #0xff
    mov r0, r1, lsr #0x10
    mov r4, r2, lsr #0x10

_03804324:
    ldrh r1, [r3]
    tst r1, #0x80
    bne _03804324
    ldr r1, =REG_SPID_ADDR
    mov r2, #0x8900
    strh r2, [r3]
    mov r2, #0xd8
    strh r2, [r1]
    sub r2, r1, #2

_03804348:  
    ldrh r1, [r2]
    tst r1, #0x80
    bne _03804348
    bl sub_03803d40
    mov r0, r4
    bl sub_03803d40
    ldr r1, =REG_SPICNT_ADDR
    mov r2, #0x8100
    mov r0, r5
    strh r2, [r1]
    bl sub_03803d40
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

asm void NVRAM_DeepPowerDown(u32 param1)
{
    ldr r2, =REG_SPICNT_ADDR
        
_03804388:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03804388
    ldr r0, =REG_SPID_ADDR
    mov r1, #0x8100
    strh r1, [r2]
    mov r1, #0xb9
    strh r1, [r0]
    sub r1, r0, #2

_038043AC:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _038043AC
    bx lr
}

asm void NVRAM_ReleaseFromDeepPowerDown(u32 param1)
{
    ldr r2, =REG_SPICNT_ADDR

_038043C8:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _038043C8
    ldr r0, =REG_SPID_ADDR
    mov r1, #0x8100
    strh r1, [r2]
    mov r1, #0xab
    strh r1, [r0]
    sub r1, r0, #2

_038043EC:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _038043EC
    bx lr
}

asm void NVRAM_ChipErase(u32 param1)
{
    ldr r2, =REG_SPICNT_ADDR

_03804408:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03804408
    ldr r0, =REG_SPID_ADDR
    mov r1, #0x8100
    strh r1, [r2]
    mov r1, #0xc7
    strh r1, [r0]
    sub r1, r0, #2

_0380442C:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _0380442C
    bx lr
}

asm void NVRAM_ReadSiliconId(u8 *param1)
{
    stmdb sp!, {r3, lr}
    ldr r3, =REG_SPICNT_ADDR

_0380444C:
    ldrh r1, [r3]
    tst r1, #0x80
    bne _0380444C
    ldr r1, =REG_SPID_ADDR
    mov r2, #0x8900
    strh r2, [r3]
    mov r2, #0x9f
    strh r2, [r1]
    sub r2, r1, #2

_03804470:
    ldrh r1, [r2]
    tst r1, #0x80
    bne _03804470
    ldr r1, =REG_SPID_ADDR
    mov r2, #0
    strh r2, [r1]
    sub lr, r1, #2

_0380448C:
    ldrh r1, [lr]
    tst r1, #0x80
    bne _0380448C
    ldr r12, =REG_SPID_ADDR
    mov r2, #0x8100
    ldrh r3, [r12]
    mov r1, #0
    strb r3, [r0, #0]
    strh r2, [lr, #0]
    strh r1, [r12, #0]
    sub r2, r12, #2

_038044B8:
    ldrh r1, [r2]
    tst r1, #0x80
    bne _038044B8
    ldr r1, =REG_SPID_ADDR
    ldrh r1, [r1]
    strb r1, [r0, #1]
    ldmia sp!, {r3, lr}
    bx lr
}

asm void NVRAM_SoftwareReset(void)
{
    ldr r2, =REG_SPICNT_ADDR

_038044E4:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _038044E4
    ldr r0, =REG_SPID_ADDR
    mov r1, #0x8100
    strh r1, [r2]
    mov r1, #0xff
    strh r1, [r0]
    sub r1, r0, #2

_03804508:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03804508
    bx lr
}
