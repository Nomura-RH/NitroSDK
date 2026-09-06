#ifndef NITRO_PM_H_
#define NITRO_PM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nitro/misc.h>
#include <nitro/pxi/common/fifo.h>
#include <nitro/spi/common/pm_common.h>
#include <nitro/spi/common/type.h>
#include <nitro/types.h>

#include "spi.h"

typedef enum {
    PM_STATUS_READY = 0,
    PM_STATUS_START_SLEEP,
    PM_STATUS_UTILITY,
    PM_STATUS_READ_REGISTER,
    PM_STATUS_WRITE_REGISTER
} PMStatus;

typedef struct PMWork {
    u16 command[SPI_PXI_CONTINUOUS_PACKET_MAX];
    PMStatus status;
    u32 param;
    u32 regNumber;
} PMWork;

extern BOOL PMi_Initialized;
static inline BOOL PM_IsAvailable(void)
{
    return PMi_Initialized;
}

void PM_Init(void);
void PM_ExecuteProcess(SPIEntry *entry);
void PM_AnalyzeCommand(u32 data);

void PM_SetLEDPattern(PMLEDPattern pattern);
PMLEDPattern PM_GetLEDPattern(void);
void PMi_SetLED(PMLEDStatus status);

u8 PMi_GetRegister(u16 reg);
void PMi_SetRegister(u16 reg, u8 data);
void PMi_ResetControl(u8 ctrl);
void PMi_SetControl(u8 ctrl);
void PMi_DoSleep(void);
void PMi_SwitchUtilityProc(u32 procNumber);
void PMi_SendPxiCommand(u16 command, u16 addr, u16 data);
void PM_SelfBlinkProc(void);

#ifdef __cplusplus
}
#endif

#endif
