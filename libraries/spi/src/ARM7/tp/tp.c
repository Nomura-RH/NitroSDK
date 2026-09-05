#include "nitro/spi/ARM7/tp.h"

#include "nitro/spi/ARM7/spi.h"
#include <nitro/hw/common/lcd.h>
#include <nitro/os.h>
#include <nitro/os/common/systemWork.h>
#include <nitro/os/common/valarm.h>
#include <nitro/spi/common/type.h>

static TPWork tpw;

static void TpVAlarmHandler(void *arg);
static void SetStability(u16 range);

void TP_Init(void)
{
    s32 i;

    tpw.status = TP_STATUS_READY;
    tpw.range = SPI_TP_DEFAULT_STABILITY_RANGE;
    tpw.rangeMin = SPI_TP_DEFAULT_STABILITY_RANGE;

    for (i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; i++) {
        tpw.command[i] = 0;
    }

    if (!OS_IsVAlarmAvailable()) {
        OS_InitVAlarm();
    }
    for (i = 0; i < SPI_TP_SAMPLING_FREQUENCY_MAX; i++) {
        OS_CreateVAlarm(&(tpw.vAlarm[i]));
        OS_SetVAlarmTag(&(tpw.vAlarm[i]), SPI_TP_VALARM_TAG);
    }

    SPI_Wait();
    TP_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(TP_COMMAND_DETECT_TOUCH);
    SPI_DummyWait();
    TP_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
}

void TP_AnalyzeCommand(u32 data)
{
    if (data & SPI_PXI_START_BIT) {
        for (int i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; i++) {
            tpw.command[i] = 0;
        }
    }

    tpw.command[(data & SPI_PXI_INDEX_MASK) >> SPI_PXI_INDEX_SHIFT] = (u16)((data & SPI_PXI_DATA_MASK) >> SPI_PXI_DATA_SHIFT);

    if (data & SPI_PXI_END_BIT) {
        u16 command = (tpw.command[0] & 0xFF00) >> 8;
        u16 wu16[2];

        switch (command) {
        case SPI_PXI_COMMAND_TP_SETUP_STABILITY:
            wu16[0] = tpw.command[0] & 0xFF;
            SetStability(wu16[0]);
            break;

        case SPI_PXI_COMMAND_TP_SAMPLING:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_TP, (u32)command, 0)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
            }
            break;

        case SPI_PXI_COMMAND_TP_AUTO_ON:
            if (tpw.status != 0) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }

            wu16[0] = tpw.command[0] & 0xFF;
            if (wu16[0] == 0 || wu16[0] > SPI_TP_SAMPLING_FREQUENCY_MAX) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }

            wu16[1] = tpw.command[1];
            if (wu16[1] >= HW_LCD_LINES) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }

            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_TP, command, 2, wu16[0], wu16[1])) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }

            tpw.status = TP_STATUS_AUTO_START;
            break;

        case SPI_PXI_COMMAND_TP_AUTO_OFF:
            if (tpw.status != TP_STATUS_AUTO_SAMPLING) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }

            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_TP, command, 0)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }

            tpw.status = TP_STATUS_AUTO_WAIT_END;
            break;

        default:
            SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_COMMAND);
        }
    }
}

#ifdef SDK_TP_AUTO_ADJUST_RANGE
#define RANGE_MAX                 35
#define RANGE_INC_CNT             4
#define RANGE_DEC_CNT             4
#define RANGE_DEC_CONDITION(d, r) ((d) < ((r) >> 1))

static void TP_AutoAdjustRange(SPITpData *tpdata, u16 density)
{
    static u8 invalid_cnt = 0;
    static u8 valid_cnt = 0;

    if (!tpdata->e.touch) {
        invalid_cnt = 0;
        valid_cnt = 0;
        return;
    }

    if (tpdata->e.validity) {
        valid_cnt = 0;
        if (++invalid_cnt >= RANGE_INC_CNT) {
            invalid_cnt = 0;
            if (tpw.range < RANGE_MAX) {
                tpw.range++;
            }
        }
    } else {
        invalid_cnt = 0;
        if (!RANGE_DEC_CONDITION(density, tpw.range)) {
            valid_cnt = 0;
            return;
        }
        if (++valid_cnt >= RANGE_DEC_CNT) {
            valid_cnt = 0;
            if (tpw.range > tpw.rangeMin) {
                tpw.range--;
                invalid_cnt = RANGE_INC_CNT - 1;
            }
        }
    }
}
#endif

