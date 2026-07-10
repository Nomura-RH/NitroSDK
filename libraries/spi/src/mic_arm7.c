
#include <nitro/hw/ARM7/mmap_global.h>
#include <nitro/os/common/systemWork.h>
#include <nitro/spi/ARM7/spi.h>
#include <nitro/spi/common/type.h>

static struct {
    u16 unk_0[0x10];
    u32 unk_20;
    u16 unk_24;
    u16 unk_26;
    u32 unk_28;
    u32 unk_2c;
    u32 unk_30;
    u16 unk_34;
    u16 unk_36;
    u16 unk_38;
    u16 unk_3a;
} micw;

static BOOL MicSetTimerValue(u32 param1);
static void MIC_TimerHandler(void);
static void MicTimerHandler(void);

static u8 SPI_DummyWaitReceive(void);

void MIC_Init(void)
{
    s32 i;

    micw.unk_20 = 0;

    for (i = 0; i < 0x10; ++i) {
        micw.unk_0[i] = 0;
    }
    
    reg_OS_TM3CNT_H &= ~0x80;
}

void MIC_AnalyzeCommand(u32 data)
{
    if (data & 0x02000000) {
        s32 i;

        for (i = 0; i < 0x10; ++i) {
            micw.unk_0[i] = 0;
        }
    }
    micw.unk_0[(data & 0xf0000) >> 16] = (u16)((data & 0xffff) >> 0);

    if (data & 0x1000000) {
        u16 v0;
        u32 v1;

        v0 = (u16)((micw.unk_0[0] & 0xff00) >> 8);

        switch (v0) {
        case SPI_PXI_COMMAND_MIC_SAMPLING:
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_MIC, (u32)v0, 1, (u32)(micw.unk_0[0] & 0xff))) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_EXCLUSIVE);
            }
            OS_GetSystemWork()->mic_sampling_data = 0;
            OS_GetSystemWork()->mic_last_address = (u32)NULL;
            break;
        case SPI_PXI_COMMAND_MIC_AUTO_ON:
            if (micw.unk_20 != 0) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            micw.unk_24 = (u16)(micw.unk_0[0] & 0xff);
            v1 = (((u32)micw.unk_0[1] << 16) | (u32)micw.unk_0[2]);
            if ((v1 < HW_MAIN_MEM) || (v1 >= HW_MAIN_MEM_END)) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            micw.unk_28 = v1;
            v1 = (((u32)micw.unk_0[3] << 16) | (u32)micw.unk_0[4]);
            if (((u32)micw.unk_28 + v1) > HW_MAIN_MEM_END) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            micw.unk_30 = v1;
            v1 = (((u32)micw.unk_0[5] << 16) | (u32)micw.unk_0[6]);
            if (!MicSetTimerValue(v1)) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            micw.unk_2c = 0;
            micw.unk_26 = (u16)(micw.unk_24 & SPI_MIC_SAMPLING_TYPE_ADMODE_MASK);
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_MIC, (u32)v0, 0)) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }
            OS_GetSystemWork()->mic_sampling_data = 0;
            OS_GetSystemWork()->mic_last_address = (u32)NULL;
            micw.unk_20 = 1;
            break;

        case SPI_PXI_COMMAND_MIC_AUTO_OFF:
            if (micw.unk_20 != 2) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            if (!SPIi_SetEntry(SPI_DEVICE_TYPE_MIC, (u32)v0, 0)) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_EXCLUSIVE);
                return;
            }
            micw.unk_20 = 3;
            reg_OS_TM3CNT_H &= ~0x80;
            break;

        case SPI_PXI_COMMAND_MIC_AUTO_ADJUST:
            if (micw.unk_20 != 2) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_ILLEGAL_STATUS);
                return;
            }
            v1 = (((u32)micw.unk_0[1] << 16) | (u32)micw.unk_0[2]);
            if (!MicSetTimerValue(v1)) {
                SPIi_ReturnResult(v0, SPI_PXI_RESULT_INVALID_PARAMETER);
                return;
            }
            {
                OSIntrMode e = OS_DisableInterrupts();
                reg_OS_TM3CNT_H &= ~0x80;
                reg_OS_TM3CNT_L = micw.unk_34;
                reg_OS_TM3CNT_H = (u16)(0xc0 | micw.unk_36);
                (void)OS_RestoreInterrupts(e);
            }

            SPIi_ReturnResult(v0, SPI_PXI_RESULT_SUCCESS);
            break;

        default:
            SPIi_ReturnResult(v0, SPI_PXI_RESULT_INVALID_COMMAND);
        }
    }
}

