
#include "nitro/hw/common/lcd.h"
#include "nitro/os.h"
#include "nitro/os/common/systemWork.h"
#include "nitro/os/common/valarm.h"
#include "nitro/spi/ARM7/spi.h"
#include "nitro/spi/ARM7/tp.h"
#include "nitro/spi/common/type.h"

extern void _u32_div_f();

static struct {
    u16 unk_0[0x10];
    int unk_20;
    s32 unk_24;
    s32 unk_28;
    OSVAlarm unk_2c[4];
    u16 unk_cc[4];
} tpw;

// SPI_DummyWait was probably an static function declared in a header which wasn't inlined. This file was split
// into multiple files, hence the copy of it.
static void SPI_DummyWait(void);
static void TpVAlarmHandler(void *param1);
static void SetStability(u16 range);

static inline void SPI_Wait(void)
{
    while (reg_SPI_SPICNT & REG_SPI_SPICNT_BUSY_MASK);
}

static inline void SPI_Send(u16 data)
{
    reg_SPI_SPID = (u16)(data & 0x00ff);
}

static inline void SPI_SendWait(u16 data)
{
    SPI_Send(data);
    SPI_Wait();
}

static inline void TP_SPIChangeMode(SPITransMode continuous)
{
    reg_SPI_SPICNT = (u16)((0x0001 << REG_SPI_SPICNT_E_SHIFT) | (0x0000 << REG_SPI_SPICNT_I_SHIFT) | (SPI_COMMPARTNER_TP << REG_SPI_SPICNT_SEL_SHIFT) |
                           (continuous << REG_SPI_SPICNT_MODE_SHIFT) |
                           (0x0000 << REG_SPI_SPICNT_BUSY_SHIFT) |
                           (SPI_BAUDRATE_2MHZ << REG_SPI_SPICNT_BAUDRATE_SHIFT));
}

void TP_Init(void)
{
    s32 i;

    tpw.unk_20 = 0;
    tpw.unk_24 = SPI_TP_DEFAULT_STABILITY_RANGE;
    tpw.unk_28 = SPI_TP_DEFAULT_STABILITY_RANGE;

    for (i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; ++i) {
        tpw.unk_0[i] = 0;
    }

    if (!OS_IsVAlarmAvailable()) {
        OS_InitVAlarm();
    }
    for (i = 0; i < SPI_TP_SAMPLING_FREQUENCY_MAX; ++i) {
        OS_CreateVAlarm(&(tpw.unk_2c[i]));
        OS_SetVAlarmTag(&(tpw.unk_2c[i]), SPI_TP_VALARM_TAG);
    }

    SPI_Wait();
    TP_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(0x84);
    SPI_DummyWait();
    TP_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
}

static void SPI_DummyWait(void)
{
    reg_SPI_SPID = 0;
    while (reg_SPI_SPICNT & 0x80);
}

void TP_AnalyzeCommand(u32 data)
{
    if (data & SPI_PXI_START_BIT) {
        s32 i;

        for (i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; ++i) {
            tpw.unk_0[i] = 0;
        }
    }

    tpw.unk_0[(data & SPI_PXI_INDEX_MASK) >> SPI_PXI_INDEX_SHIFT] = (u16)((data & SPI_PXI_DATA_MASK) >> SPI_PXI_DATA_SHIFT);

    if (data & SPI_PXI_END_BIT) {
        u16 command;
        u16 wu16[2];

        command = (u16)((tpw.unk_0[0] & 0xff00) >> 8);

        switch (command) {
        case SPI_PXI_COMMAND_TP_SETUP_STABILITY:
            wu16[0] = (u16)(tpw.unk_0[0] & 0xff);
            SetStability(wu16[0]);
            break;

        case SPI_PXI_COMMAND_TP_SAMPLING:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_TP, (u32)command, 0)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
            }
            break;

        case SPI_PXI_COMMAND_TP_AUTO_ON:
            if (tpw.unk_20 != 0) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            wu16[0] = (u16)(tpw.unk_0[0] & 0xff);
            if ((wu16[0] == 0) || (wu16[0] > SPI_TP_SAMPLING_FREQUENCY_MAX)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            wu16[1] = tpw.unk_0[1];
            if (wu16[1] >= HW_LCD_LINES) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_TP, (u32)command, 2, (u32)wu16[0], (u32)wu16[1])) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }
            tpw.unk_20 = 1;
            break;

        case SPI_PXI_COMMAND_TP_AUTO_OFF:
            if (tpw.unk_20 != 2) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_TP, (u32)command, 0)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }
            tpw.unk_20 = 3;
            break;

        default:
            SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_COMMAND);
        }
    }
}

