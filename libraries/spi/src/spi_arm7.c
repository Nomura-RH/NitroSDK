
#include "nitro/mi/memory.h"
#include "nitro/spi/common/type.h"
#include <nitro/os/common/thread.h>
#include <nitro/os/common/message.h>
#include <nitro/pxi/common/fifo.h>

#define SPI_MESSAGE_QUEUE_LENGTH 0x10
#define SPI_THREAD_STACK_SIZE 0x200

u16 spiInitialized;
static struct {
    u32 unk_0;
    u32 unk_4;
    OSThread unk_8;
    u8 unk_ac[SPI_THREAD_STACK_SIZE];
    OSMessageQueue unk_2ac;
    OSMessage unk_2cc[SPI_MESSAGE_QUEUE_LENGTH];
    SPIMessage unk_30c[0x10];
    u32 unk_48c;
    OSThreadQueue unk_490;
    u32 unk_498;
} spiWork;

static void SpiCommonThread(void *);
static void SpiPxiCallback(PXIFifoTag tag, u32 data, BOOL err);

void SPI_Init(u32 prio)
{
    if (spiInitialized) {
        return;
    }
    spiInitialized = 1;

    spiWork.unk_0 = FALSE;
    spiWork.unk_4 = SPI_DEVICE_TYPE_MAX;

    TP_Init();
    NVRAM_Init();
    MIC_Init();
    PM_Init();

    PXI_Init();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_TOUCHPANEL, SpiPxiCallback);
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_MIC, SpiPxiCallback);
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_PM, SpiPxiCallback);
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_NVRAM, SpiPxiCallback);

    OS_InitMessageQueue(&(spiWork.unk_2ac), spiWork.unk_2cc, SPI_MESSAGE_QUEUE_LENGTH);
    {
        s32 i;

        for (i = 0; i < SPI_MESSAGE_QUEUE_LENGTH; ++i) {
            MI_CpuFill8(&spiWork.unk_30c[i], 0, sizeof(SPIMessage));
        }
        spiWork.unk_48c = 0;
    }

    OS_InitThreadQueue(&(spiWork.unk_490));
    OS_CreateThread(&(spiWork.unk_8), SpiCommonThread, NULL, (void *)(spiWork.unk_ac + SPI_THREAD_STACK_SIZE), SPI_THREAD_STACK_SIZE, prio);
    OS_WakeupThreadDirect(&(spiWork.unk_8));
}

