#ifndef NITRO_TP_H_
#define NITRO_TP_H_

#include <nitro/spi/common/type.h>

#ifdef __cplusplus
extern "C" {
#endif

void TP_Init(void);
void TP_ExecuteProcess(SPIMessage *param1);
void TP_AnalyzeCommand(u32 data);
void TP_ExecSampling(SPITpData *param1, u32 param2, u16 *param3);

#ifdef __cplusplus
}
#endif

#endif


