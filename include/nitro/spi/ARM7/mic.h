#ifndef NITRO_MIC_H_
#define NITRO_MIC_H_

#include <nitro/os/common/interrupt.h>

#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SDK_TEG
#define MIC_USE_INNER_REFERENCE 1
#define MIC_COMMAND_SAMPLING_8  0x00EE
#define MIC_COMMAND_SAMPLING_12 0x00E6
#else
#if (SDK_TS_VERSION >= 100)
#undef MIC_USE_INNER_REFERENCE
#define MIC_COMMAND_SAMPLING_8  0x00EC
#define MIC_COMMAND_SAMPLING_12 0x00E4
#else
#define MIC_USE_INNER_REFERENCE 1
#define MIC_COMMAND_SAMPLING_8  0x00EE
#define MIC_COMMAND_SAMPLING_12 0x00E6
#endif
#endif

#define MIC_COMMAND_POWER_DOWN 0x84

#define MIC_S8_VALID_BIT_MASK  0x7F80
#define MIC_S8_VALID_BIT_SHIFT 7

#define MIC_S12_VALID_BIT_MASK    0x7FF8
#define MIC_S12_VALID_BIT_SHIFT   3
#define MIC_S12_VALID_BIT_L_SHIFT 1

#define reg_MIC_TMCNT_L reg_OS_TM3CNT_L
#define reg_MIC_TMCNT_H reg_OS_TM3CNT_H

#define MIC_TMCNT_L_REG_OFFSET REG_TM3CNT_L_OFFSET
#define MIC_TMCNT_H_REG_OFFSET REG_TM3CNT_H_OFFSET

#define MIC_TMCNT_H_E_SHIFT REG_OS_TM3CNT_H_E_SHIFT
#define MIC_TMCNT_H_E_SIZE  REG_OS_TM3CNT_H_E_SIZE
#define MIC_TMCNT_H_E_MASK  REG_OS_TM3CNT_H_E_MASK

#define MIC_TMCNT_H_I_SHIFT REG_OS_TM3CNT_H_I_SHIFT
#define MIC_TMCNT_H_I_SIZE  REG_OS_TM3CNT_H_I_SIZE
#define MIC_TMCNT_H_I_MASK  REG_OS_TM3CNT_H_I_MASK

#define MIC_TMCNT_H_CH_SHIFT REG_OS_TM3CNT_H_CH_SHIFT
#define MIC_TMCNT_H_CH_SIZE  REG_OS_TM3CNT_H_CH_SIZE
#define MIC_TMCNT_H_CH_MASK  REG_OS_TM3CNT_H_CH_MASK

#define MIC_TMCNT_H_PS_SHIFT REG_OS_TM3CNT_H_PS_SHIFT
#define MIC_TMCNT_H_PS_SIZE  REG_OS_TM3CNT_H_PS_SIZE
#define MIC_TMCNT_H_PS_MASK  REG_OS_TM3CNT_H_PS_MASK

#define MIC_IE_TIMER OS_IE_TIMER3

#define MIC_MULTI_INTR_STACK_SIZE 0x100

#define MIC_INTRINFO_OFFSET_COUNT 0
#define MIC_INTRINFO_OFFSET_SP    4
#define MIC_INTRINFO_OFFSET_IE    8

typedef enum {
    MIC_STATUS_READY = 0,
    MIC_STATUS_AUTO_START,
    MIC_STATUS_AUTO_SAMPLING,
    MIC_STATUS_AUTO_END,
    MIC_STATUS_END_WAIT,
} MICStatus;

typedef struct MICWork {
    u16 command[SPI_PXI_CONTINUOUS_PACKET_MAX];
    MICStatus status;
    u16 type;
    u16 admode;
    void *buf;
    u32 index;
    u32 size;
    u16 timerValue;
    u16 timerPrescaler;
    u16 temp16;
    u16 temporary;
} MICWork;

typedef struct MICIntrInfo {
    u32 count;
    u32 sp;
    u32 ie;
    u32 handler;
} MICIntrInfo;

static inline void MIC_SPIChangeMode(SPITransMode continuous)
{
    reg_SPI_SPICNT = (u16)((1 << REG_SPI_SPICNT_E_SHIFT)
        | (0 << REG_SPI_SPICNT_I_SHIFT)
        | (SPI_COMMPARTNER_TP << REG_SPI_SPICNT_SEL_SHIFT)
        | (continuous << REG_SPI_SPICNT_MODE_SHIFT)
        | (0 << REG_SPI_SPICNT_BUSY_SHIFT)
        | (SPI_BAUDRATE_2MHZ << REG_SPI_SPICNT_BAUDRATE_SHIFT));
}

void MIC_Init(void);
void MIC_AnalyzeCommand(u32 data);
void MIC_ExecuteProcess(SPIEntry *entry);
u16 MIC_ExecSampling8(void);
u16 MIC_ExecSampling12(void);
u16 MIC_OneTimeSampling8(void);
u16 MIC_OneTimeSampling12(void);
void MIC_SetIrqFunction(OSIrqMask intrBit, OSIrqFunction function);
void MIC_EnableMultipleInterrupt(void);
void MIC_DisableMultipleInterrupt(void);
void MIC_IrqHandler(void);

#ifdef __cplusplus
}
#endif

#endif
