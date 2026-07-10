#define __SPIEEPROM_C_
#define __INSYSROM__

#include "system.h"

#include "EEPROM.h"

#include "Compati.h"

#define SPICNT_LAST (SPI_ENABLE | SPI_TARGET_EEPROM | SPI_SCK_1M)
#define SPICNT_CONT (SPICNT_LAST | SPI_CONTINUOUS_ON)

#define SPI_EEPROM_READ          0x03
#define SPI_EEPROM_WRITE         0x02
#define SPI_EEPROM_READ_STATUS   0x05
#define SPI_EEPROM_WRITE_STATUS  0x01
#define SPI_EEPROM_WRITE_ENABLE  0x06
#define SPI_EEPROM_WRITE_DISABLE 0x04

#define EEPROM_STS_WIP 0x01
#define EEPROM_STS_WEL 0x02
#define EEPROM_STS_BP0 0x04
#define EEPROM_STS_BP1 0x08

#define EEPROM_ERR 0x8000

void MTM_IrisWaitEeprom(void);
void EEPROM_WriteU8(u32 adrs, u8 value);

Eeprom eeprom_data;
u32 useEeprom;

static const Eeprom c_eeprom_data = {
    {
        0x6D,
        0x9E,
        0x40,
        0x05,
        0x1B,
        0x6C,
        0x48,
        0x80,
        0x38,
        0x00,
        0x35,
        0x07,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xB0,
        0x00,
        0x04,
        0x01,
        0xD8,
        0xFF,
        0xFF,
        0xC7,
        0xBB,
        0x01,
        0xFF,
        0x7F,
        0x5A,
        0x01,
        0x3F,
        0x01,
        0x3F,
        0x36,
        0x36,
        0x00,
        0x78,
        0x28,
        0x55,
        0x08,
        0x28,
        0x16,
        0x00,
        0x01,
        0x0E,
        0x20,
        0x02,
        0x98,
        0x98,
        0x1F,
        0x0A,
        0x08,
        0x04,
        0x01,
        0x00,
        0x00,
        0x00,
        0xFF,
        0xFF,
        0xFE,
        0xFE,
        0xFE,
        0xFE,
        0xFC,
        0xFC,
        0xFA,
        0xFA,
        0xFA,
        0xFA,
        0xFA,
        0xF8,
        0xF8,
        0xF6,
        0xA5,
        0x12,
        0x14,
        0x12,
        0x41,
        0x23,
        0x03,
        0x04,
        0x70,
        0x35,
        0x0E,
        0x16,
        0x16,
        0x00,
        0x00,
        0x06,
        0x01,
        0xFF,
        0xFE,
        0xFF,
        0xFF,
        0x00,
        0x0E,
        0x13,
        0x00,
        0x00,
        0x28,
        0x1C,
    },

    { 36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36,
        36 },

    MDRF2958,
    RF_MDRF2958_BITS,
    RF_MDRF2958_INIT_NUM,
    RF_MDRF2958_CHAN_NUM,

    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

    { DEFAULT_SRCMACADRS0, DEFAULT_SRCMACADRS1, DEFAULT_SRCMACADRS2 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    DEFAULT_ENCH,

    { {
          { DEFAULT_SRCMACADRS0, DEFAULT_SRCMACADRS1, DEFAULT_SRCMACADRS2 },
          { DEFAULT_BSSID0, DEFAULT_BSSID1, DEFAULT_BSSID2 },
          { 0, 0, 0 },

          { DEFAULT_SSID },
          DEFAULT_SSIDMASK,

          DEFAULT_SSIDLENGTH,
          1,
          DEFAULT_AUTOALGO,
          0,

          DEFAULT_MIB,
          WL_CMDLABEL_PMG_PS,
          DEFAULT_WAKEUP,
          DEFAULT_RXDTIM,

          DEFAULT_CHLIST,
          DEFAULT_MAXCHTIME,
          DEFAULT_JOINTIME,
          DEFAULT_AUTHTIME,
          DEFAULT_ASSTIME,

          0,
          DEFAULT_LSNINTV,
          DEFAULT_REASONCODE,

          DEFAULT_MODE,
          DEFAULT_RETRYLIMIT,
          0,
          2,

          { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

          DEFAULT_WEPMODE,
          DEFAULT_WEPKEYID,
          DEFAULT_BEACONTYPE,
          DEFAULT_RESPONSEBCPRBREQ,

          2,
          1,
          1,
          0,

          200,
          10,

          0,
          0,
          0,

          0,
          0,

          { DEFAULT_SRCMACADRS0, DEFAULT_SRCMACADRS1, DEFAULT_SRCMACADRS2 },
          { DEFAULT_DESTMACADRS0, DEFAULT_DESTMACADRS1, DEFAULT_DESTMACADRS2 },

          DEFAULT_BRS,
          DEFAULT_SRS,

          DEFAULT_CCAMODE,
          DEFAULT_EDTHRESHOLD,
          DEFAULT_MEASURETIME,
      },

        HostDbgLevelSet,
        0,
        3,
        0,
        0,
        0 },

    0xFFFF,
    0,

    { IP(172, 16, 40, 150),
        IP(255, 255, 255, 0),
        IP(172, 16, 40, 254),
        IP(172, 16, 40, 150),

        0x00,

        { 0, 0, 0, 0, 0 } },

    { 0 },

    0x02,
    0x03,
    0x03
};

#ifdef SDK_ARM7
void MTM_IrisWaitEeprom(void)
{
    do {
        while (*(vu16 *)REG_SPICNT & SPI_BUSY) {}

        *(vu16 *)REG_SPICNT = SPICNT_CONT;

        *(vu16 *)REG_SPIDATA = SPI_EEPROM_READ_STATUS;
        while (*(vu16 *)REG_SPICNT & SPI_BUSY) {}

        *(vu16 *)REG_SPICNT = SPICNT_LAST;

        *(vu16 *)REG_SPIDATA = 0x00;
        while (*(vu16 *)REG_SPICNT & SPI_BUSY) {}

    } while (*(vu16 *)REG_SPIDATA & EEPROM_STS_WIP);
}
#endif

u16 MTM_IrisReadEeprom(u16 *eep_src, u16 *dst, u32 hw_size)
{
    u8 *pDest = (u8 *)dst;
    u32 adrs = (u32)eep_src;
    u32 r;

    for (; hw_size > 0; hw_size--) {
        r = EEPROM_Read(adrs / 2);
        if (r == 0xFFFFFFFF) {
            return EEPROM_ERR;
        }
        if (adrs & 1) {
            r >>= 8;
        }

        *pDest++ = r;
        adrs++;
    }

    return 0;
}

u16 MTM_IrisProgramEeprom(u16 *src, u16 *eep_dst, u32 hw_size)
{
    u8 *pSrc = (u8 *)src;
    u32 adrs = (u32)eep_dst;

    for (; hw_size > 0; hw_size--) {
        EEPROM_WriteU8(adrs++, *pSrc++);
    }

    return 0;
}

u16 MTM_IrisExProgramEeprom(u16 *src, u16 *eep_dst, u32 hw_size)
{
    u8 *pSrc = (u8 *)src;
    u32 adrs = (u32)eep_dst;

    for (; hw_size > 0; hw_size--) {
        EEPROM_WriteU8(adrs++, *pSrc++);
    }

    return 0;
}

void InitVirtualEeprom(void)
{
    Print("Init virtual eeprom\r\n");
    APP_DmaCopy16((void *)&c_eeprom_data, (void *)&eeprom_data, sizeof(c_eeprom_data));
}