asm void SPI_Lock(u32 param1)
{
    stmdb sp!, {r4, r5, r6, lr}
    ldr r5, =spiWork.unk_490
    ldr r4, =spiWork - 4
    mov r6, r0

_03800A54:
    bl OS_DisableInterrupts
    ldr r1, [r4, #4]
    cmp r1, #0
    beq _03800A74
    bl OS_RestoreInterrupts
    mov r0, r5
    bl OS_SleepThread
    b _03800A54

_03800A74:
    ldr r1, =spiWork - 4
    mov r2, #1
    str r2, [r1, #4]
    mov r2, #4
    str r2, [r1, #8]
    str r6, [r1, #0x49c]
    bl OS_RestoreInterrupts
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

asm void SPI_Unlock(u32 param1)
{
    stmdb sp!, {r3, lr}
    ldr r1, =spiWork - 4
    ldr r2, [r1, #4]
    cmp r2, #0
    beq _03800AF0
    ldr r2, [r1, #8]
    cmp r2, #4
    ldreq r1, [r1, #0x49c]
    cmpeq r1, r0
    bne _03800AF0
    bl OS_DisableInterrupts
    ldr r1, =spiWork - 4
    mov r2, #5
    str r2, [r1, #8]
    mov r2, #0
    str r2, [r1, #4]
    str r2, [r1, #0x49c]
    bl OS_RestoreInterrupts
    ldr r0, =spiWork.unk_490
    bl OS_WakeupThread

_03800AF0:
    ldmia sp!, {r3, lr}
    bx lr
}

asm void SPIi_ReturnResult(u32 param1, u32 param2)
{
    stmdb sp!, {r4, r5, r6, lr}
    and r2, r0, #0x70
    cmp r2, #0x30
    bgt _03800B38
    bge _03800B84
    cmp r2, #0x10
    bgt _03800B2C
    bge _03800B6C
    cmp r2, #0
    beq _03800B6C
    b _03800B88

_03800B2C:
    cmp r2, #0x20
    beq _03800B84
    b _03800B88

_03800B38:
    cmp r2, #0x50
    bgt _03800B50
    bge _03800B74
    cmp r2, #0x40
    beq _03800B74
    b _03800B88

_03800B50:
    cmp r2, #0x60
    bgt _03800B60
    beq _03800B7C
    b _03800B88

_03800B60:
    cmp r2, #0x70
    beq _03800B7C
    b _03800B88

_03800B6C:
    mov r4, #6
    b _03800B88

_03800B74:
    mov r4, #9
    b _03800B88

_03800B7C:
    mov r4, #8
    b _03800B88

_03800B84:
    mov r4, #4

_03800B88:
    and r0, r0, #0xff
    orr r0, r0, #0x80
    mov r0, r0, lsl #8
    orr r2, r0, #0x3000000
    and r0, r1, #0xff
    orr r6, r2, r0
    mov r5, #0

_03800BA4:
    mov r0, r4
    mov r1, r6
    mov r2, r5
    bl PXI_SendWordByFifo
    cmp r0, #0
    blt _03800BA4
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}

BOOL SPIi_CheckException(u32 dummy)
{
    return spiWork.unk_0 == 0;
}

void SPIi_GetException(u32 param1)
{
    spiWork.unk_0 = 1;
    spiWork.unk_4 = param1;
}

void SPIi_ReleaseException(u32 param1)
{
    if (spiWork.unk_4 == param1) {
        spiWork.unk_4 = 5;
        spiWork.unk_0 = 0;
        OS_WakeupThread(&spiWork.unk_490);
    }
}

asm BOOL SPIi_SetEntry(u32 param1, u32 param2, u16 param3, ...)
{
    stmdb sp!, {r0, r1, r2, r3}
    stmdb sp!, {r4, r5, r6, lr}
    ldrh r2, [sp, #0x18]
    mov r5, r0
    cmp r2, #4
    mov r4, r1
    movhi r0, #0
    bhi _03800CF4
    bl OS_DisableInterrupts
    ldr r3, =spiWork - 4
    add r2, sp, #0x18
    ldr r12, [r3, #0x490]
    mov r1, #0x18
    mul r6, r12, r1
    ldr lr, =spiWork.unk_30c // accessing unk_0 of inner structure
    ldr r12, =spiWork.unk_30c + 4 // accessing unk_4 of inner structure
    str r5, [lr, r6]
    ldr r5, [r3, #0x490]
    bic r2, r2, #3
    mul lr, r5, r1
    str r4, [r12, lr]
    add r6, r2, #4
    ldrh r1, [sp, #0x18]
    ldr r5, =spiWork.unk_0
    mov lr, #0
    mov r2, #0x18
    b _03800CBC

_03800CA0:
    ldr r12, [r3, #0x490]
    add r6, r6, #4
    mla r4, r12, r2, r5
    add r4, r4, lr, lsl #2
    ldr r12, [r6, #-4]
    add lr, lr, #1
    str r12, [r4, #0x314]

_03800CBC:
    cmp lr, r1
    blt _03800CA0
    ldr r1, =spiWork - 4
    ldr r4, [r1, #0x490]
    add r2, r4, #1
    and r2, r2, #0xf
    str r2, [r1, #0x490]
    bl OS_RestoreInterrupts
    ldr r1, =spiWork.unk_30c
    mov r0, #0x18
    mla r1, r4, r0, r1
    ldr r0, =spiWork.unk_2ac
    mov r2, #0
    bl OS_SendMessage

_03800CF4:
    ldmia sp!, {r4, r5, r6, lr}
    add sp, sp, #0x10
    bx lr
}

asm BOOL SPIi_CheckEntry(void)
{
    stmdb sp!, {r3, lr}
    ldr r0, =spiWork.unk_2ac
    add r1, sp, #0
    mov r2, #0
    bl OS_ReadMessage
    ldmia sp!, {r3, lr}
    bx lr
}

static asm void SpiCommonThread(void *arg)
{
    stmdb sp!, {r3, r4, r5, r6, lr}
    sub sp, sp, #4
    ldr r6, =spiWork.unk_2ac
    add r5, sp, #0
    mov r4, #1

_03800D48:
    mov r0, r6
    mov r1, r5
    mov r2, r4
    bl OS_ReceiveMessage
    ldr r0, [sp]
    ldr r1, [r0]
    cmp r1, #3

    addls pc, pc, r1, lsl #2
    b _03800D48
    b _03800D7C
    b _03800D94
    b _03800D84
    b _03800D8C

_03800D7C:
    bl TP_ExecuteProcess
    b _03800D48

_03800D84:
    bl MIC_ExecuteProcess
    b _03800D48

_03800D8C:
    bl PM_ExecuteProcess
    b _03800D48

_03800D94:
    bl NVRAM_ExecuteProcess
    b _03800D48
}

static asm void SpiPxiCallback(PXIFifoTag tag, u32 data, BOOL err)
{
    stmdb sp!, {r3, lr}
    cmp r2, #0
    bne _03800E00
    sub r0, r0, #4
    cmp r0, #5

    addls pc, pc, r0, lsl #2
    b _03800E00
    b _03800DF8
    b _03800E00
    b _03800DD4
    b _03800E00
    b _03800DEC
    b _03800DE0

_03800DD4:
    mov r0, r1
    bl TP_AnalyzeCommand
    b _03800E00

_03800DE0:
    mov r0, r1
    bl MIC_AnalyzeCommand
    b _03800E00

_03800DEC:
    mov r0, r1
    bl PM_AnalyzeCommand
    b _03800E00

_03800DF8:
    mov r0, r1
    bl NVRAM_AnalyzeCommand

_03800E00:
    ldmia sp!, {r3, lr}
    bx lr
}
