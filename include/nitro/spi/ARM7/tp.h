#ifndef NITRO_TP_H_
#define NITRO_TP_H_

#include <nitro/spi/common/type.h>

#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SDK_TEG
#define TP_COMMAND_SAMPLING_X 0xD3
#define TP_COMMAND_SAMPLING_Y 0x93
#else
#if (SDK_TS_VERSION >= 100)
#define TP_COMMAND_SAMPLING_X 0xD1
#define TP_COMMAND_SAMPLING_Y 0x91
#else
#define TP_COMMAND_SAMPLING_X 0xD3
#define TP_COMMAND_SAMPLING_Y 0x93
#endif
#endif

#define TP_COMMAND_DETECT_TOUCH 0x84

#define TP_VALID_BIT_MASK  0x7FF8
#define TP_VALID_BIT_SHIFT 3

#define TP_VALARM_DELAY_MAX 10

#define SDK_TP_AUTO_ADJUST_RANGE

typedef enum TPStatus {
    TP_STATUS_READY = 0,
    TP_STATUS_AUTO_START,
    TP_STATUS_AUTO_SAMPLING,
    TP_STATUS_AUTO_WAIT_END
} TPStatus;

typedef struct TPWork {
    u16 command[SPI_PXI_CONTINUOUS_PACKET_MAX];
    TPStatus status;
    s32 range;
    s32 rangeMin;
    OSVAlarm vAlarm[SPI_TP_SAMPLING_FREQUENCY_MAX];
    u16 vCount[SPI_TP_SAMPLING_FREQUENCY_MAX];
} TPWork;

static inline void TP_SPIChangeMode(SPITransMode continuous)
{
    reg_SPI_SPICNT = (u16)((1 << REG_SPI_SPICNT_E_SHIFT)
        | (0 << REG_SPI_SPICNT_I_SHIFT)
        | (SPI_COMMPARTNER_TP << REG_SPI_SPICNT_SEL_SHIFT)
        | (continuous << REG_SPI_SPICNT_MODE_SHIFT)
        | (0 << REG_SPI_SPICNT_BUSY_SHIFT)
        | (SPI_BAUDRATE_2MHZ << REG_SPI_SPICNT_BAUDRATE_SHIFT));
}

void TP_Init(void);
void TP_ExecuteProcess(SPIEntry *entry);
void TP_AnalyzeCommand(u32 data);
#ifdef SDK_TP_AUTO_ADJUST_RANGE
void TP_ExecSampling(SPITpData *data, s32 range, u16 *density);
#else
void TP_ExecSampling(SPITpData *data, s32 range);
#endif

#ifdef __cplusplus
}
#endif

#endif
