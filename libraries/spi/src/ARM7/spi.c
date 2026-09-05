#include "nitro/spi/ARM7/spi.h"

#include <nitro/mi/memory.h>
#include <nitro/os/common/message.h>
#include <nitro/os/common/thread.h>
#include <nitro/pxi/common/fifo.h>
#include <nitro/spi/common/type.h>

static u16 spiInitialized = FALSE;
static SPIWork spiWork;

static void SpiCommonThread(void *arg);
static void SpiPxiCallback(PXIFifoTag tag, u32 data, BOOL err);

void SPI_Init(u32 prio)
{
    if (spiInitialized) {
        return;
    }
    spiInitialized = TRUE;

    spiWork.exception = FALSE;
    spiWork.type = SPI_DEVICE_TYPE_MAX;

    TP_Init();
    NVRAM_Init();
#ifndef SDK_SMALL_BUILD
    MIC_Init();
#endif
#ifndef SDK_TEG
    PM_Init();
#endif

    PXI_Init();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_TOUCHPANEL, SpiPxiCallback);
#ifndef SDK_SMALL_BUILD
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_MIC, SpiPxiCallback);
#endif
#ifndef SDK_TEG
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_PM, SpiPxiCallback);
#endif
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_NVRAM, SpiPxiCallback);

    OS_InitMessageQueue(&spiWork.message, spiWork.msg_buf, SPI_MESSAGE_ARRAY_MAX);

    for (int i = 0; i < SPI_MESSAGE_ARRAY_MAX; i++) {
        MI_CpuFill8(&spiWork.entry[i], 0, sizeof(SPIEntry));
    }

    spiWork.entryIndex = 0;

    OS_InitThreadQueue(&spiWork.lock);
    OS_CreateThread(&spiWork.thread, SpiCommonThread, NULL, (void *)(spiWork.stack + SPI_THREAD_STACK_SIZE / sizeof(u64)), SPI_THREAD_STACK_SIZE, prio);
    OS_WakeupThreadDirect(&spiWork.thread);
}

void SPI_Lock(u32 id)
{
    while (TRUE) {
        OSIntrMode enabled = OS_DisableInterrupts();

        if (spiWork.exception) {
            OS_RestoreInterrupts(enabled);

            OS_SleepThread(&spiWork.lock);

            continue;
        } else {
            SPIi_GetException(SPI_DEVICE_TYPE_ARM7);
            spiWork.lockId = id;

            OS_RestoreInterrupts(enabled);

            break;
        }
    }
}

void SPI_Unlock(u32 id)
{
    if (spiWork.exception && spiWork.type == SPI_DEVICE_TYPE_ARM7 && spiWork.lockId == id) {
        OSIntrMode enabled = OS_DisableInterrupts();

        spiWork.type = SPI_DEVICE_TYPE_MAX;
        spiWork.exception = FALSE;
        spiWork.lockId = 0;

        OS_RestoreInterrupts(enabled);

        OS_WakeupThread(&spiWork.lock);
    }
}

void SPIi_ReturnResult(u16 command, u16 result)
{
    PXIFifoTag tag;

    switch (command & 0x70) {

    case 0x00:
    case 0x10:
        tag = PXI_FIFO_TAG_TOUCHPANEL;
        break;

#ifndef SDK_SMALL_BUILD
    case 0x40:
    case 0x50:
        tag = PXI_FIFO_TAG_MIC;
        break;
#endif

#ifndef SDK_TEG
    case 0x60:
    case 0x70:
        tag = PXI_FIFO_TAG_PM;
        break;
#endif

    case 0x20:
    case 0x30:
        tag = PXI_FIFO_TAG_NVRAM;
        break;
    }

    while (0 > PXI_SendWordByFifo(tag,
               SPI_PXI_START_BIT
                   | SPI_PXI_END_BIT
                   | (0 << SPI_PXI_INDEX_SHIFT)
                   | ((u32)((command & 0xFF) | 0x80) << 8)
                   | (u32)(result & 0xFF),
               0)) {
    }
}

BOOL SPIi_CheckException(SPIDeviceType type)
{
    return !spiWork.exception;
}

void SPIi_GetException(SPIDeviceType type)
{
    spiWork.exception = TRUE;
    spiWork.type = type;
}

void SPIi_ReleaseException(SPIDeviceType type)
{
    if (spiWork.type == type) {
        spiWork.type = SPI_DEVICE_TYPE_MAX;
        spiWork.exception = FALSE;
        OS_WakeupThread(&spiWork.lock);
    }
}

BOOL SPIi_SetEntry(SPIDeviceType type, u32 process, u16 args, ...)
{
    va_list vlist;

    if (args > SPI_ENTRY_ARGS_MAX) {
        return FALSE;
    }

    OSIntrMode e = OS_DisableInterrupts();
    spiWork.entry[spiWork.entryIndex].type = type;
    spiWork.entry[spiWork.entryIndex].process = process;

    va_start(vlist, args);
    for (int i = 0; i < args; i++) {
        spiWork.entry[spiWork.entryIndex].arg[i] = va_arg(vlist, u32);
    }
    va_end(vlist);

    void *w = &spiWork.entry[spiWork.entryIndex];
    spiWork.entryIndex = (spiWork.entryIndex + 1) % SPI_MESSAGE_ARRAY_MAX;
    OS_RestoreInterrupts(e);
    return OS_SendMessage(&spiWork.message, w, OS_MESSAGE_NOBLOCK);
}

BOOL SPIi_CheckEntry(void)
{
    OSMessage msg;
    BOOL result = OS_ReadMessage(&spiWork.message, &msg, OS_MESSAGE_NOBLOCK);
    return result;
}

static void SpiCommonThread(void *arg)
{
    OSMessage msg;

    while (TRUE) {
        OS_ReceiveMessage(&spiWork.message, &msg, OS_MESSAGE_BLOCK);

        SPIEntry *entry = (SPIEntry *)msg;

        switch (entry->type) {
        case SPI_DEVICE_TYPE_TP:
            TP_ExecuteProcess(entry);
            break;

#ifndef SDK_SMALL_BUILD
        case SPI_DEVICE_TYPE_MIC:
            MIC_ExecuteProcess(entry);
            break;
#endif

#ifndef SDK_TEG
        case SPI_DEVICE_TYPE_PM:
            PM_ExecuteProcess(entry);
            break;
#endif

        case SPI_DEVICE_TYPE_NVRAM:
            NVRAM_ExecuteProcess(entry);
            break;
        }
    }
}

static void SpiPxiCallback(PXIFifoTag tag, u32 data, BOOL err)
{
    if (err) {
        return;
    }

    switch (tag) {
    case PXI_FIFO_TAG_TOUCHPANEL:
        TP_AnalyzeCommand(data);
        break;

#ifndef SDK_SMALL_BUILD
    case PXI_FIFO_TAG_MIC:
        MIC_AnalyzeCommand(data);
        break;
#endif

#ifndef SDK_TEG
    case PXI_FIFO_TAG_PM:
        PM_AnalyzeCommand(data);
        break;
#endif

    case PXI_FIFO_TAG_NVRAM:
        NVRAM_AnalyzeCommand(data);
        break;
    }
}
