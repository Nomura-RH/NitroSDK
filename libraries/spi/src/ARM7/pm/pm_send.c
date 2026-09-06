#include "nitro/spi/ARM7/pm.h"

void PMi_SendPxiData(u32 data)
{
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_PM, data, FALSE) != PXI_FIFO_SUCCESS) {
    }
}

void PMi_SendPxiCommand(u16 command, u16 addr, u16 data)
{
    u32 pxi_send_data = ((command & PM_COMMAND_MASK) << PM_COMMAND_SHIFT)
        | ((addr & PM_REG_OP_ADDR_MASK) << PM_REG_OP_ADDR_SHIFT)
        | ((data & PM_REG_OP_DATA_MASK) << PM_REG_OP_DATA_SHIFT);

    PMi_SendPxiData(pxi_send_data);
}