static asm BOOL MicSetTimerValue(u32 param1)
{
    cmp r0, #0x10000
    bcs _03802634
    ldr r1, =micw
    mov r2, #0
    strh r2, [r1, #0x36]
    rsb r0, r0, #0x10000
    strh r0, [r1, #0x34]
    mov r0, #1
    bx lr

_03802634:
    cmp r0, #0x400000
    bcs _0380265C
    ldr r1, =micw
    mov r3, #1
    mov r0, r0, lsr #6
    rsb r2, r0, #0x10000
    strh r3, [r1, #0x36]
    mov r0, r3
    strh r2, [r1, #0x34]
    bx lr

_0380265C:
    cmp r0, #0x1000000
    bcs _03802684
    ldr r1, =micw
    mov r2, #2
    mov r0, r0, lsr #8
    strh r2, [r1, #0x36]
    rsb r0, r0, #0x10000
    strh r0, [r1, #0x34]
    mov r0, #1
    bx lr

_03802684:
    cmp r0, #0x4000000
    movcs r0, #0
    bxcs lr
    ldr r1, =micw
    mov r2, #3
    mov r0, r0, lsr #0xa
    strh r2, [r1, #0x36]
    rsb r0, r0, #0x10000
    strh r0, [r1, #0x34]
    mov r0, #1
    bx lr
}

asm void MIC_ExecuteProcess(SPIMessage *param1)
{
    stmdb sp!, {r3, r4, r5, lr}
    mov r4, r0
    ldr r2, [r4, #4]
    cmp r2, #0x40
    beq _038026DC
    cmp r2, #0x41
    beq _0380278C
    cmp r2, #0x42
    beq _03802824
    b _038028C0

_038026DC:
    bl OS_DisableInterrupts
    mov r5, r0
    mov r0, #2
    bl SPIi_CheckException
    cmp r0, #0
    bne _03802714
    mov r0, r5
    bl OS_RestoreInterrupts
    ldr r0, [r4, #4]
    mov r1, #4
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    b _038028C0

_03802714:
    mov r0, #2
    bl SPIi_GetException
    mov r0, r5
    bl OS_RestoreInterrupts
    ldr r0, [r4, #8]
    and r0, r0, #1
    cmp r0, #1
    bne _03802748
    bl MIC_ExecSampling12
    ldr r1, [r4, #8]
    tst r1, #2
    eorne r0, r0, #0x8000
    b _03802758

_03802748:
    bl MIC_ExecSampling8
    ldr r1, [r4, #8]
    tst r1, #2
    eorne r0, r0, #0x80

_03802758:
    ldr r1, =0x027fff94
    movne r0, r0, lsl #0x10
    movne r0, r0, lsr #0x10
    strh r0, [r1, #0]
    str r1, [r1, #-4]
    ldr r0, [r4, #4]
    mov r1, #0
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    mov r0, #2
    bl SPIi_ReleaseException
    b _038028C0

_0380278C:
    ldr r0, =micw
    ldr r1, [r0, #0x20]
    cmp r1, #1
    bne _03802810
    mov r1, #0
    strh r1, [r0, #0x3a]
    strh r1, [r0, #0x38]
    bl OS_DisableInterrupts
    mov r5, r0
    mov r0, #0x40
    bl OS_EnableIrqMask
    ldr r1, =MIC_TimerHandler
    mov r0, #0x40
    bl MIC_SetIrqFunction
    bl MIC_EnableMultipleInterrupt
    ldr r1, =micw
    ldr r2, =REG_TM3CNT_L_ADDR
    ldrh r3, [r1, #0x34]
    mov r0, r5
    strh r3, [r2]
    ldrh r1, [r1, #0x36]
    orr r1, r1, #0xc0
    strh r1, [r2, #2]
    bl OS_RestoreInterrupts
    ldr r0, [r4, #4]
    mov r1, #0
    mov r0, r0, lsl #0x10
    mov r0, r0, lsr #0x10
    bl SPIi_ReturnResult
    ldr r0, =micw
    mov r1, #2
    str r1, [r0, #0x20]
    b _038028C0

_03802810:
    mov r0, r2, lsl #0x10
    mov r0, r0, lsr #0x10
    mov r1, #3
    bl SPIi_ReturnResult
    b _038028C0

_03802824:
    ldr r0, =micw
    ldr r1, [r0, #0x20]
    sub r0, r1, #3
    cmp r0, #1
    bhi _038028A0
    ldr r1, =REG_TM3CNT_H_ADDR
    ldrh r0, [r1]
    bic r0, r0, #0x80
    strh r0, [r1]
    bl OS_DisableInterrupts
    mov r4, r0
    mov r0, #0x40
    mov r1, #0
    bl MIC_SetIrqFunction
    bl MIC_DisableMultipleInterrupt
    mov r0, r4
    bl OS_RestoreInterrupts
    ldr r0, =micw
    mov r1, #0
    ldr r0, [r0, #0x20]
    cmp r0, #3
    bne _03802888
    mov r0, #0x42
    bl SPIi_ReturnResult
    b _03802890

_03802888:
    mov r0, #0x51
    bl SPIi_ReturnResult

_03802890:
    ldr r0, =micw
    mov r1, #0
    str r1, [r0, #0x20]
    b _038028C0

_038028A0:
    cmp r1, #3
    mov r1, #3
    bne _038028B8
    mov r0, #0x42
    bl SPIi_ReturnResult
    b _038028C0

_038028B8:
    mov r0, #0x51
    bl SPIi_ReturnResult

_038028C0:
    ldmia sp!, {r3, r4, r5, lr}
    bx lr
}

static asm void MIC_TimerHandler(void)
{
    stmdb sp!, {r3, lr}
    bl MicTimerHandler
    ldr r3, =HW_INTR_CHECK_BUF
    ldr r0, =REG_IF_ADDR
    ldr r2, [r3]
    mov r1, #0x40
    orr r2, r2, #0x40
    str r2, [r3]
    str r1, [r0]
    ldmia sp!, {r3, lr}
    bx lr
}

static asm void MicTimerHandler(void)
{
    stmdb sp!, {r4, r5, r6, r7, lr}
    ldr r4, =micw
    ldrh r5, [r4, #0x26]
    and r0, r5, #4
    cmp r0, #4
    ldrh r6, [r4, #0x38]
    bne _03802930
    b _03802934

_03802930:
    ldrh r7, [r4, #0x3a]

_03802934:
    beq _0380293C
    b _03802940

_0380293C:
    ldr r7, =0xffff

_03802940:
    bl SPIi_CheckEntry
    cmp r0, #0
    bne _038029AC
    mov r0, #2
    bl SPIi_CheckException
    cmp r0, #0
    beq _038029AC
    and r0, r5, #1
    cmp r0, #1
    bne _0380298C
    bl MIC_ExecSampling12
    tst r5, #2
    beq _03802978
    b _0380297C

_03802978:
    mov r7, r0

_0380297C:
    bne _03802984
    b _03802988

_03802984:
    eor r7, r0, #0x8000

_03802988:
    b _038029AC

_0380298C:
    bl MIC_ExecSampling8
    tst r5, #2
    beq _0380299C
    b _038029A0

_0380299C:
    mov r7, r0

_038029A0:
    bne _038029A8
    b _038029AC

_038029A8:
    eor r7, r0, #0x80

_038029AC:
    and r0, r5, #1
    ldr r3, =0x027ffc00
    ldr r1, [r4, #0x2c]
    cmp r0, #1
    bne _038029DC
    ldr r2, [r4, #0x28]
    strh r7, [r2, r1]!
    str r2, [r3, #0x390]
    add r3, r3, #0x394
    strh r7, [r3]
    add r1, r1, #2
    b _03802A14

_038029DC:
    and r7, r7, #0xff
    tst r1, #1
    bne _038029F4
    mov r6, r7
    add r1, r1, #1
    b _03802A14

_038029F4:
    orr r0, r6, r7, lsl #8
    ldr r2, [r4, #0x28]
    sub r1, r1, #1
    strh r0, [r2, r1]!
    str r2, [r3, #0x390]
    add r3, r3, #0x394
    strh r0, [r3]
    add r1, r1, #2

_03802A14:
    strh r6, [r4, #0x38]
    strh r7, [r4, #0x3a]
    ldr r0, [r4, #0x30]
    cmp r1, r0
    bcs _03802A2C
    b _03802A30

_03802A2C:
    mov r1, #0

_03802A30:
    str r1, [r4, #0x2c]
    bcc _03802A98
    ldrh r0, [r4, #0x24]
    and r0, r0, #0x10
    cmp r0, #0x10
    bne _03802A58
    mov r0, #0x51
    mov r1, #0
    bl SPIi_ReturnResult
    b _03802A98

_03802A58:
    mov r0, #2
    mov r1, #0x42
    mov r2, #0
    bl SPIi_SetEntry
    cmp r0, #0
    bne _03802A80
    mov r0, #0x51
    mov r1, #4
    bl SPIi_ReturnResult
    b _03802A98

_03802A80:
    mov r0, #4
    str r0, [r4, #0x20]
    ldr r1, =REG_TM3CNT_H_ADDR
    ldrh r0, [r1]
    bic r0, r0, #0x80
    strh r0, [r1]

_03802A98:
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
}

asm u16 MIC_ExecSampling8(void)
{
    stmdb sp!, {r4, lr}
    ldr r2, =REG_SPICNT_ADDR

_03802AB8:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03802AB8
    ldr r1, =0x8a01
    ldr r0, =REG_SPID_ADDR
    strh r1, [r2]
    mov r1, #0xec
    strh r1, [r0]
    sub r1, r0, #2

_03802ADC:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03802ADC
    bl SPI_DummyWaitReceive
    ldr r2, =0x8201
    ldr r1, =REG_SPICNT_ADDR
    mov r4, r0, lsl #0x18
    strh r2, [r1]
    bl SPI_DummyWaitReceive
    ldr r1, =0x7f80
    orr r0, r0, r4, lsr #0x10
    and r0, r0, r1
    mov r0, r0, lsl #9
    mov r0, r0, lsr #0x10
    ldmia sp!, {r4, lr}
    bx lr
}

static asm u8 SPI_DummyWaitReceive(void)
{
    ldr r0, =REG_SPID_ADDR
    mov r1, #0
    strh r1, [r0]
    sub r1, r0, #2

_03802B40:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03802B40
    ldr r0, =REG_SPID_ADDR
    ldrh r0, [r0]
    and r0, r0, #0xff
    bx lr
}

u16 asm MIC_ExecSampling12(void)
{
    stmdb sp!, {r4, lr}
    ldr r2, =REG_SPICNT_ADDR

_03802B68:
    ldrh r0, [r2]
    tst r0, #0x80
    bne _03802B68
    ldr r1, =0x8a01
    ldr r0, =REG_SPID_ADDR
    strh r1, [r2]
    mov r1, #0xe4
    strh r1, [r0]
    sub r1, r0, #2

_03802B8C:
    ldrh r0, [r1]
    tst r0, #0x80
    bne _03802B8C
    bl SPI_DummyWaitReceive
    ldr r2, =0x8201
    ldr r1, =REG_SPICNT_ADDR
    mov r4, r0, lsl #0x18
    strh r2, [r1]
    bl SPI_DummyWaitReceive
    ldr r1, =0x7ff8
    orr r0, r0, r4, lsr #0x10
    and r0, r0, r1
    mov r0, r0, lsl #0x11
    mov r0, r0, lsr #0x10
    ldmia sp!, {r4, lr}
    bx lr
}

