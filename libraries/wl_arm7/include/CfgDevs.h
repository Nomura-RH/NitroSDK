#ifndef __CFGDEVS_H_
#define __CFGDEVS_H_

#if (EEPROM_TYPE == AT93C46)
#define ACC_CFG_ROM_ADRS  (6 * SRLDEV_CFG_ADRSBITCNT)
#define ACC_CFG_ROM_ERASE (10 * SRLDEV_CFG_ERASE_PERIOD)
#elif (EEPROM_TYPE == BR93L66)
#define ACC_CFG_ROM_ADRS  (8 * SRLDEV_CFG_ADRSBITCNT)
#define ACC_CFG_ROM_ERASE (5 * SRLDEV_CFG_ERASE_PERIOD)
#endif
#define ACC_CFG_ROM_DATA (SRLDEV_CFG_DATA16BIT)

#define AccConfigROM(_adrsBitCnt_, _dataBitCnt_, _eraseWait_) \
    *(vu16 *)MREG_ROM_CFG = (u16)(_adrsBitCnt_ | _dataBitCnt_ | _eraseWait_)

#define ACC_CFG_BBP_DATA (SDV_RF_CLK_4M | SDV_RF_ADD_CLK)
#define ACC_CFG_RF_DATA \
    (SDV_RF_CLK_4M | ((SDV_RF_ADRS_CNT + SDV_RF_DATA_CNT) * SDV_RF_BITCOUNT))

#define MAX2822_CFG_ADRS 0x00
#define MAX2822_EN_ADRS  0x01
#define MAX2822_RF_ADRS  0x02
#define MAX2822_CF_ADRS  0x03
#define MAX2822_RX_ADRS  0x04
#define MAX2822_TX_ADRS  0x05

#endif // __MAC_H_
