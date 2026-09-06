#include "nitro/spi/ARM7/mic.h"
#include <nitro/hw/ARM7/ioreg_EXI.h>
#include <nitro/hw/ARM7/ioreg_OS.h>
#include <nitro/os/common/interrupt.h>

#define MIC_INTR_FACTORS_MAX  22
#define MIC_INTR_FACTORS_MASK 0x01DF3FFF

#define MIC_FLEXIBLE_IRQ_HANDLER
#define MIC_UPDATE_IE_IN_IRQ_VECTOR

#ifdef MIC_FLEXIBLE_IRQ_HANDLER
typedef struct MICIntrPrio {
    u32 ieBit;
    u32 tableIndex;
} MICIntrPrio;

static const MICIntrPrio micIntrPrio[MIC_INTR_FACTORS_MAX] = {
    { REG_OS_IE_T3_MASK, REG_OS_IE_T3_SHIFT },
    { REG_OS_IE_WL_MASK, REG_OS_IE_WL_SHIFT },
    { REG_OS_IE_MC_MASK, REG_OS_IE_MC_SHIFT },
    { REG_OS_IE_MI_MASK, REG_OS_IE_MI_SHIFT },
    { REG_OS_IE_IFN_MASK, REG_OS_IE_IFN_SHIFT },
    { REG_OS_IE_T0_MASK, REG_OS_IE_T0_SHIFT },
    { REG_OS_IE_T1_MASK, REG_OS_IE_T1_SHIFT },
    { REG_OS_IE_T2_MASK, REG_OS_IE_T2_SHIFT },
    { REG_OS_IE_D0_MASK, REG_OS_IE_D0_SHIFT },
    { REG_OS_IE_D1_MASK, REG_OS_IE_D1_SHIFT },
    { REG_OS_IE_D2_MASK, REG_OS_IE_D2_SHIFT },
    { REG_OS_IE_D3_MASK, REG_OS_IE_D3_SHIFT },
    { REG_OS_IE_K_MASK, REG_OS_IE_K_SHIFT },
    { REG_OS_IE_I_D_MASK, REG_OS_IE_I_D_SHIFT },
    { REG_OS_IE_HB_MASK, REG_OS_IE_HB_SHIFT },
    { REG_OS_IE_VE_MASK, REG_OS_IE_VE_SHIFT },
    { REG_OS_IE_VB_MASK, REG_OS_IE_VB_SHIFT },
    { REG_OS_IE_A7_MASK, REG_OS_IE_A7_SHIFT },
    { REG_OS_IE_PM_MASK, REG_OS_IE_PM_SHIFT },
    { REG_EXI_SIOCNT_START_MASK, REG_EXI_SIOCNT_START_SHIFT },
    { REG_OS_IE_IFE_MASK, REG_OS_IE_IFE_SHIFT },
    { REG_OS_IE_SPI_MASK, REG_OS_IE_SPI_SHIFT }
};
#endif

static MICIntrInfo micIntrInfo;

void MIC_SetIrqFunction(OSIrqMask intrBit, OSIrqFunction function)
{
    for (int i = 0; i < OS_IRQ_TABLE_MAX; i++) {
        if (intrBit & 1) {
            OS_IRQTable[i] = function;
        }
        intrBit >>= 1;
    }
}

void MIC_EnableMultipleInterrupt(void)
{
    u32 handler = *(vu32 *)HW_INTR_VECTOR_BUF;
    if (handler != (u32)MIC_IrqHandler) {
        micIntrInfo.count = 0;
        micIntrInfo.sp = HW_PRV_WRAM_IRQ_STACK_END - MIC_MULTI_INTR_STACK_SIZE;
        micIntrInfo.ie = MIC_IE_TIMER;
        micIntrInfo.handler = handler;
        OSIntrMode enabled = OS_DisableInterrupts();
        *(vu32 *)HW_INTR_VECTOR_BUF = (u32)MIC_IrqHandler;
        OS_RestoreInterrupts(enabled);
    }
}

void MIC_DisableMultipleInterrupt(void)
{
    if (*(vu32 *)HW_INTR_VECTOR_BUF == (u32)MIC_IrqHandler) {
        OSIntrMode enabled = OS_DisableInterrupts();
        *(vu32 *)HW_INTR_VECTOR_BUF = micIntrInfo.handler;
        OS_RestoreInterrupts(enabled);
    }
}

#include <nitro/code32.h>

