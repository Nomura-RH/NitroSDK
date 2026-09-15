#include "nitro/spi/ARM7/mic.h"
#include "nitro/spi/ARM7/spi.h"

#ifdef SDK_PATCH3
static s16 offset12;
static s8 offset8;
static u32 sam12;
static u32 sam8;
static u32 counter12;
static u32 counter8;
#endif

u16 MIC_ExecSampling8(void)
{
    SPI_Wait();
    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_SAMPLING_8);
    u16 temp = SPI_DummyWaitReceive() << 8;
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    temp |= SPI_DummyWaitReceive();

    temp = (temp & MIC_S8_VALID_BIT_MASK) >> MIC_S8_VALID_BIT_SHIFT;
    
#ifdef SDK_PATCH3
    sam8 += temp;
    counter8++;
    if (counter8 >= 0x1000) {
        s8 average = sam8 / 4096 - 0x80;
        if (offset8 < average && offset8 < 0xC) {
            offset8++;
        } else if (average < offset8 && offset8 > -0xC) {
            offset8--;
        }
        counter8 = 0;
        sam8 = 0;
    }
    s32 adjusted = temp - offset8;
    if (adjusted > 0xFF) {
        adjusted = 0xFF;
    } else if (adjusted < 0) {
        adjusted = 0;
    }
    return adjusted;
#else
    return temp;
#endif
}

u16 MIC_ExecSampling12(void)
{
    SPI_Wait();
    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_SAMPLING_12);
    u16 temp = SPI_DummyWaitReceive() << 8;
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    temp |= SPI_DummyWaitReceive();

    temp = (temp & MIC_S12_VALID_BIT_MASK) << MIC_S12_VALID_BIT_L_SHIFT;
    
#ifdef SDK_PATCH3
    sam12 += temp;
    counter12++;
    if (counter12 >= 0x100) {
        s16 average = ((sam12 / 256) & 0xFFF0) - 0x8000;
        if (offset12 < average && offset12 < 0xC00) {
            offset12 += 16;
        } else if (average < offset12 && offset12 > -0xC00) {
            offset12 -= 16;
        }
        counter12 = 0;
        sam12 = 0;
    }
    s32 adjusted = temp - offset12;
    if (adjusted > 0xFFF0) {
        adjusted = 0xFFF0;
    } else if (adjusted < 0) {
        adjusted = 0;
    }
    return adjusted;
#else
    return temp;
#endif
}

u16 MIC_OneTimeSampling8(void)
{
    u16 temp;
#ifdef MIC_USE_INNER_REFERENCE
    s32 i;
#endif

    SPI_Wait();

#ifdef MIC_USE_INNER_REFERENCE
    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_SAMPLING_8);
    for (i = 0; i < 22; i++) {
        SPI_DummyWait();
    }
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
#endif

    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_SAMPLING_8);
    temp = SPI_DummyWaitReceive() << 8;
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    temp |= SPI_DummyWaitReceive();

    temp = (temp & MIC_S8_VALID_BIT_MASK) >> MIC_S8_VALID_BIT_SHIFT;

#ifdef MIC_USE_INNER_REFERENCE
    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_POWER_DOWN);
    SPI_DummyWait();
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
#endif

    return temp;
}

u16 MIC_OneTimeSampling12(void)
{
    u16 temp;
#ifdef MIC_USE_INNER_REFERENCE
    s32 i;
#endif

    SPI_Wait();

#ifdef MIC_USE_INNER_REFERENCE
    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_SAMPLING_12);
    for (i = 0; i < 12; i++) {
        SPI_DummyWait();
    }
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
#endif

    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_SAMPLING_12);
    temp = SPI_DummyWaitReceive() << 8;
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    temp |= SPI_DummyWaitReceive();

    temp = (temp & MIC_S12_VALID_BIT_MASK) << MIC_S12_VALID_BIT_L_SHIFT;

#ifdef MIC_USE_INNER_REFERENCE
    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_POWER_DOWN);
    SPI_DummyWait();
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
#endif

    return temp;
}
