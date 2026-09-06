#include "nitro/spi/ARM7/mic.h"

#include "nitro/spi/ARM7/spi.h"
#include <nitro/hw/ARM7/mmap_global.h>
#include <nitro/os/common/interrupt.h>
#include <nitro/os/common/systemWork.h>
#include <nitro/os/common/timer.h>
#include <nitro/spi/common/type.h>

#define MIC_MEASURE_HANDLER_COST   0
#define MIC_TIMER_HANDLER_OPTIMIZE 1

static MICWork micw;

static BOOL MicSetTimerValue(u32 value);
static void MIC_TimerHandler(void);
static void MicTimerHandler(void);

void MIC_Init(void)
{
    micw.status = MIC_STATUS_READY;

    for (int i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; i++) {
        micw.command[i] = 0;
    }

    reg_MIC_TMCNT_H &= ~MIC_TMCNT_H_E_MASK;
}

void MIC_AnalyzeCommand(u32 data)
{
    if (data & SPI_PXI_START_BIT) {
        for (int i = 0; i < SPI_PXI_CONTINUOUS_PACKET_MAX; i++) {
            micw.command[i] = 0;
        }
    }
    micw.command[(data & SPI_PXI_INDEX_MASK) >> SPI_PXI_INDEX_SHIFT] = (data & SPI_PXI_DATA_MASK) >> SPI_PXI_DATA_SHIFT;

    if (data & SPI_PXI_END_BIT) {
        u16 command = (micw.command[0] & 0xFF00) >> 8;

        u32 wu32;

        switch (command) {
        case SPI_PXI_COMMAND_MIC_SAMPLING:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_MIC, command, 1, micw.command[0] & 0xFF)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
            }
            OS_GetSystemWork()->mic_sampling_data = 0;
            OS_GetSystemWork()->mic_last_address = (u32)NULL;
            break;

        case SPI_PXI_COMMAND_MIC_AUTO_ON:
            if (micw.status != MIC_STATUS_READY) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            micw.type = micw.command[0] & 0xFF;
            wu32 = (micw.command[1] << 16) | micw.command[2];
            if ((wu32 < HW_MAIN_MEM) || (wu32 >= HW_MAIN_MEM_END)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            micw.buf = wu32;
            wu32 = (micw.command[3] << 16) | micw.command[4];
            if ((u32)micw.buf + wu32 > HW_MAIN_MEM_END) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            micw.size = wu32;
            wu32 = (micw.command[5] << 16) | micw.command[6];
            if (!MicSetTimerValue(wu32)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            micw.index = 0;
            micw.admode = micw.type & SPI_MIC_SAMPLING_TYPE_ADMODE_MASK;
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_MIC, (u32)command, 0)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }
            OS_GetSystemWork()->mic_sampling_data = 0;
            OS_GetSystemWork()->mic_last_address = (u32)NULL;
            micw.status = MIC_STATUS_AUTO_START;
            break;

        case SPI_PXI_COMMAND_MIC_AUTO_OFF:
            if (micw.status != MIC_STATUS_AUTO_SAMPLING) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_MIC, command, 0)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }
            micw.status = MIC_STATUS_AUTO_END;
            reg_MIC_TMCNT_H &= ~MIC_TMCNT_H_E_MASK;
            break;

        case SPI_PXI_COMMAND_MIC_AUTO_ADJUST:
            if (micw.status != MIC_STATUS_AUTO_SAMPLING) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            wu32 = (micw.command[1] << 16) | micw.command[2];
            if (!MicSetTimerValue(wu32)) {
                SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }

            OSIntrMode e = OS_DisableInterrupts();
            reg_MIC_TMCNT_H &= ~MIC_TMCNT_H_E_MASK;
            reg_MIC_TMCNT_L = micw.timerValue;
            reg_MIC_TMCNT_H = (u16)(MIC_TMCNT_H_E_MASK | MIC_TMCNT_H_I_MASK | micw.timerPrescaler);
            OS_RestoreInterrupts(e);

            SPIi_ReturnResult(command, SPI_PXI_RESULT_SUCCESS);
            break;

        default:
            SPIi_ReturnResult(command, SPI_PXI_RESULT_INVALID_COMMAND);
        }
    }
}

static BOOL MicSetTimerValue(u32 value)
{
    if (value < 0x10000) {
        micw.timerPrescaler = OS_TIMER_PRESCALER_1;
        micw.timerValue = 0x10000 - value;
        return TRUE;
    }
    if (value < 0x400000) {
        micw.timerPrescaler = OS_TIMER_PRESCALER_64;
        micw.timerValue = 0x10000 - (value >> 6);
        return TRUE;
    }
    if (value < 0x1000000) {
        micw.timerPrescaler = OS_TIMER_PRESCALER_256;
        micw.timerValue = 0x10000 - (value >> 8);
        return TRUE;
    }
    if (value < 0x4000000) {
        micw.timerPrescaler = OS_TIMER_PRESCALER_1024;
        micw.timerValue = 0x10000 - (value >> 10);
        return TRUE;
    }
    return FALSE;
}

