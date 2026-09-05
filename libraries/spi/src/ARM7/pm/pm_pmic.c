#include "nitro/spi/ARM7/pm.h"
#include "nitro/spi/ARM7/spi.h"

static void PMi_ChangeSpiMode(SPITransMode continuous)
{
    reg_SPI_SPICNT = (u16)((1 << REG_SPI_SPICNT_E_SHIFT)
        | (0 << REG_SPI_SPICNT_I_SHIFT)
        | (SPI_COMMPARTNER_PMIC << REG_SPI_SPICNT_SEL_SHIFT)
        | (continuous << REG_SPI_SPICNT_MODE_SHIFT)
        | (0 << REG_SPI_SPICNT_BUSY_SHIFT)
        | (PM_BAUDRATE_PMIC_DEFAULT << REG_SPI_SPICNT_BAUDRATE_SHIFT));
}

static void PMi_ChangeSpiModeTP(void)
{
    reg_SPI_SPICNT = (u16)((1 << REG_SPI_SPICNT_E_SHIFT)
        | (0 << REG_SPI_SPICNT_I_SHIFT)
        | (SPI_COMMPARTNER_TP << REG_SPI_SPICNT_SEL_SHIFT)
        | (SPI_TRANSMODE_1BYTE << REG_SPI_SPICNT_MODE_SHIFT)
        | (0 << REG_SPI_SPICNT_BUSY_SHIFT)
        | (PM_BAUDRATE_PMIC_DEFAULT << REG_SPI_SPICNT_BAUDRATE_SHIFT));
}

void PMi_SetRegister(u16 reg, u8 data)
{
    SPI_Wait();

    PMi_ChangeSpiModeTP();

    PMi_ChangeSpiMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait((u8)(reg | (PMIC_REG_WRITE & PMIC_REG_OP_MASK) << PMIC_REG_OP_SHIFT));

    PMi_ChangeSpiMode(SPI_TRANSMODE_1BYTE);
    SPI_Send(data);
}

u8 PMi_GetRegister(u16 reg)
{
    SPI_Wait();

    PMi_ChangeSpiModeTP();

    PMi_ChangeSpiMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait((u8)(reg | (PMIC_REG_READ & PMIC_REG_OP_MASK) << PMIC_REG_OP_SHIFT));

    PMi_ChangeSpiMode(SPI_TRANSMODE_1BYTE);
    u8 data = SPI_DummyWaitReceive();
    return data;
}

void PMi_SetControl(u8 sw)
{
    u8 data = PMi_GetRegister(REG_PMIC_CTL_ADDR);
    data |= sw;
    PMi_SetRegister(REG_PMIC_CTL_ADDR, data);
}

void PMi_ResetControl(u8 sw)
{
    u8 data = PMi_GetRegister(REG_PMIC_CTL_ADDR);
    data &= ~sw;
    PMi_SetRegister(REG_PMIC_CTL_ADDR, data);
}
