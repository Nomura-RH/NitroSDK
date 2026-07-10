#ifndef __FLASH_H_
#define __FLASH_H_

#define FLASH_ADRS(_field_) (u32)(&((LPFLASH_TABLE)WL_FLASH_STR_ADRS)->_field_)

#define FLASH_SIZE(_field_) sizeof(((LPFLASH_TABLE)WL_FLASH_STR_ADRS)->_field_)

#define WL_FLASH_STR_ADRS 0x002a

#define FLASH_PAGE_SIZE   256
#define WL_FLASH_MAX_SIZE (0x200 - WL_FLASH_STR_ADRS)
#define WL_FLASH_MIN_SIZE \
    (sizeof(FLASH_TABLE) - FLASH_SIZE(rfInit_channelDependData))

typedef struct {
    u8 init[2 * 5];
    u16 ch_Reg[14];
    u8 ch_PwrBB[14];
} MAX2822_TABLE, *LPMAX2822_TABLE;

typedef struct {
    u8 init[3 * 12];
    u8 ch_Reg[14][6];
    u8 ch_PwrBB[14];
    u8 ch_PwrRF[14];
} RF2958_TABLE, *LPRF2958_TABLE;

typedef struct {
    u8 init[27];
    u8 ch_BbpCnt;
    u8 ch_BB[2][15];
    u8 ch_RF[3][15];
} MM3156_TABLE, *LPMM3156_TABLE;

typedef struct {
    u8 init[29];
    u8 ch_BbpCnt;
    u8 ch_BB[2][15];
    u8 ch_RF[2][15];
} MM3156ES3_TABLE, *LPMM3156ES3_TABLE;

typedef struct {
    u8 init[RF_MM3156CS1_INIT_NUM];
    u8 ch_BbpCnt;
    u8 ch_BB[2][15];
    u8 ch_RF[RF_MM3156CS1_CHAN_NUM][15];
} MM3156CS1_TABLE, *LPMM3156CS1_TABLE;

typedef struct {
    u8 init[RF_MM3156CS2_INIT_NUM];
    u8 ch_BbpCnt;
    u8 ch_BB[2][15];
    u8 ch_RF[RF_MM3156CS2_CHAN_NUM][15];
} MM3156CS2_TABLE, *LPMM3156CS2_TABLE;

typedef struct {
    u8 init[41];
    u8 ch_BbpCnt;
    u8 ch_BB[2][15];
    u8 ch_RF[3][15];
} MM3218_TABLE, *LPMM3218_TABLE;

typedef struct {
    u8 init[41];
    u8 ch_BbpCnt;
    u8 ch_BB[2][15];
    u8 ch_RF[3][15];
} MM3218CS2_TABLE, *LPMM3218CS2_TABLE;

typedef struct {
    u16 checkSum;
    u16 tableSize;

    u8 vendorID;
    u8 moduleID;
    u8 serialNum[5];
    u8 rsv;

    u16 macAdrs[3];
    u16 enableChannel;

    u16 wlOperation;

    u8 rfDeviceId;
    u8 rfBitWidth;
    u8 rfInitRegNum;
    u8 rfDepChRegNum;

    u16 macTxRxRegs[16];
    u8 bbpInitRegs[105];
    u8 pad;

    union {
        MAX2822_TABLE MAX2822Tbl;
        RF2958_TABLE RF2958Tbl;
        MM3156_TABLE MM3156Tbl;
        MM3156ES3_TABLE MM3156ES3Tbl;
        MM3156CS1_TABLE MM3156CS1Tbl;
        MM3156CS2_TABLE MM3156CS2Tbl;
        MM3218_TABLE MM3218Tbl;
        MM3218CS2_TABLE MM3218CS2Tbl;
    } rfInit_channelDependData;
} FLASH_TABLE, *LPFLASH_TABLE;

u32 FLASH_MakeImage(void);
u32 FLASH_VerifyCheckSum(u32 *pCrc);
void FLASH_Wait(void);
void FLASH_Read(u32 adrs, u32 size, u8 *pBuf);
void FLASH_DirectRead(u32 adrs, u32 size, u8 *pBuf);

#ifndef ONLY_WL
void FLASH_Write(u32 adrs, u32 size, u8 *pBuf);
#endif

#endif // __FLASH_H_
