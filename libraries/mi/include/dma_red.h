#ifndef NITRO_MI_DMA_RED_H_
#define NITRO_MI_DMA_RED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nitro/misc.h>
#include <nitro/types.h>
#include <nitro/memorymap.h>
#include <nitro/os/systemCall.h>
#include <nitro/mi/dma.h>

#ifndef SIMULATOR
    #define __MI_DmaSet(dmaNo, srcp, destp, dmaCntData)                        \
        {                                                                      \
            vu32 * dmaCntp = &((vu32 *)REG_DMA0SAD_ADDR)[dmaNo * 3];           \
            dmaCntp[0] = (vu32)(srcp);                                         \
            dmaCntp[1] = (vu32)(destp);                                        \
            dmaCntp[2] = (vu32)(dmaCntData);                                   \
            {u32 dummy = dmaCntp[2];}                                          \
            {u32 dummy = dmaCntp[2];}                                          \
        }

    #define __MI_DmaSetAsync(dmaNo, srcp, destp, dmaCntData)                   \
        {                                                                      \
            vu32 * dmaCntp = &((vu32 *)REG_DMA0SAD_ADDR)[dmaNo * 3];           \
            dmaCntp[0] = (vu32)(srcp);                                         \
            dmaCntp[1] = (vu32)(destp);                                        \
            dmaCntp[2] = (vu32)(dmaCntData);                                   \
        }
#else
    #define __MI_DmaSet(dmaNo, srcp, destp, dmaCntData)                        \
        {                                                                      \
            int i;                                                             \
            for (i = 0; i < (dmaCntData & 0x1ffff); i++)                       \
            if ((dmaCntData) & MI_DMA_SRC_FIX) {                               \
                if ((dmaCntData) & MI_DMA_32BIT_BUS)                           \
                ((vu32 *)(destp))[i] = ((vu32 *)(srcp))[0];                    \
                else    ((vu16 *)(destp))[i] = ((vu16 *)(srcp))[0];            \
            } else {                                                           \
                if ((dmaCntData) & MI_DMA_32BIT_BUS)                           \
                ((vu32 *)(destp))[i] = ((vu32 *)(srcp))[i];                    \
                else    ((vu16 *)(destp))[i] = ((vu16 *)(srcp))[i];            \
            }                                                                  \
        }

    #define __MI_DmaSetAsync(dmaNo, srcp, destp, dmaCntData)                   \
        __MI_DmaSet(dmaNo, srcp, destp, dmaCntData)
#endif