void MIC_ExecuteProcess(SPIEntry *entry)
{
    switch (entry->process) {
    case SPI_PXI_COMMAND_MIC_SAMPLING: {
        OSIntrMode e = OS_DisableInterrupts();
        if (!SPIi_CheckException(SPI_DEVICE_TYPE_MIC)) {
            OS_RestoreInterrupts(e);

            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_EXCLUSIVE);
            return;
        }
        SPIi_GetException(SPI_DEVICE_TYPE_MIC);
        OS_RestoreInterrupts(e);
    }

        if ((entry->arg[0] & SPI_MIC_SAMPLING_TYPE_BIT_MASK) == SPI_MIC_SAMPLING_TYPE_12BIT) {
#ifdef SDK_TEG
            u16 temp = MIC_OneTimeSampling12();
#else
#if (SDK_TS_VERSION >= 100)
            u16 temp = MIC_ExecSampling12();
#else
            u16 temp = MIC_OneTimeSampling12();
#endif
#endif
            if (entry->arg[0] & SPI_MIC_SAMPLING_TYPE_SIGNED_MASK) {
                temp ^= 0x8000;
            }

            OS_GetSystemWork()->mic_sampling_data = temp;
            OS_GetSystemWork()->mic_last_address = HW_MIC_SAMPLING_DATA;
        } else {
#ifdef SDK_TEG
            u16 temp = MIC_OneTimeSampling8();
#else
#if (SDK_TS_VERSION >= 100)
            u16 temp = MIC_ExecSampling8();
#else
            u16 temp = MIC_OneTimeSampling8();
#endif
#endif
            if (entry->arg[0] & SPI_MIC_SAMPLING_TYPE_SIGNED_MASK) {
                temp ^= 0x0080;
            }

            OS_GetSystemWork()->mic_sampling_data = temp;
            OS_GetSystemWork()->mic_last_address = HW_MIC_SAMPLING_DATA;
        }

        SPIi_ReturnResult((u16)(entry->process), SPI_PXI_RESULT_SUCCESS);

        SPIi_ReleaseException(SPI_DEVICE_TYPE_MIC);
        break;

    case SPI_PXI_COMMAND_MIC_AUTO_ON:
        if (micw.status == MIC_STATUS_AUTO_START) {
#ifdef SDK_TEG
            MIC_OneTimeSampling12();
#else
#if (SDK_TS_VERSION < 100)
            MIC_OneTimeSampling12();
#endif
#endif

            micw.temporary = 0;
            micw.temp16 = 0;

            OSIntrMode e = OS_DisableInterrupts();

            OS_EnableIrqMask(MIC_IE_TIMER);
            MIC_SetIrqFunction(MIC_IE_TIMER, MIC_TimerHandler);

            MIC_EnableMultipleInterrupt();

            reg_MIC_TMCNT_L = micw.timerValue;
            reg_MIC_TMCNT_H = (u16)(MIC_TMCNT_H_E_MASK | MIC_TMCNT_H_I_MASK | micw.timerPrescaler);

            OS_RestoreInterrupts(e);

            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_SUCCESS);

            micw.status = MIC_STATUS_AUTO_SAMPLING;
        } else {
            SPIi_ReturnResult(entry->process, SPI_PXI_RESULT_ILLEGAL_STATUS);
        }
        break;

    case SPI_PXI_COMMAND_MIC_AUTO_OFF:
        if (micw.status == MIC_STATUS_AUTO_END || micw.status == MIC_STATUS_END_WAIT) {
            reg_MIC_TMCNT_H &= ~MIC_TMCNT_H_E_MASK;

            OSIntrMode e = OS_DisableInterrupts();

            MIC_SetIrqFunction(MIC_IE_TIMER, NULL);

            MIC_DisableMultipleInterrupt();

            OS_RestoreInterrupts(e);

#ifdef SDK_TEG
            MIC_OneTimeSampling12();
#else
#if (SDK_TS_VERSION < 100)
            MIC_OneTimeSampling12();
#endif
#endif

            if (micw.status == MIC_STATUS_AUTO_END) {
                SPIi_ReturnResult(SPI_PXI_COMMAND_MIC_AUTO_OFF, SPI_PXI_RESULT_SUCCESS);
            } else {
                SPIi_ReturnResult(SPI_PXI_COMMAND_MIC_BUFFER_FULL, SPI_PXI_RESULT_SUCCESS);
            }

            micw.status = MIC_STATUS_READY;
        } else {
            if (micw.status == MIC_STATUS_AUTO_END) {
                SPIi_ReturnResult(SPI_PXI_COMMAND_MIC_AUTO_OFF, SPI_PXI_RESULT_ILLEGAL_STATUS);
            } else {
                SPIi_ReturnResult(SPI_PXI_COMMAND_MIC_BUFFER_FULL, SPI_PXI_RESULT_ILLEGAL_STATUS);
            }
        }
        break;
    }
}

