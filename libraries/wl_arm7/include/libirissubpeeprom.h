#ifndef __IRIS_EEPROM_H__
#define __IRIS_EEPROM_H__

#define EEPROM_ERR        0x8000
#define EEPROM_PARAM_ERR  (EEPROM_ERR | 0x00FF)
#define EEPROM_CRC_ERR    (EEPROM_ERR | 0x0008)
#define EEPROM_VERIFY_ERR (EEPROM_ERR | 0x0000)
/*
typedef struct iris_eeprom_tag {
    u32	size;
    u16	pageSize;
    u32 usrAdr;
    u32 usrSize;
    u32 sysAdr;
    u16	freq;
    u16	adrComm;
} eepromType;
*/
u16 IrisIdentifyEeprom(void);

u16 IrisReadEeprom(u16 *eep_src, u16 *dst, u32 hw_size);

u16 IrisProgramEeprom(u16 *src, u16 *eep_dst, u32 hw_size);

u16 IrisVerifyEeprom(u16 *eep, u16 *dat);

u16 IrisProgramEepromEx(u16 *src, u16 *dst);

#endif /*__IRIS_EEPROM_H__*/