void TP_ExecuteProcess(SPIEntry *entry)
{
    switch (entry->process) {
    case SPI_PXI_COMMAND_TP_AUTO_SAMPLING:
        if (tpw.status != TP_STATUS_AUTO_SAMPLING) {
            return;
        }
        // fallthrough
    case SPI_PXI_COMMAND_TP_SAMPLING: {
        OSIntrMode e = OS_DisableInterrupts();
        if (!SPIi_CheckException(SPI_DEVICE_TYPE_TP)) {
            OS_RestoreInterrupts(e);
            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_EXCLUSIVE);
            return;
        }
        SPIi_GetException(SPI_DEVICE_TYPE_TP);
        OS_RestoreInterrupts(e);
    }

        SPITpData temp;
#ifdef SDK_TP_AUTO_ADJUST_RANGE
        u16 density;
        TP_ExecSampling(&temp, tpw.range, &density);
        TP_AutoAdjustRange(&temp, density);
#else
        TP_ExecSampling(&temp, tpw.range);
#endif
        *((u16 *)(&(OS_GetSystemWork()->touch_panel[0]))) = temp.halfs[0];
        *((u16 *)(&(OS_GetSystemWork()->touch_panel[2]))) = temp.halfs[1];

        if (entry->process == SPI_PXI_COMMAND_TP_SAMPLING) {
            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_SUCCESS);
        } else {
            SPIi_ReturnResult(entry->process, entry->arg[0] & 0xFF);
        }
        SPIi_ReleaseException(SPI_DEVICE_TYPE_TP);
        break;

    case SPI_PXI_COMMAND_TP_AUTO_ON:
        if (tpw.status == TP_STATUS_AUTO_START) {
            for (int i = 0; i < entry->arg[0]; i++) {
                u16 vCount = (entry->arg[1] + ((i * HW_LCD_LINES) / entry->arg[0])) % HW_LCD_LINES;
                if (vCount >= 200 && vCount < 215) {
                    vCount = 215;
                }
                tpw.vCount[i] = vCount;
                OS_SetPeriodicVAlarm(&tpw.vAlarm[i], tpw.vCount[i], TP_VALARM_DELAY_MAX, TpVAlarmHandler, (void *)i);
            }
            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_SUCCESS);
            tpw.status = TP_STATUS_AUTO_SAMPLING;
        } else {
            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_ILLEGAL_STATUS);
        }
        break;

    case SPI_PXI_COMMAND_TP_AUTO_OFF:
        if (tpw.status == TP_STATUS_AUTO_WAIT_END) {
            OS_CancelVAlarms(SPI_TP_VALARM_TAG);
            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_SUCCESS);
            tpw.status = TP_STATUS_READY;
        } else {
            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_ILLEGAL_STATUS);
        }
        break;
    }
}

static void TpVAlarmHandler(void *arg)
{
    if (!SPIi_SetEntry(SPI_DEVICE_TYPE_TP, SPI_PXI_COMMAND_TP_AUTO_SAMPLING, 1, (u32)arg)) {
        SPITpData temp;

        temp.e.validity = SPI_TP_VALIDITY_INVALID_XY;
        *((u16 *)(&(OS_GetSystemWork()->touch_panel[0]))) = temp.halfs[0];
        *((u16 *)(&(OS_GetSystemWork()->touch_panel[2]))) = temp.halfs[1];
        SPIi_ReturnResult(SPI_PXI_COMMAND_TP_AUTO_SAMPLING, (u32)arg & 0xFF);
    }
}

static void SetStability(u16 range)
{
    if (range == 0) {
        SPIi_ReturnResult(SPI_PXI_COMMAND_TP_SETUP_STABILITY, SPI_PXI_RESULT_INVALID_PARAMETER);
        return;
    }
    tpw.range = range;
    tpw.rangeMin = range;

    SPIi_ReturnResult(SPI_PXI_COMMAND_TP_SETUP_STABILITY, SPI_PXI_RESULT_SUCCESS);
    return;
}
