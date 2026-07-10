#ifndef __INCLUDEWL_H_
#define __INCLUDEWL_H_

#include "WlFrame.h"

void APP_BBP_Write(u32 adrs, u32 data);
u32 APP_BBP_Read(u32 adrs);
void APP_RF_Write(u32 adrs, u32 data);
u8 APP_GetWlStatus(void);
void APP_SetWlStatus(u8 sts);
void APP_SetWlStatusScmd(u8 sts);
WlCounter *App_GetWlCounter(void);
void App_WInitCounter(void);
u8 App_GetCurChannel(void);
void App_DMA_Write(void *destAdrs, void *srcAdrs, u32 length);
void App_DMA_Read(void *destAdrs, void *srcAdrs, u32 length);
u32 APP_GetRfId(void);
u16 APP_CalcBbpCRC(void);
void App_DbgWlSetDDO(u32 level, u32 data);
void App_DbgWlClrDDO(u32 level, u32 data);

u32 EEPROM_Read(u32 adrs);
void EEPROM_WriteEnable(void);
void EEPROM_Write(u32 adrs, u32 data);
void EEPROM_WriteAll(u32 data);
void EEPROM_WriteDisable(void);
void EEPROM_Erase(u32 adrs);
void EEPROM_EraseAll(void);

void EEPROM_WriteU8(u32 adrs, u8 value);
void EEPROM_WriteU16(u32 adrs, u16 value);
void EEPROM_WriteU32(u32 adrs, u32 value);
u8 EEPROM_ReadU8(u32 adrs);
u16 EEPROM_ReadU16(u32 adrs);
u32 EEPROM_ReadU32(u32 adrs);

u32 EEPROM_InitBbpRegs(void);
u32 EEPROM_InitTxPowerRegs(void);
u32 EEPROM_InitRfRegs(u32 id);
u32 EEPROM_InitFixValue(void);
u32 EEPROM_InitWmParam(void);
u32 EEPROM_InitAppParam(void);
u32 EEPROM_InitAllParam(u32 rfid);
u32 EEPROM_InitIrisParam(void);

u16 HostSetChannel(u16 channel);

#if (EEPROM_TYPE == AT93C46)
#define ACC_CFG_ROM_ADRS \
    (6 * SRLDEV_CFG_ADRSBITCNT) // ROM AhXrbg = 6 bits
#define ACC_CFG_ROM_ERASE \
    (10 * SRLDEV_CFG_ERASE_PERIOD) // ROM ÁÔ(10ms)
#elif (EEPROM_TYPE == BR93L66)
#define ACC_CFG_ROM_ADRS \
    (8 * SRLDEV_CFG_ADRSBITCNT) // ROM AhXrbg = 8 bits
#define ACC_CFG_ROM_ERASE \
    (5 * SRLDEV_CFG_ERASE_PERIOD) // ROM ÁÔ(5ms)
#endif
#define ACC_CFG_ROM_DATA \
    (SRLDEV_CFG_DATA16BIT) // ROM f[^rbg   = 16 bits

#define AccConfigROM(_adrsBitCnt_, _dataBitCnt_, _eraseWait_) \
    *(vu16 *)MREG_ROM_CFG = (u16)(_adrsBitCnt_ | _dataBitCnt_ | _eraseWait_)

#endif // __INCLUDEWL_H_
