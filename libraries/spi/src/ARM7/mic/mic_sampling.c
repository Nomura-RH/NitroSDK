#include "nitro/spi/ARM7/mic.h"
#include "nitro/spi/ARM7/spi.h"

u16 MIC_ExecSampling8(void)
{
    SPI_Wait();
    MIC_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(MIC_COMMAND_SAMPLING_8);
    u16 temp = SPI_DummyWaitReceive() << 8;
    MIC_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    temp |= SPI_DummyWaitReceive();

    temp = (temp & MIC_S8_VALID_BIT_MASK) >> MIC_S8_VALID_BIT_SHIFT;
    return temp;
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
    return temp;
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