static void TP_AutoAdjustRange(SPITpData *tpdata, u16 density)
{
    static u8 invalid_cnt = 0;
    static u8 valid_cnt = 0;
#define RANGE_MAX 35
#define RANGE_INC_CNT 4
#define RANGE_DEC_CNT 4
#define RANGE_DEC_CONDITION(d, r) ((d) < ((r) >> 1))

    if (!tpdata->e.touch) {
        invalid_cnt = 0;
        valid_cnt = 0;
        return;
    }

    if (tpdata->e.validity) {
        valid_cnt = 0;
        if (++invalid_cnt >= RANGE_INC_CNT) {
            invalid_cnt = 0;
            if (tpw.unk_24 < RANGE_MAX) {
                tpw.unk_24 += 1;
            }
        }
    } else {
        invalid_cnt = 0;
        if (!RANGE_DEC_CONDITION(density, tpw.unk_24)) {
            valid_cnt = 0;
            return;
        }
        if (++valid_cnt >= RANGE_DEC_CNT) {
            valid_cnt = 0;
            if (tpw.unk_24 > tpw.unk_28) {
                tpw.unk_24 -= 1;
                invalid_cnt = RANGE_INC_CNT - 1;
            }
        }
    }
}

void TP_ExecuteProcess(SPIMessage *param1)
{
    switch (param1->unk_4) {
    case SPI_PXI_COMMAND_TP_AUTO_SAMPLING:
        if (tpw.unk_20 != 2) {
            return;
        }
        // fallthrough
    case SPI_PXI_COMMAND_TP_SAMPLING:
        {
            OSIntrMode e;

            e = OS_DisableInterrupts();
            if (!SPIi_CheckException(SPI_DEVICE_TYPE_TP)) {
                (void)OS_RestoreInterrupts(e);
                SPIi_ReturnResult((u16)(param1->unk_4), SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }
            SPIi_GetException(SPI_DEVICE_TYPE_TP);
            (void)OS_RestoreInterrupts(e);
        }
        {
            SPITpData temp;
            u16 density;
            TP_ExecSampling(&temp, tpw.unk_24, &density);
            TP_AutoAdjustRange(&temp, density);
            *((u16 *)(&(OS_GetSystemWork()->touch_panel[0]))) = temp.halfs[0];
            *((u16 *)(&(OS_GetSystemWork()->touch_panel[2]))) = temp.halfs[1];
        }
        if (param1->unk_4 == SPI_PXI_COMMAND_TP_SAMPLING) {
            SPIi_ReturnResult((u16)(param1->unk_4), SPI_PXI_RESULT_SUCCESS);
        } else {
            SPIi_ReturnResult((u16)(param1->unk_4), (u16)(param1->unk_8[0] & 0xff));
        }
        SPIi_ReleaseException(SPI_DEVICE_TYPE_TP);
        break;

    case SPI_PXI_COMMAND_TP_AUTO_ON:
        if (tpw.unk_20 == 1) {
            s32 i;

            for (i = 0; i < param1->unk_8[0]; ++i) {
                u16 v = (u16)((param1->unk_8[1] + ((i * HW_LCD_LINES) / param1->unk_8[0])) % HW_LCD_LINES);
                tpw.unk_cc[i] = v < 0xc8 ? v : (v < 0xd7 ? 0xd7 : v);
                OS_SetPeriodicVAlarm(&(tpw.unk_2c[i]), (s16)(tpw.unk_cc[i]), 10, TpVAlarmHandler, (void *)i);
            }
            SPIi_ReturnResult((u16)(param1->unk_4), SPI_PXI_RESULT_SUCCESS);
            tpw.unk_20 = 2;
        } else {
            SPIi_ReturnResult((u16)(param1->unk_4), SPI_PXI_RESULT_ILLEGAL_STATUS);
        }
        break;

    case SPI_PXI_COMMAND_TP_AUTO_OFF:
        if (tpw.unk_20 == 3) {
            OS_CancelVAlarms(SPI_TP_VALARM_TAG);
            SPIi_ReturnResult((u16)(param1->unk_4), SPI_PXI_RESULT_SUCCESS);
            tpw.unk_20 = 0;
        } else {
            SPIi_ReturnResult((u16)(param1->unk_4), SPI_PXI_RESULT_ILLEGAL_STATUS);
        }
        break;
    }
}

static asm void TpVAlarmHandler(void *param1)
{
    stmdb sp!, {r3, r4, lr}
    sub sp, sp, #4
    mov r4, r0
    mov r3, r4
    mov r0, #0
    mov r1, #0x10
    mov r2, #1
    bl SPIi_SetEntry
    cmp r0, #0
    bne _0380144C
    ldr r0, [sp]
    ldr r3, =0x027fffaa
    bic r0, r0, #0x6000000
    orr r0, r0, #0x6000000
    str r0, [sp]
    ldrh r0, [sp]
    ldrh r2, [sp, #2]
    strh r0, [r3]
    and r1, r4, #0xff
    mov r0, #0x10
    strh r2, [r3, #2]
    bl SPIi_ReturnResult

_0380144C:
    add sp, sp, #4
    ldmia sp!, {r3, r4, lr}
    bx lr
}

static void SetStability(u16 range)
{
    if (range == 0) {
        SPIi_ReturnResult(SPI_PXI_COMMAND_TP_SETUP_STABILITY, SPI_PXI_RESULT_INVALID_PARAMETER);
        return;
    }
    tpw.unk_24 = (s32)range;
    tpw.unk_28 = (s32)range;

    SPIi_ReturnResult(SPI_PXI_COMMAND_TP_SETUP_STABILITY, SPI_PXI_RESULT_SUCCESS);
    return;
}
