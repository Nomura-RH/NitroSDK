#include "nitro/spi/ARM7/nvram.h"
#include "nitro/spi/ARM7/spi.h"
#include "nitro/spi/common/type.h"
#include <nitro/hw/ARM7/ioreg.h>
#include <nitro/mi/byteAccess.h>

#define SDK_SPI_LOW_SPEED_LOW_CODE_SIZE

void NVRAM_WriteEnable(void)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(NVRAM_INSTRUCTION_WREN);
}

void NVRAM_WriteDisable(void)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(NVRAM_INSTRUCTION_WRDI);
}

void NVRAM_ReadStatusRegister(u8 *buf)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(NVRAM_INSTRUCTION_RDSR);
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_Dummy();
#ifdef SDK_SPI_LOW_SPEED_LOW_CODE_SIZE
    SPI_Wait();
    MI_WriteByte(buf, SPI_Receive());
#else
    u16 r;
    u16 temp;

    if ((u32)buf % 2) {
        temp = *((u16 *)(buf - 1));
        SPI_Wait();
        r = SPI_Receive();
        temp = (temp & 0xFF) | (r << 8);
        *((u16 *)(buf - 1)) = temp;
    } else {
        temp = *((u16 *)buf);
        SPI_Wait();
        r = SPI_Receive();
        temp = (temp & 0xFF00) | r;
        *((u16 *)buf) = temp;
    }
#endif
}

void NVRAM_ReadDataBytes(u32 address, u32 size, u8 *buf)
{
    u16 adr[3];
    s32 i;

    if (size < 1) {
        return;
    }

    adr[0] = (address & 0x00FF0000) >> 16;
    adr[1] = (address & 0x0000FF00) >> 8;
    adr[2] = address & 0x000000FF;

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_Send(NVRAM_INSTRUCTION_READ);
    for (i = 0; i < 3; i++) {
        SPI_Wait();
        SPI_Send(adr[i]);
    }
#ifdef SDK_SPI_LOW_SPEED_LOW_CODE_SIZE
    SPI_Wait();
    for (i = 0; i < size - 1; i++) {
        SPI_Dummy();
        SPI_Wait();
        MI_WriteByte(&buf[i], SPI_Receive());
    }
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
    MI_WriteByte(&buf[i], SPI_Receive());
#else
    s32 j;
    u16 r;
    u16 temp;

    if (size == 1) {
        SPI_Wait();
        NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
        SPI_Dummy();
        if ((u32)buf % 2) {
            temp = *((u16 *)(buf - 1));
            SPI_Wait();
            r = SPI_Receive();
            temp = (temp & 0xFF) | (r << 8);
            *((u16 *)(buf - 1)) = temp;
        } else {
            temp = *((u16 *)buf);
            SPI_Wait();
            r = SPI_Receive();
            temp = (temp & 0xFF00) | r;
            *((u16 *)buf) = temp;
        }
    } else {
        SPI_Wait();
        SPI_Dummy();
        if ((u32)buf % 2) {
            temp = *((u16 *)(buf - 1));
            SPI_Wait();
            r = SPI_Receive();
            SPI_Dummy();
            temp = (temp & 0x00ff) | (r << 8);
            *((u16 *)(buf - 1)) = temp;
            i = 1;
        } else {
            i = 0;
        }

        for (j = 0; i < size - 1; i++, j++) {
            if (j % 2) {
                SPI_Wait();
                temp |= SPI_Receive() << 8;
                SPI_Dummy();
                *((u16 *)(buf + i - 1)) = temp;
            } else {
                SPI_Wait();
                temp = SPI_Receive();
                SPI_Dummy();
            }
        }

        if (j % 2) {
            SPI_Wait();
            temp |= SPI_Receive() << 8;
            NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
            SPI_Dummy();
            *((u16 *)(buf + i - 1)) = temp;
        } else {
            temp = *((u16 *)(buf + i));
            SPI_Wait();
            r = SPI_Receive();
            NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
            SPI_Dummy();
            temp = (temp & 0xFF00) | r;
            *((u16 *)(buf + i)) = temp;
        }
        SPI_Wait();
    }
#endif
}