#define __MI_DmaClear(dmaNo, data, destp, size, bit)                           \
    {                                                                          \
        *(vu ## bit *)HW_DMA_CLEAR_DATA_BUF = (vu ## bit)(data);               \
        __MI_DmaSet(dmaNo, HW_DMA_CLEAR_DATA_BUF, destp, (                     \
                        MI_DMA_ENABLE | MI_DMA_TIMING_IMM |                    \
                        MI_DMA_SRC_FIX | MI_DMA_DEST_INC |                     \
                        MI_DMA_ ## bit ## BIT_BUS | ((size) / (bit / 8))));    \
    }

#define __MI_DmaClearIf(dmaNo, data, destp, size, bit)                         \
    {                                                                          \
        *(vu ## bit *)DMA_CLEAR_DATA_BUF = (vu ## bit)(data);                  \
        __MI_DmaSet(dmaNo, DMA_CLEAR_DATA_BUF, destp, (                        \
                        MI_DMA_ENABLE | MI_DMA_TIMING_IMM |                    \
                        MI_DMA_IF_ENABLE |                                     \
                        MI_DMA_SRC_FIX | MI_DMA_DEST_INC |                     \
                        MI_DMA_ ## bit ## BIT_BUS | ((size) / (bit / 8))));    \
    }

#define __MI_DmaClearArray(dmaNo, data, destp, bit)                            \
    __MI_DmaClear(dmaNo, data, destp, sizeof(destp), bit)

#define MI_DmaClearArrayIf(dmaNo, data, destp, bit)                            \
    __MI_DmaClearIf(dmaNo, data, destp, sizeof(destp), bit)

#define __MI_DmaCopy(dmaNo, srcp, destp, size, bit)                            \
    __MI_DmaSet(dmaNo, srcp, destp, (                                          \
                    MI_DMA_ENABLE | MI_DMA_TIMING_IMM |                        \
                    MI_DMA_SRC_INC | MI_DMA_DEST_INC |                         \
                    MI_DMA_ ## bit ## BIT_BUS | ((size) / ((bit) / 8))))

#define __MI_DmaCopyIf(dmaNo, srcp, destp, size, bit)                          \
    __MI_DmaSet(dmaNo, srcp, destp, (                                          \
                    MI_DMA_ENABLE | MI_DMA_TIMING_IMM |                        \
                    MI_DMA_IF_ENABLE |                                         \
                    MI_DMA_SRC_INC | MI_DMA_DEST_INC |                         \
                    MI_DMA_ ## bit ## BIT_BUS | ((size) / (bit / 8))))

#define __MI_DmaCopyArray(dmaNo, srcp, destp, bit)                             \
    __MI_DmaCopy(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_DmaCopyArrayIf(dmaNo, srcp, destp, bit)                           \
    __MI_DmaCopyIf(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_H_DmaCopy(dmaNo, srcp, destp, size, bit)                          \
    __MI_DmaSet(dmaNo, srcp, destp, (                                          \
                    MI_DMA_ENABLE | MI_DMA_TIMING_H_BLANK |                    \
                    MI_DMA_SRC_INC | MI_DMA_DEST_RELOAD |                      \
                    MI_DMA_CONTINUOUS_ON |                                     \
                    MI_DMA_ ## bit ## BIT_BUS | ((size) / ((bit) / 8))))

#define __MI_H_DmaCopyIf(dmaNo, srcp, destp, size, bit)                        \
    __MI_DmaSet(dmaNo, srcp, destp, (                                          \
                    MI_DMA_ENABLE | MI_DMA_TIMING_H_BLANK |                    \
                    MI_DMA_IF_ENABLE |                                         \
                    MI_DMA_SRC_INC | MI_DMA_DEST_RELOAD |                      \
                    MI_DMA_CONTINUOUS_ON |                                     \
                    MI_DMA_ ## bit ## BIT_BUS | ((size) / (bit / 8))))

#define __MI_H_DmaCopyArray(dmaNo, srcp, destp, bit)                           \
    __MI_H_DmaCopy(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_H_DmaCopyArrayIf(dmaNo, srcp, destp, bit)                         \
    __MI_H_DmaCopyIf(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_V_DmaCopy(dmaNo, srcp, destp, size, bit)                          \
    __MI_DmaSet(dmaNo, srcp, destp, (                                          \
                    MI_DMA_ENABLE | MI_DMA_TIMING_V_BLANK |                    \
                    MI_DMA_SRC_INC | MI_DMA_DEST_INC |                         \
                    MI_DMA_ ## bit ## BIT_BUS | ((size) / (bit / 8))))

#define __MI_V_DmaCopyIf(dmaNo, srcp, destp, size, bit)                        \
    __MI_DmaSet(dmaNo, srcp, destp, (                                          \
                    MI_DMA_ENABLE | MI_DMA_TIMING_V_BLANK |                    \
                    MI_DMA_IF_ENABLE |                                         \
                    MI_DMA_SRC_INC | MI_DMA_DEST_INC |                         \
                    MI_DMA_ ## bit ## BIT_BUS | ((size) / (bit / 8))))

#define __MI_V_DmaCopyArray(dmaNo, srcp, destp, bit)                           \
    __MI_V_DmaCopy(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_V_DmaCopyArrayIf(dmaNo, srcp, destp, bit)                         \
    __MI_V_DmaCopyIf(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_DmaDispMainmem(dmaNo, srcp)                                       \
    __MI_DmaSet(dmaNo, srcp, REG_DISP_MMEM_FIFO_ADDR, (                        \
                    MI_DMA_ENABLE | MI_DMA_TIMING_DISP_MMEM |                  \
                    MI_DMA_SRC_INC | MI_DMA_DEST_FIX |                         \
                    MI_DMA_CONTINUOUS_ON |                                     \
                    MI_DMA_32BIT_BUS | (4)))

#define __MI_GX_Dma(dmaNo, srcp, length)                                       \
    __MI_DmaSetAsync(dmaNo, srcp, REG_GXFIFO_ADDR, (                           \
                         MI_DMA_ENABLE | MI_DMA_TIMING_GXFIFO |                \
                         MI_DMA_SRC_INC | MI_DMA_DEST_FIX |                    \
                         MI_DMA_32BIT_BUS | (length)))

#define __MI_GX_DmaIf(dmaNo, srcp, length)                                     \
    __MI_DmaSetAsync(dmaNo, srcp, REG_GXFIFO_ADDR  (                           \
                         MI_DMA_ENABLE | MI_DMA_TIMING_GXFIFO |                \
                         MI_DMA_IF_ENABLE |                                    \
                         MI_DMA_SRC_INC | MI_DMA_DEST_FIX |                    \
                         MI_DMA_32BIT_BUS | (length)))

#define __MI_GX_DmaFast(dmaNo, srcp, length)                                   \
    __MI_DmaSetAsync(dmaNo, srcp, REG_GXFIFO_ADDR, (                           \
                         MI_DMA_ENABLE | MI_DMA_TIMING_IMM |                   \
                         MI_DMA_SRC_INC | MI_DMA_DEST_FIX |                    \
                         MI_DMA_32BIT_BUS | (length)))

#define __MI_GX_DmaFastIf(dmaNo, srcp, length)                                 \
    __MI_DmaSetAsync(dmaNo, srcp, REG_GXFIFO_ADDR, (                           \
                         DMA_ENABLE | DMA_TIMING_IMM |                         \
                         DMA_IF_ENABLE |                                       \
                         DMA_SRC_INC | DMA_DEST_FIX |                          \
                         DMA_32BIT_BUS | (length)))

#define __MI_GX_DmaArray(dmaNo, srcp, destp, bit)                              \
    __MI_GX_Dma(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_GX_DmaArrayIf(dmaNo, srcp, destp, bit)                            \
    __MI_GX_DmaIf(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_GX_DmaArrayFast(dmaNo, srcp, destp, bit)                          \
    __MI_GX_DmaFast(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_GX_DmaArrayFastIf(dmaNo, srcp, destp, bit)                        \
    __MI_GX_DmaFastIf(dmaNo, srcp, destp, sizeof(srcp), bit)

#define __MI_WaitDma(dmaNo)                                                    \
    {                                                                          \
        vu32 * (dmaCntp) = &((vu32 *)REG_DMA0SAD_ADDR)[dmaNo * 3];             \
        while (dmaCntp[2] & MI_DMA_ENABLE);                                    \
    }

#define __MI_StopDma(dmaNo)                                                    \
    {                                                                          \
        vu16 * dmaCntp = &((vu16 *)REG_DMA0SAD_ADDR)[dmaNo * 6];               \
        dmaCntp[5] &= ~((MI_DMA_TIMING_MASK | MI_DMA_CONTINUOUS_ON)            \
                        >> 16);                                                \
        dmaCntp[5] &= ~(MI_DMA_ENABLE >> 16);                                  \
        {u32 dummy = dmaCntp[5];}                                              \
        {u32 dummy = dmaCntp[5];}                                              \
    }

#define __MI_CpuClear(data, destp, size, bit)    UTL_CpuClear ## bit(data, (void *)(destp), size)

#define __MI_CpuClearArray(data, destp, bit)                                   \
    __MI_CpuClear(data, destp, sizeof(destp), bit)

#define __MI_CpuCopy(srcp, destp, size, bit)    UTL_CpuCopy ## bit((void *)(srcp), (void *)(destp), size)

#define __MI_CpuCopyArray(srcp, destp, bit)                                    \
    __MI_CpuCopy(srcp, destp, sizeof(srcp), bit)

#define __MI_CpuClearFast(data, destp, size)  UTL_CpuClearFast(data, (void *)(destp), size)

#define MI_CpuClearArrayFast(data, destp)                                      \
    __MI_CpuClearFast(data, destp, sizeof(destp))

#define __MI_CpuCopyFast(srcp, destp, size)   UTL_CpuCopyFast((void *)(srcp), (void *)(destp), size)

#define MI_CpuCopyArrayFast(srcp, destp)                                       \
    __MI_CpuCopyFast(srcp, destp, sizeof(srcp))

#ifdef __cplusplus
}
#endif

#endif
