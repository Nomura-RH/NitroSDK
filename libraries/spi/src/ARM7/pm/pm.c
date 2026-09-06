#include "nitro/spi/ARM7/pm.h"

#include <nitro/hw/ARM7/ioreg_SPI.h>
#include <nitro/os/common/interrupt.h>
#include <nitro/spi/common/pm_common.h>

BOOL PMi_Initialized = FALSE;
u16 PMi_TriggerBL = 0;
u16 PMi_KeyPattern = 0;
PMWork PMi_Work;

#define PMi_GetPxiCommand(x) (((x) & 0xFF00) >> 8)
#define PMi_GetPxiIndex(x)   (((x) & SPI_PXI_INDEX_MASK) >> SPI_PXI_INDEX_SHIFT)
#define PMi_GetPxiData(x)    (((x) & SPI_PXI_DATA_MASK) >> SPI_PXI_DATA_SHIFT)
#define PMi_GetPxiDataLo(x)  (((x) & 0xFF) >> SPI_PXI_DATA_SHIFT)

void PM_Init(void)
{
    PMi_Initialized = TRUE;
    PMi_Work.status = PM_STATUS_READY;

    for (int i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; i++) {
        PMi_Work.command[i] = 0;
    }
}

void PM_AnalyzeCommand(u32 data)
{
    if (data & SPI_PXI_START_BIT) {
        for (int i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; i++) {
            PMi_Work.command[i] = 0;
        }
    }

    PMi_Work.command[PMi_GetPxiIndex(data)] = PMi_GetPxiData(data);

    if (data & SPI_PXI_END_BIT) {
        u16 command = PMi_GetPxiCommand(PMi_Work.command[0]);

        switch (command) {
        case SPI_PXI_COMMAND_PM_SYNC:
            SPIi_ReturnResult(SPI_PXI_COMMAND_PM_SYNC, PM_RESULT_SUCCESS);
            break;

        case SPI_PXI_COMMAND_PM_SLEEP_START:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_PM, command, 2, PMi_GetPxiDataLo(PMi_Work.command[0]), PMi_GetPxiData(PMi_Work.command[1]))) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
            }
            break;

        case SPI_PXI_COMMAND_PM_REG_WRITE:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_PM, command, 2, PMi_GetPxiDataLo(PMi_Work.command[0]), PMi_GetPxiData(PMi_Work.command[1]))) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
            }
            break;

        case SPI_PXI_COMMAND_PM_REG_READ:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_PM, command, 1, PMi_GetPxiData(PMi_Work.command[0]))) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
            }
            break;

        case SPI_PXI_COMMAND_PM_UTILITY:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_PM, command, 1, PMi_GetPxiDataLo(PMi_Work.command[0]) << 16 | PMi_GetPxiData(PMi_Work.command[1]))) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
            }
            break;

        case SPI_PXI_COMMAND_PM_SELF_BLINK: {
            PMLEDPattern pattern = PMi_GetPxiDataLo(PMi_Work.command[0]);
            PM_SetLEDPattern(pattern);
            SPIi_ReturnResult(SPI_PXI_COMMAND_PM_SELF_BLINK, PM_RESULT_SUCCESS);
        } break;

        case SPI_PXI_COMMAND_PM_GET_BLINK:
            SPIi_ReturnResult(SPI_PXI_COMMAND_PM_GET_BLINK, PM_GetLEDPattern());
            break;

        default:
            SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_COMMAND);
            break;
        }
    }
}

void PM_ExecuteProcess(SPIEntry *entry)
{
    OSIntrMode e = OS_DisableInterrupts();
    if (!SPIi_CheckException(SPI_DEVICE_TYPE_PM)) {
        OS_RestoreInterrupts(e);
        SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_EXCLUSIVE);
        return;
    }
    SPIi_GetException(SPI_DEVICE_TYPE_PM);
    OS_RestoreInterrupts(e);

    switch (entry->process) {
    case SPI_PXI_COMMAND_PM_SLEEP_START:
        PMi_Work.status = PM_STATUS_START_SLEEP;
        PMi_TriggerBL = entry->arg[0];
        PMi_KeyPattern = entry->arg[1];
        PMi_DoSleep();
        break;

    case SPI_PXI_COMMAND_PM_REG_WRITE:
        PMi_Work.status = PM_STATUS_WRITE_REGISTER;
        PMi_Work.regNumber = entry->arg[0];
        PMi_Work.param = entry->arg[1];
        PMi_SetRegister(PMi_Work.regNumber, PMi_Work.param);
        SPIi_ReturnResult(SPI_PXI_COMMAND_PM_REG_WRITE, PM_RESULT_SUCCESS);
        break;

    case SPI_PXI_COMMAND_PM_REG_READ:
        PMi_Work.status = PM_STATUS_READ_REGISTER;
        PMi_Work.regNumber = entry->arg[0];

        u16 addr = PMi_Work.regNumber;
        SPIi_ReturnResult(SPI_PXI_COMMAND_PM_REG0VALUE + addr, PMi_GetRegister(addr));
        break;

    case SPI_PXI_COMMAND_PM_UTILITY:
        PMi_Work.status = PM_STATUS_UTILITY;
        PMi_Work.param = entry->arg[0];
        PMi_SwitchUtilityProc(PMi_Work.param);
        SPIi_ReturnResult(SPI_PXI_COMMAND_PM_UTILITY, PM_RESULT_SUCCESS);
        break;

    case SPI_PXI_COMMAND_PM_SELF_BLINK:
        PMi_SetLED(entry->arg[0]);
        break;

    default:
        SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_INVALID_COMMAND);
    }

    SPIi_ReleaseException(SPI_DEVICE_TYPE_PM);
}