static void MIC_TimerHandler(void)
{
#if MIC_MEASURE_HANDLER_COST
    OSTick  begin;
    OSTick  end;

    begin = OS_GetTick();
#endif
    MicTimerHandler();

    OS_SetIrqCheckFlag(MIC_IE_TIMER);

    reg_OS_IF = MIC_IE_TIMER;
#if MIC_MEASURE_HANDLER_COST
    end = OS_GetTick();
    OS_TPrintf("%d us - ", OS_TicksToMicroSeconds(end - begin));
#endif
}

#if MIC_TIMER_HANDLER_OPTIMIZE && defined(SDK_CODE_ARM)
static asm void MicTimerHandler(void)
{
    stmdb sp!, {r4, r5, r6, r7, lr}
    ldr r4, =micw
    ldrh r5, [r4, #MICWork.admode]
    and r0, r5, #SPI_MIC_SAMPLING_TYPE_FILTER_MASK
    cmp r0, #SPI_MIC_SAMPLING_TYPE_FILTER_OFF
    ldrh r6, [r4, #MICWork.temp16]
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    bne @ldrneh1
    b @ldrneh2
@ldrneh1:
    ldrh r7, [r4, #MICWork.temporary]
@ldrneh2:
    beq @ldreq1
    b @ldreq2
@ldreq1:
    ldr r7, =0xFFFF
@ldreq2:
#else
    ldrneh r7, [r4, #MICWork.temporary]
    ldreq r7, =0xFFFF
#endif
    bl SPIi_CheckEntry
    cmp r0, #0
    bne @sampling_end
    mov r0, #SPI_DEVICE_TYPE_MIC
    bl SPIi_CheckException
    cmp r0, #0
    beq @sampling_end
    and r0, r5, #SPI_MIC_SAMPLING_TYPE_BIT_MASK
    cmp r0, #SPI_MIC_SAMPLING_TYPE_12BIT
    bne @8bit_sampling
    
@12bit_sampling:
    bl MIC_ExecSampling12
    tst r5, #SPI_MIC_SAMPLING_TYPE_SIGNED_MASK
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @moveq1
    b @moveq2
@moveq1:
    mov r7, r0
@moveq2:
    bne @eorne1
    b @eorne2
@eorne1:
    eor r7, r0, #0x8000
@eorne2:
#else
    moveq r7, r0
    eorne r7, r0, #0x8000
#endif
    b @sampling_end

@8bit_sampling:
    bl MIC_ExecSampling8
    tst r5, #SPI_MIC_SAMPLING_TYPE_SIGNED_MASK
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @moveq3
    b @moveq4
@moveq3:
    mov r7, r0
@moveq4:
    bne @eorne3
    b @eorne4
@eorne3:
    eor r7, r0, #0x80
@eorne4:
#else
    moveq r7, r0
    eorne r7, r0, #0x80
#endif

@sampling_end:
    and r0, r5, #SPI_MIC_SAMPLING_TYPE_BIT_MASK
    ldr r3, =HW_MAIN_MEM_SYSTEM
    ldr r1, [r4, #MICWork.index]
    cmp r0, #SPI_MIC_SAMPLING_TYPE_12BIT
    bne @8bit_store_memory

@12bit_store_memory:
    ldr r2, [r4, #MICWork.buf]
    strh r7, [r2, r1]!
    str r2, [r3, #OSSystemWork.mic_last_address]
    add r3, r3, #OSSystemWork.mic_sampling_data
    strh r7, [r3]
    add r1, r1, #2
    b @store_memory_end

@8bit_store_memory:
    and r7, r7, #0xff
    tst r1, #1
    bne @8bit_store_memory_next
    mov r6, r7
    add r1, r1, #1
    b @store_memory_end

@8bit_store_memory_next:
    orr r0, r6, r7, lsl #8
    ldr r2, [r4, #MICWork.buf]
    sub r1, r1, #1
    strh r0, [r2, r1]!
    str r2, [r3, #OSSystemWork.mic_last_address]
    add r3, r3, #OSSystemWork.mic_sampling_data
    strh r0, [r3]
    add r1, r1, #2

@store_memory_end:
    strh r6, [r4, #MICWork.temp16]
    strh r7, [r4, #MICWork.temporary]
    ldr r0, [r4, #MICWork.size]
    cmp r1, r0
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    bcs @movcs1
    b @movcs2
@movcs1:
    mov r1, #0
@movcs2:
#else
    movcs r1, #0
#endif
    str r1, [r4, #MICWork.index]
    bcc @loop_check_end
    ldrh r0, [r4, #MICWork.type]
    and r0, r0, #SPI_MIC_SAMPLING_TYPE_LOOP_MASK
    cmp r0, #SPI_MIC_SAMPLING_TYPE_LOOP_ON
    bne @loop_off

@loop_on:
    mov r0, #SPI_PXI_COMMAND_MIC_BUFFER_FULL
    mov r1, #SPI_PXI_RESULT_SUCCESS
    bl SPIi_ReturnResult
    b @loop_check_end

@loop_off:
    mov r0, #SPI_DEVICE_TYPE_MIC
    mov r1, #SPI_PXI_COMMAND_MIC_AUTO_OFF
    mov r2, #0
    bl SPIi_SetEntry
    cmp r0, #0
    bne @loop_off_next

    mov r0, #SPI_PXI_COMMAND_MIC_BUFFER_FULL
    mov r1, #SPI_PXI_RESULT_EXCLUSIVE
    bl SPIi_ReturnResult
    b @loop_check_end

@loop_off_next:
    mov r0, #MIC_STATUS_END_WAIT
    str r0, [r4, #MICWork.status]
    ldr r1, =REG_TM3CNT_H_ADDR
    ldrh r0, [r1]
    bic r0, r0, #MIC_TMCNT_H_E_MASK
    strh r0, [r1]

@loop_check_end:
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
}
#else
static void MicTimerHandler(void)
{
    if ((micw.admode & SPI_MIC_SAMPLING_TYPE_FILTER_MASK) == SPI_MIC_SAMPLING_TYPE_FILTER_OFF) {
        micw.temporary = 0xFFFF;
    }

    if (!SPIi_CheckEntry() && SPIi_CheckException(SPI_DEVICE_TYPE_MIC)) {
        if ((micw.admode & SPI_MIC_SAMPLING_TYPE_BIT_MASK) == SPI_MIC_SAMPLING_TYPE_12BIT) {
            micw.temporary = MIC_ExecSampling12();
            if (micw.admode & SPI_MIC_SAMPLING_TYPE_SIGNED_MASK) {
                micw.temporary ^= 0x8000;
            }
        } else {
            micw.temporary = MIC_ExecSampling8();
            if (micw.admode & SPI_MIC_SAMPLING_TYPE_SIGNED_MASK) {
                micw.temporary ^= 0x80;
            }
        }
    }

    if ((micw.admode & SPI_MIC_SAMPLING_TYPE_BIT_MASK) == SPI_MIC_SAMPLING_TYPE_12BIT) {
        ((vu16 *)micw.buf)[micw.index / 2] = micw.temporary;
        OS_GetSystemWork()->mic_sampling_data = micw.temporary;
        OS_GetSystemWork()->mic_last_address = (u32)(micw.buf) + micw.index;
        micw.index += 2;
    } else {
        if (micw.index % 2) {
            micw.temp16 |= (micw.temporary << 8) & 0xFF00;
            ((vu16 *)micw.buf)[micw.index / 2] = micw.temp16;
            OS_GetSystemWork()->mic_sampling_data = micw.temp16;
            OS_GetSystemWork()->mic_last_address = (u32)(micw.buf) + micw.index;
        } else {
            micw.temp16 = micw.temporary & 0xFF;
        }
        micw.index++;
    }

    if (micw.index >= micw.size) {
        micw.index = 0;
        if ((micw.type & SPI_MIC_SAMPLING_TYPE_LOOP_MASK) == SPI_MIC_SAMPLING_TYPE_LOOP_ON) {
            SPIi_ReturnResult(SPI_PXI_COMMAND_MIC_BUFFER_FULL, SPI_PXI_RESULT_SUCCESS);
        } else {
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_MIC, SPI_PXI_COMMAND_MIC_AUTO_OFF, 0)) {
                SPIi_ReturnResult(SPI_PXI_COMMAND_MIC_BUFFER_FULL, SPI_PXI_RESULT_EXCLUSIVE);
            } else {
                micw.status = MIC_STATUS_END_WAIT;
                reg_MIC_TMCNT_H &= ~MIC_TMCNT_H_E_MASK;
            }
        }
    }
}
#endif

