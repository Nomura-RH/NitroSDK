#ifndef NITRO_NVRAM_H_
#define NITRO_NVRAM_H_

#ifdef __cplusplus
extern "C" {
#endif

void NVRAM_Init(void);
BOOL NVRAM_ReadDataBytes(u32 address, u32 size, u8 *pData);
void NVRAM_ExecuteProcess(SPIMessage *param1);
void NVRAM_AnalyzeCommand(u32 data);
void NVRAM_WriteEnable(void);
void NVRAM_WriteDisable(void);
void NVRAM_ReadStatusRegister(u8 *param1);
void NVRAM_ReadDataBytesAtHigherSpeed(u32 param1, u32 param2, u32 param3);
void NVRAM_PageWrite(u32 param1, u16 param2, u32 param3);
void NVRAM_PageProgram(u32 param1, u16 param2, u32 param3);
void NVRAM_PageErase(u32 param1);
void NVRAM_SectorErase(u32 param1);
void NVRAM_DeepPowerDown(u32 param1);
void NVRAM_ReleaseFromDeepPowerDown(u32 param1);
void NVRAM_ChipErase(u32 param1);
void NVRAM_ReadSiliconId(u8 *param1);
void NVRAM_SoftwareReset(void);

#ifdef __cplusplus
}
#endif

#endif
