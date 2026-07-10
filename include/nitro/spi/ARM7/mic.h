#ifndef NITRO_MIC_H_
#define NITRO_MIC_H_

#ifdef __cplusplus
extern "C" {
#endif

void MIC_Init(void);
void MIC_ExecuteProcess(SPIMessage *param1);
u16 MIC_ExecSampling8(void);
u16 MIC_ExecSampling12(void);
void MIC_AnalyzeCommand(u32 data);
void MIC_SetIrqFunction(u32 param1, void (*param2)(void));
void MIC_EnableMultipleInterrupt(void);
void MIC_DisableMultipleInterrupt(void);
void MIC_IrqHandler(void);

#ifdef __cplusplus
}
#endif

#endif