#ifdef SDK_NVRAM_USE_READ_HIGHER_SPEED
void NVRAM_ReadDataBytesAtHigherSpeed(u32 address, u32 size, u8 *buf)
{
    u16 adr[3];
    s32 i;

    if (size < 1) {
        return;
    }

    adr[0] = (address & 0x00FF0000) >> 16;
    adr[1] = (address & 0x0000FF00) >> 8;
    adr[2] = address & 0x000000FF;

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_Send(NVRAM_INSTRUCTION_FAST_READ);
    for (i = 0; i < 3; i++) {
        SPI_Wait();
        SPI_Send(adr[i]);
    }
    SPI_Wait();
    SPI_Dummy();
#ifdef SDK_SPI_LOW_SPEED_LOW_CODE_SIZE
    SPI_Wait();
    for (i = 0; i < size - 1; i++) {
        SPI_Dummy();
        SPI_Wait();
        MI_WriteByte(&buf[i], SPI_Receive());
    }

    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
    MI_WriteByte(&buf[i], SPI_Receive());
#else
    s32 j;
    u16 r;
    u16 temp;

    if (size == 1) {
        SPI_Wait();
        NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
        SPI_Dummy();
        if ((u32)buf % 2) {
            temp = *((u16 *)(buf - 1));
            SPI_Wait();
            r = SPI_Receive();
            temp = (temp & 0xFF) | (r << 8);
            *((u16 *)(buf - 1)) = temp;
        } else {
            temp = *((u16 *)buf);
            SPI_Wait();
            r = SPI_Receive();
            temp = (temp & 0xFF00) | r;
            *((u16 *)buf) = temp;
        }
    } else {
        SPI_Wait();
        SPI_Dummy();
        if ((u32)buf % 2) {
            temp = *((u16 *)(buf - 1));
            SPI_Wait();
            r = SPI_Receive();
            SPI_Dummy();
            temp = (temp & 0xFF) | (r << 8);
            *((u16 *)(buf - 1)) = temp;
            i = 1;
        } else {
            i = 0;
        }

        for (j = 0; i < (size - 1); i++, j++) {
            if (j % 2) {
                SPI_Wait();
                temp |= SPI_Receive() << 8;
                SPI_Dummy();
                *((u16 *)(buf + i - 1)) = temp;
            } else {
                SPI_Wait();
                temp = SPI_Receive();
                SPI_Dummy();
            }
        }

        if (j % 2) {
            SPI_Wait();
            temp |= SPI_Receive() << 8;
            NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
            SPI_Dummy();
            *((u16 *)(buf + i - 1)) = temp;
        } else {
            temp = *((u16 *)(buf + i));
            SPI_Wait();
            r = SPI_Receive();
            NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
            SPI_Dummy();
            temp = (u16)((temp & 0xFF00) | r);
            *((u16 *)(buf + i)) = temp;
        }
        SPI_Wait();
    }
#endif
}
#endif

void NVRAM_PageWrite(u32 address, u16 size, const u8 *buf)
{
    u16 adr[3];
    s32 i;

    if (size < 1) {
        return;
    }

    if ((address + size - 1) / SPI_NVRAM_PAGE_SIZE > address / SPI_NVRAM_PAGE_SIZE) {
        size = SPI_NVRAM_PAGE_SIZE - (address % SPI_NVRAM_PAGE_SIZE);
    }

    adr[0] = (address & 0x00FF0000) >> 16;
    adr[1] = (address & 0x0000FF00) >> 8;
    adr[2] = address & 0x000000FF;

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_Send(NVRAM_INSTRUCTION_PW);
    for (i = 0; i < 3; i++) {
        SPI_Wait();
        SPI_Send(adr[i]);
    }
#ifdef SDK_SPI_LOW_SPEED_LOW_CODE_SIZE
    for (i = 0; i < size - 1; i++) {
        SPI_Wait();
        SPI_Send(MI_ReadByte(&buf[i]));
    }

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(MI_ReadByte(&buf[i]));
#else
    u16 temp;

    if ((u32)buf % 2) {
        temp = *((u16 *)(buf - 1));
    }

    for (i = 0; i < (size - 1); i++) {
        if (((u32)buf + i) % 2) {
            SPI_Wait();
            SPI_Send((temp & 0xFF00) >> 8);
        } else {
            temp = *((u16 *)(buf + i));
            SPI_Wait();
            SPI_Send(temp & 0xFF);
        }
    }

    if (((u32)buf + i) % 2) {
        SPI_Wait();
        NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
        SPI_Send((temp & 0xFF00) >> 8);
    } else {
        temp = *((u16 *)(buf + i));
        SPI_Wait();
        NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
        SPI_Send(temp & 0xFF);
    }
    SPI_Wait();
#endif
}