asm void MIC_IrqHandler(void)
{
    mov r12, #HW_REG_BASE
    add r1, r12, #REG_IME_OFFSET
    ldrh r0, [r1]
    tst r0, r0
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @bxeq1
    b @bxeq2
@bxeq1:
    bx lr
@bxeq2:
#else
    bxeq lr
#endif

    ldr r3, [r12, #REG_IE_OFFSET]
    ldr r1, [r12, #REG_IF_OFFSET]
    ands r2, r1, r3
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @bxeq3
    b @bxeq4
@bxeq3:
    bx lr
@bxeq4:
#else
    bxeq lr
#endif

#ifdef MIC_FLEXIBLE_IRQ_HANDLER
    ldr r0, =MIC_INTR_FACTORS_MASK
    tst r2, r0
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @streq1
    b @streq2
@streq1:
    str r2, [r12, #REG_IF_OFFSET]
@streq2:
    beq @bxeq5
    b @bxeq6
@bxeq5:
    bx lr
@bxeq6:
#else
    streq r2, [r12, #REG_IF_OFFSET]
    bxeq lr
#endif
#endif

    stmdb sp!, {lr}
    mrs r0, spsr
    stmdb sp!, {r0}
    stmdb sp, {sp, lr}^
    sub sp, sp, #8
    mov r0, #HW_PSR_SYS_MODE | HW_PSR_IRQ_DISABLE | HW_PSR_ARM_STATE
    msr cpsr_c, r0
    ldr r1, =OSi_ThreadInfo
    ldrh r0, [r1, #OS_THREADINFO_OFFSET_IRQDEPTH]
    add r0, r0, #1
    strh r0, [r1, #OS_THREADINFO_OFFSET_IRQDEPTH]
    ldr r1, =micIntrInfo
    cmp r0, #1
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @moveq1
    b @moveq2
@moveq1:
    mov r0, sp
@moveq2:
    beq @ldreq1
    b @ldreq2
@ldreq1:
    ldr sp, [r1, #MIC_INTRINFO_OFFSET_SP]
@ldreq2:
    beq @streq3
    b @streq4
@streq3:
    str r0, [r1, #MIC_INTRINFO_OFFSET_SP]
@streq4:
#else
    moveq r0, sp
    ldreq sp, [r1, #MIC_INTRINFO_OFFSET_SP]
    streq r0, [r1, #MIC_INTRINFO_OFFSET_SP]
#endif
    stmdb sp!, {r3}
#ifdef MIC_FLEXIBLE_IRQ_HANDLER
    ldr r1, =micIntrPrio
    ldr r0, [r1, #MICIntrPrio.ieBit]
    tst r0, r2
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    bne @strne1
    b @strne2
@strne1:
    str r0, [r12, #REG_IF_OFFSET]
@strne2:
    bne @ldrne1
    b @ldrne2
@ldrne1:
    ldr r0, [r1, #MICIntrPrio.tableIndex]
@ldrne2:
    bne @ldrne3
    b @ldrne4
@ldrne3:
    ldr r3, =OS_IRQTable
@ldrne4:
    bne @ldrne5
    b @ldrne6
@ldrne5:
    ldr r0, [r3, r0, lsl #2]
@ldrne6:
#else
    strne r0, [r12, #REG_IF_OFFSET]
    ldrne r0, [r1, #MICIntrPrio.tableIndex]
    ldrne r3, =OS_IRQTable
    ldrne r0, [r3, r0, lsl #2]
#endif
    bne @call_user_handler

    mov r3, #1
@loop01:
    ldr r0, [r1, r3, lsl #3]
    tst r0, r2
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @addeq1
    b @addeq2
@addeq1:
    add r3, r3, #1
@addeq2:
#else
    addeq r3, r3, #1
#endif
    beq @loop01

    str r0, [r12, #REG_IF_OFFSET]
    add r0, r1, r3, lsl #3
    ldr r2, [r0, #MICIntrPrio.tableIndex]
    ldr r3, =OS_IRQTable
    ldr r0, [r3, r2, lsl #2]
    ldr r2, =OSi_ThreadInfo
    ldrh r3, [r2, #OS_THREADINFO_OFFSET_IRQDEPTH]
    cmp r3, #1
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @ldreq3
    b @ldreq4
@ldreq3:
    ldr r2, [r1, #MICIntrPrio.ieBit]
@ldreq4:
    beq @streq5
    b @streq6
@streq5:
    str r2, [r12, #REG_IE_OFFSET]
@streq6:
    beq @moveq3
    b @moveq4
@moveq3:
    mov r2, #HW_PSR_SYS_MODE | HW_PSR_ARM_STATE
@moveq4:
    beq @msreq1
    b @msreq2
@msreq1:
    msr cpsr_c, r2
@msreq2:
#else
    ldreq r2, [r1, #MICIntrPrio.ieBit]
    streq r2, [r12, #REG_IE_OFFSET]
    moveq r2, #HW_PSR_SYS_MODE | HW_PSR_ARM_STATE
    msreq cpsr_c, r2
#endif

@call_user_handler:
#ifdef MIC_UPDATE_IE_IN_IRQ_VECTOR
    ldr r1, [r12, #REG_IE_OFFSET]
    stmdb sp!, {r1}
#endif
    add lr, pc, #0
    bx r0
#else
    ldr r0, [r1, #MIC_INTRINFO_OFFSET_IE]
    ands r0, r0, r2
    beq @low_priority_ie

@high_priority_ie:
    mov r1, #1
    mov r2, #0

@loop01:
    ands r3, r0, r1, lsl r2
    addeq r2, r2, #1
    beq @loop01

    str r3, [r12, #REG_IF_OFFSET]
    ldr r1, =OS_IRQTable
    ldr r0, [r1, r2, lsl #2]
    b @call_user_handler

@low_priority_ie:
    mov r1, #1
    mov r0, #0

@loop02:
    ands r3, r2, r1, lsl r0
    addeq r0, r0, #1
    beq @loop02

    str r3, [r12, #REG_IF_OFFSET]
    ldr r1, =OS_IRQTable
    ldr r0, [r1, r0, lsl #2]
    ldr r1, =OSi_ThreadInfo
    ldr r2, =micIntrInfo
    ldrh r3, [r1, #OS_THREADINFO_OFFSET_IRQDEPTH]
    cmp r3, #1
    ldreq r1, [r2, #MIC_INTRINFO_OFFSET_IE]
    streq r1, [r12, #REG_IE_OFFSET]
    moveq r1, #HW_PSR_SYS_MODE | HW_PSR_ARM_STATE
    msreq cpsr_c, r1

@call_user_handler:
#ifdef MIC_UPDATE_IE_IN_IRQ_VECTOR
    ldr r1, [ r12, #REG_IE_OFFSET ]
    stmdb sp!, {r1}
#endif
    add lr, pc, #0
    bx r0
#endif

    mov r0, #HW_PSR_SYS_MODE | HW_PSR_IRQ_DISABLE | HW_PSR_ARM_STATE
    msr cpsr_c, r0
    mov r12, #HW_REG_BASE
#ifdef MIC_UPDATE_IE_IN_IRQ_VECTOR
    ldmia sp!, {r0}
    ldr r1, [r12, #REG_IE_OFFSET]
    eor r2, r0, r1
    and r1, r2, r1
    and r0, r2, r0
    ldmia sp!, {r3}
    orr r3, r3, r1
    bic r3, r3, r0
    str r3, [r12, #REG_IE_OFFSET]
#else
    ldmia sp!, {r0}
    str r0, [r12, #REG_IE_OFFSET]
#endif
    ldr r2, =OSi_ThreadInfo
    ldr r3, =micIntrInfo
    ldrh r0, [r2, #OS_THREADINFO_OFFSET_IRQDEPTH]
    subs r1, r0, #1
    strh r1, [r2, #OS_THREADINFO_OFFSET_IRQDEPTH]
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @moveq5
    b @moveq6
@moveq5:
    mov r0, sp
@moveq6:
    beq @ldreq5
    b @ldreq6
@ldreq5:
    ldr sp, [r3, #MIC_INTRINFO_OFFSET_SP]
@ldreq6:
    beq @streq7
    b @streq8
@streq7:
    str r0, [r3, #MIC_INTRINFO_OFFSET_SP]
@streq8:
#else
    moveq r0, sp
    ldreq sp, [r3, #MIC_INTRINFO_OFFSET_SP]
    streq r0, [r3, #MIC_INTRINFO_OFFSET_SP]
#endif
    mov r0, #HW_PSR_IRQ_MODE | HW_PSR_IRQ_DISABLE | HW_PSR_ARM_STATE
    msr cpsr_c, r0
    ldmia sp, {sp, lr}^
    nop
    add sp, sp, #8
    ldmia sp!, {r0}
    msr spsr_cf, r0
    tst r1, r1
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    beq @ldreq7
    b @ldreq8
@ldreq7:
    ldr r0, =OS_IrqHandler_ThreadSwitch
@ldreq8:
    beq @addeq3
    b @addeq4
@addeq3:
    add lr, pc, #0
@addeq4:
    beq @bxeq7
_03802EBC:
    b @bxeq8
@bxeq7:
    bx r0
@bxeq8:
#else
    ldreq r0, =OS_IrqHandler_ThreadSwitch
    addeq lr, pc, #0
    bxeq r0
#endif
    ldmia sp!, {pc}
}

#include <nitro/codereset.h>
