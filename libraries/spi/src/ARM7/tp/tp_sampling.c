#include "nitro/spi/ARM7/spi.h"
#include "nitro/spi/ARM7/tp.h"
#include <nitro/exi/ARM7/genPort.h>
#include <nitro/math/math.h>

typedef enum {
    TP_DETECT_AXIS_X,
    TP_DETECT_AXIS_Y
} TPDetectAxis;

enum {
    TOUCH_OFF = 0,
    TOUCH_ON = 1,
    TOUCH_MAY_OFFNOISE = 2
};

static u16 last_touch_flg = 0;

static u32 TPi_DetectTouch(void)
{
    EXIi_SelectRcnt(EXI_GPIOIF_GPIO);
#ifdef SDK_TEG
    EXIi_SetBitRcnt0H(REG_EXI_RCNT0_H_DIR_R6_MASK, 0);
#endif

    SPI_Wait();
    TP_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(TP_COMMAND_DETECT_TOUCH);
    SPI_DummyWait();
#ifdef SDK_TEG
    SPI_DummyWait();
#endif
    TP_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();

    if (!last_touch_flg) {
        return !(reg_EXI_RCNT0_H & REG_EXI_RCNT0_H_DATA_R6_MASK);
    }

    if (!(reg_EXI_RCNT0_H & REG_EXI_RCNT0_H_DATA_R6_MASK)) {
        return TOUCH_ON;
    }

    TP_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(TP_COMMAND_DETECT_TOUCH);
    SPI_DummyWait();
#ifdef SDK_TEG
    SPI_DummyWait();
#endif
    TP_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();

    if (!(reg_EXI_RCNT0_H & REG_EXI_RCNT0_H_DATA_R6_MASK)) {
        return TOUCH_MAY_OFFNOISE;
    } else {
        return TOUCH_OFF;
    }
}

#define TRY_COUNT 5

#ifdef SDK_TP_AUTO_ADJUST_RANGE
static SPITpValidity TPi_DetectPos(u16 *data, s32 range, TPDetectAxis axis, u16 *density)
#else
static SPITpValidity TPi_DetectPos(u16 *data, s32 range, TPDetectAxis axis)
#endif
{
    s32 i, j, k;
    s32 temp[TRY_COUNT];
    u16 command;
    SPITpValidity validity;

    SDK_NULL_ASSERT(data);
    SDK_ASSERT(range >= 0);

    if (axis == TP_DETECT_AXIS_X) {
        command = TP_COMMAND_SAMPLING_X;
        validity = SPI_TP_VALIDITY_INVALID_X;
    } else {
        command = TP_COMMAND_SAMPLING_Y;
        validity = SPI_TP_VALIDITY_INVALID_Y;
    }

    SPI_Wait();
    TP_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    SPI_SendWait(command);
#ifdef SDK_TEG
    SPI_DummyWait();
#endif

    for (i = 0; i < TRY_COUNT; i++) {
        temp[i] = (SPI_DummyWaitReceive()) << 8;
        temp[i] |= SPI_SendWaitReceive(command);
        temp[i] = (temp[i] & TP_VALID_BIT_MASK) >> TP_VALID_BIT_SHIFT;
    }

    TP_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();
#ifdef SDK_TP_AUTO_ADJUST_RANGE
    s32 maxRange = 0;
    for (i = 0; i < TRY_COUNT - 1; i++) {
        for (j = i + 1; j < TRY_COUNT; j++) {
            s32 range = MATH_ABS(temp[i] - temp[j]);

            if (range > maxRange) {
                maxRange = range;
            }
        }
    }
    *density = maxRange;
#endif
    for (i = 0; i < TRY_COUNT - 2; i++) {
        for (j = i + 1; j < TRY_COUNT - 1; j++) {
            if (MATH_ABS(temp[i] - temp[j]) > range) {
                continue;
            }
            for (k = j + 1; k < TRY_COUNT; k++) {
                if (MATH_ABS(temp[i] - temp[k]) <= range) {
                    *data = (u16)((((temp[i] << 1) + temp[j] + temp[k]) >> 2) & ~0x7);
                    return SPI_TP_VALIDITY_VALID;
                }
            }
        }
    }

    *data = ((temp[0] + temp[4]) >> 1) & ~0x7;

    return validity;
}

#ifdef SDK_TP_AUTO_ADJUST_RANGE
void TP_ExecSampling(SPITpData *data, s32 range, u16 *density)
#else
void TP_ExecSampling(SPITpData *data, s32 range)
#endif
{
    u16 temp_pos;
    u32 temp_touch;
#ifdef SDK_TP_AUTO_ADJUST_RANGE
    u16 density_x, density_y;
    *density = 0;
#endif
    SDK_NULL_ASSERT(data);

    if (range < 0) {
        range = -range;
    }

    temp_touch = TPi_DetectTouch();
    if (temp_touch == TOUCH_OFF) {
        data->e.x = 0;
        data->e.y = 0;
        data->e.touch = SPI_TP_TOUCH_OFF;
        data->e.validity = SPI_TP_VALIDITY_INVALID_XY;
        last_touch_flg = 0;
        return;
    }

#ifdef SDK_TP_AUTO_ADJUST_RANGE
    data->e.validity = TPi_DetectPos(&temp_pos, range, TP_DETECT_AXIS_X, &density_x);
#else
    data->e.validity = TPi_DetectPos(&temp_pos, range, TP_DETECT_AXIS_X);
#endif

    data->e.x = temp_pos;
#ifdef SDK_TP_AUTO_ADJUST_RANGE
    if (TPi_DetectPos(&temp_pos, range, TP_DETECT_AXIS_Y, &density_y) == SPI_TP_VALIDITY_INVALID_Y) {
#else
    if (TPi_DetectPos(&temp_pos, range, TP_DETECT_AXIS_Y) == SPI_TP_VALIDITY_INVALID_Y) {
#endif
        data->e.validity |= SPI_TP_VALIDITY_INVALID_Y;
    }

    data->e.y = temp_pos;

    TP_SPIChangeMode(SPI_TRANSMODE_CONTINUOUS);
    for (int i = 0; i < 12; i++) {
        SPI_DummyWait();
    }
    TP_SPIChangeMode(SPI_TRANSMODE_1BYTE);
    SPI_DummyWait();

    if (temp_touch == TOUCH_MAY_OFFNOISE) {
        data->e.validity = SPI_TP_VALIDITY_INVALID_XY;
    }

    temp_touch = TPi_DetectTouch();

    switch (temp_touch) {
    case TOUCH_MAY_OFFNOISE:
        data->e.touch = SPI_TP_TOUCH_ON;
        data->e.validity = SPI_TP_VALIDITY_INVALID_XY;
        last_touch_flg = 0;
        break;
    case TOUCH_ON:
        data->e.touch = SPI_TP_TOUCH_ON;
        last_touch_flg = 1;
#ifdef SDK_TP_AUTO_ADJUST_RANGE
        *density = (density_x >= density_y) ? density_x : density_y;
#endif
        break;
    case TOUCH_OFF:
        data->e.touch = SPI_TP_TOUCH_OFF;
        last_touch_flg = 0;
        break;
    default:
        OS_TPanic("Illegal Touch Parameter\n");
    }
}