void NVRAM_PageProgram(u32 address, u16 size, const u8 *buf)
{
    u16 adr[3];
    s32 i;

    if (size < 1) {
        return;
    }

    if ((address + size - 1) / SPI_NVRAM_PAGE_SIZE > address / SPI_NVRAM_PAGE_SIZE) {
        size = SPI_NVRAM_PAGE_SIZE - (address % SPI_NVRAM_PAGE_SIZE);
    }

    adr[0] = (address & 0x00FF0000) >> 16;
    adr[1] = (address & 0x0000FF00) >> 8;
    adr[2] = address & 0x000000FF;

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_Send(NVRAM_INSTRUCTION_PP);
    for (i = 0; i < 3; i++) {
        SPI_Wait();
        SPI_Send(adr[i]);
    }
#ifdef SDK_SPI_LOW_SPEED_LOW_CODE_SIZE
    for (i = 0; i < size - 1; i++) {
        SPI_Wait();
        SPI_Send(MI_ReadByte(&buf[i]));
    }

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(MI_ReadByte(&buf[i]));
#else
    u16 temp;

    if ((u32)buf % 2) {
        temp = *((u16 *)(buf - 1));
    }

    for (i = 0; i < (size - 1); i++) {
        if (((u32)buf + i) % 2) {
            SPI_Wait();
            SPI_Send((temp & 0xFF00) >> 8);
        } else {
            temp = *((u16 *)(buf + i));
            SPI_Wait();
            SPI_Send(temp & 0xFF);
        }
    }

    if (((u32)buf + i) % 2) {
        SPI_Wait();
        NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
        SPI_Send((temp & 0xFF00) >> 8);
    } else {
        temp = *((u16 *)(buf + i));
        SPI_Wait();
        NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
        SPI_Send(temp & 0xFF);
    }
    SPI_Wait();
#endif
}

void NVRAM_PageErase(u32 address)
{
    u16 adr[3];

    adr[0] = (address & 0x00FF0000) >> 16;
    adr[1] = (address & 0x0000FF00) >> 8;
    adr[2] = address & 0x000000FF;

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(NVRAM_INSTRUCTION_PE);
    SPI_SendWait(adr[0]);
    SPI_SendWait(adr[1]);
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(adr[2]);
}

void NVRAM_SectorErase(u32 address)
{
    u16 adr[3];

    adr[0] = (address & 0x00FF0000) >> 16;
    adr[1] = (address & 0x0000FF00) >> 8;
    adr[2] = address & 0x000000FF;

    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(NVRAM_INSTRUCTION_SE);
    SPI_SendWait(adr[0]);
    SPI_SendWait(adr[1]);
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(adr[2]);
}

void NVRAM_DeepPowerDown(void)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(NVRAM_INSTRUCTION_DP);
}
#ifdef SDK_NVRAM_ANOTHER_MAKER
void NVRAM_ReleaseFromDeepPowerDown(void)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(NVRAM_INSTRUCTION_RDP);
}

void NVRAM_ChipErase(void)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(NVRAM_INSTRUCTION_CE);
}

void NVRAM_ReadSiliconId(u8 *buf)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(NVRAM_INSTRUCTION_RSI);
    SPI_Dummy();
#ifdef SDK_SPI_LOW_SPEED_LOW_CODE_SIZE
    SPI_Wait();
    MI_WriteByte(buf, SPI_Receive());
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_Dummy();
    SPI_Wait();
    MI_WriteByte(&buf[1], SPI_Receive());
#else
    u16 r;
    u16 temp;
    s32 i;

    for (i = 0; i < 2; i++) {
        if ((u32)buf % 2) {
            temp = *((u16 *)(buf - 1));
            SPI_Wait();
            r = SPI_Receive();
            temp = (temp & 0xFF) | (r << 8);
            *((u16 *)(buf - 1)) = temp;
        } else {
            temp = *((u16 *)buf);
            SPI_Wait();
            r = SPI_Receive();
            temp = (temp & 0xFF00) | r;
            *((u16 *)(buf - 1)) = temp;
        }

        buf = (u8 *)(buf + 1);
        NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
        SPI_Dummy();
    }
#endif
}

void NVRAM_SoftwareReset(void)
{
    SPI_Wait();
    NVRAM_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_SendWait(NVRAM_INSTRUCTION_SR);
}
#endif
