#ifndef NITRO_PM_H_
#define NITRO_PM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nitro/misc.h>
#include <nitro/types.h>
#include <nitro/spi/common/pm_common.h>
#include <nitro/spi/common/type.h>
#include <nitro/pxi/common/fifo.h>

typedef struct PMiWork {
    u16 unk_0[0x10];
    u32 unk_20;
    u32 unk_24;
    u32 unk_28;
} PMiWork;

typedef struct PMiBlinkPatternData {
    u32 unk_0;
    u32 unk_4;
    u16 unk_8;
    u16 unk_a;
} PMiBlinkPatternData;

extern u16 PMi_KeyPattern;
extern u16 PMi_TriggerBL;
extern BOOL PMi_Initialized;
extern PMiWork PMi_Work;
extern PMLEDPattern PMi_BlinkPatternNo;

extern PMLEDStatus PMi_LEDStatus;
extern PMiBlinkPatternData PMi_BlinkPatternData[];

void PM_SetLEDPattern(PMLEDPattern pattern);
PMLEDPattern PM_GetLEDPattern(void);
u32 PMi_SetLED(PMLEDStatus status);
void PM_Init(void);
void PM_ExecuteProcess(SPIMessage *param1);
void PM_AnalyzeCommand(u32 data);

// the widths of these types might not be accurate...
u8 PMi_GetRegister(u8 param1);
void PMi_SetRegister(u16 param1, u32 param2);
void PMi_ResetControl(u8 ctrl);
void PMi_SetControl(u8 ctrl);
u16 PMi_DoSleep(void);
void PMi_SwitchUtilityProc(u32 param1);
void PMi_SendPxiCommand(u32 param1, u32 param2, u16 param3);
void PM_SelfBlinkProc(void);

#ifdef __cplusplus
}
#endif

#endif
