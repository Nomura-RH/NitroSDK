#ifndef __EEPROM_H_
#define __EEPROM_H_

#include "WmParam.h"

typedef struct {
    WmParam wmParam;

    u32 HostDbgLevel;
    u32 WmDbgLevel;
    u32 WlDbgLevel;
    u32 NetDbgLevel;
    u16 ddoDbgLevel;
    u16 testDbgLevel;
} WmTemp;

typedef struct {
    u32 srcIpAdrs;
    u32 subnetMask;
    u32 gateway;
    u32 destIpAdrs;

    u8 bps;

    u8 rsv[0x1C - 17];
} AppTemp;

typedef struct {
    u8 _10dBm;
    u8 _0dBm;
} TxPwrRegs;

typedef struct {
    u8 bbpRegs[106];

    u8 txPwrRegs[14];

    u8 rfId;
    u8 rfBits;
    u8 rfInitNum_ChanNum;
    u8 rfInitNum;

    u8 rfInit[EEPROM_RFINIT_SIZE];

    u8 rsv4[16];

    u16 irisMacAdrs[3];
    u8 serialNumber[16];
    u16 enableChannel;

    WmTemp wmTemp;

    u16 wlOperation;
    u16 pad;

    AppTemp appTemp;

    u8 reserved0[1];

    u8 rf_version;
    u8 version;
    u8 bbp_version;
} Eeprom;

#define EEPROM_ADRS(_field_)      (u32)(&((Eeprom *)EEPROM_BASE)->_field_)
#define IRIS_EEPROM_ADRS(_field_) (u32)(&((Eeprom *)EEPROM_BASE)->_field_)

u16 MTM_IrisExProgramEeprom(u16 *src, u16 *eep_dst, u32 hw_size);

// #ifdef	ONLY_WL
u16 WLi_IrisReadEeprom(u16 *eep_src, u16 *dst, u32 hw_size);
// #define	IrisReadEeprom		WLi_IrisReadEeprom
// #else
#define IrisReadEeprom MTM_IrisReadEeprom
// #endif

#define IrisProgramEeprom MTM_IrisProgramEeprom

u16 MTM_IrisReadEeprom(u16 *eep_src, u16 *dst, u32 hw_size);
u16 MTM_IrisProgramEeprom(u16 *src, u16 *eep_dst, u32 hw_size);

void InitVirtualEeprom(void);

extern Eeprom eeprom_data;
extern u32 useEeprom;

#endif // __INCLUDEWL_H_
