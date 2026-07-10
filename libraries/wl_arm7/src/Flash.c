#define __FLASH_C_
#define __INSYSROM__

#include "WlSys.h"
#include "WlLib.h"

#include "nvram_sp.h"
#include "Flash.h"

u32 FLASH_VerifyCheckSum(u32 *pCrc)
{
    u8 *p;
    u32 crc, rcrc, tbsize, c;

    tbsize = ((FLASH_TABLE *)wlMan->pFlashImg)->tableSize;

    if ((tbsize < WL_FLASH_MIN_SIZE) || (tbsize > WL_FLASH_MAX_SIZE)) {
        DbgPrint("VCS:Tbl Size=%u<%u<%u\r\n", WL_FLASH_MIN_SIZE, tbsize, WL_FLASH_MAX_SIZE);
        return 2;
    }

    p = (u8 *)((u32)wlMan->pFlashImg + 2);
    crc = 0;
    c = 0;
    for (; tbsize > 0; tbsize--) {
        c = WL_ReadByte(p++);
        crc = calc_NextCRC(c, crc);
    }

    rcrc = ((FLASH_TABLE *)wlMan->pFlashImg)->checkSum;

    if (pCrc != NULL) {
        *pCrc = (crc << 16) | rcrc;
    }

    if (crc != rcrc) {
        return 1;
    }

    return 0;
}

void FLASH_Wait(void)
{
    vu32 statusReg;

    while (TRUE) {
        NVRAM_ReadStatusRegister((u8 *)&statusReg);
        if (statusReg & NVRAM_STATUS_REGISTER_ERSER) {
            NVRAM_SoftwareReset();
        } else if (!(statusReg & NVRAM_STATUS_REGISTER_WIP)) {
            break;
        }
    }
}

void FLASH_Read(u32 adrs, u32 size, u8 *pBuf)
{
    u8 *p;

    if (wlMan->pFlashImg == HEAPBUF_NOT_ENOUGH_MEMORY) {
        return;
    }

    p = (u8 *)((u32)wlMan->pFlashImg + adrs - WL_FLASH_STR_ADRS);

    while (size > 0) {
        WL_WriteByte(pBuf++, WL_ReadByte(p++));
        size--;
    }
}

void FLASH_DirectRead(u32 adrs, u32 size, u8 *pBuf)
{
    SPI_Lock(wlMan->lockID);
    FLASH_Wait();

    NVRAM_ReadDataBytes(adrs, size, (u8 *)pBuf);
    SPI_Unlock(wlMan->lockID);
}

u32 FLASH_MakeImage(void)
{
    u32 length;

    SPI_Lock(wlMan->lockID);
    FLASH_Wait();

    length = 0;
    NVRAM_ReadDataBytes(FLASH_ADRS(tableSize), 2, (u8 *)&length);
    SPI_Unlock(wlMan->lockID);

    if ((length < WL_FLASH_MIN_SIZE) || (length > WL_FLASH_MAX_SIZE)) {
        DbgPrint("FIMG:Tbl Size=%u<%u<%u\r\n", WL_FLASH_MIN_SIZE, length, WL_FLASH_MAX_SIZE);
        return FALSE;
    }

    length += 2;

    wlMan->pFlashImg = (void *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, length);
    if (wlMan->pFlashImg == HEAPBUF_NOT_ENOUGH_MEMORY) {
        return FALSE;
    }
    wlMan->pFlashImg = (void *)((u32)wlMan->pFlashImg + WL_RSV * 2);

    SPI_Lock(wlMan->lockID);
    FLASH_Wait();

    NVRAM_ReadDataBytes(WL_FLASH_STR_ADRS, length, wlMan->pFlashImg);
    SPI_Unlock(wlMan->lockID);

    return TRUE;
}

#ifndef ONLY_WL
void FLASH_Write(u32 adrs, u32 size, u8 *pBuf)
{
    u32 wsize, i, ofst;
    u8 *p;

    if (wlMan->pFlashImg != HEAPBUF_NOT_ENOUGH_MEMORY) {
        if ((adrs > 0x2a) && (adrs < 0x200)) {
            p = (u8 *)((u32)wlMan->pFlashImg + adrs - WL_FLASH_STR_ADRS);

            for (i = 0; i < size; i++) {
                WL_WriteByte(p++, WL_ReadByte(&pBuf[i]));
            }
        }
    }

    for (i = 0; size > 0;) {
        ofst = (adrs % FLASH_PAGE_SIZE);
        if ((ofst + size) > FLASH_PAGE_SIZE) {
            wsize = FLASH_PAGE_SIZE - ofst;
        } else {
            wsize = size;
        }

        SPI_Lock(wlMan->lockID);

        FLASH_Wait();

        NVRAM_WriteEnable();

        NVRAM_PageWrite(adrs, wsize, &pBuf[i]);

        NVRAM_WriteDisable();

        SPI_Unlock(wlMan->lockID);

        size -= wsize;
        adrs += wsize;
        i += wsize;
    }
}
#endif
