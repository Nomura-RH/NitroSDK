#include <nitro.h>
#include <nitro/code32.h>

extern void NitroSpMain(void);
extern void OS_IrqHandler(void);

static void do_autoload(void);
static void detect_main_memory_size(void);
void _start(void);

extern void *const _start_ModuleParams[];

void _start_AutoloadDoneCallback(void *argv[]);

extern void __call_static_initializers(void);
extern void _fp_init(void);

extern void SDK_IRQ_STACKSIZE(void);
extern void SDK_AUTOLOAD_START(void);
extern void SDK_AUTOLOAD_LIST(void);
extern void SDK_AUTOLOAD_LIST_END(void);
extern void SDK_STATIC_BSS_START(void);
extern void SDK_STATIC_BSS_END(void);

#define IPL_PARAM_CARD_ROM_HEADER      0x023FE940
#define IPL_PARAM_DOWNLOAD_PARAMETER   0x023FE904

#define INITi_Initial_Stack   0x100

SDK_WEAK_SYMBOL asm void _start(void)
{
    mov r12, #HW_REG_BASE
    str r12, [r12, #REG_IME_OFFSET]
    ldr r1, =SDK_STATIC_BSS_END
    mov r0, #HW_PRV_WRAM
    cmp r0, r1
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    bpl @do_mov
    b @skip_mov
@do_mov:
    mov r1, r0
@skip_mov:
#else
    movpl r1, r0
#endif
    ldr r2, =HW_PRV_WRAM_END - INITi_Initial_Stack
    mov r0, #0
@1:
    cmp r1, r2
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    blt @do_stmia
    b @skip_stmia
@do_stmia:
    stmia r1!, {r0}
@skip_stmia:
#else
    stmltia r1!, {r0}
#endif
    blt @1
    mov r0, #HW_PSR_SVC_MODE
    msr cpsr_c, r0
    ldr sp, =HW_PRV_WRAM_SVC_STACK_END
    mov r0, #HW_PSR_IRQ_MODE
    msr cpsr_c, r0
    ldr r0, =HW_PRV_WRAM_IRQ_STACK_END
    mov sp, r0
    ldr r1, =SDK_IRQ_STACKSIZE
    sub r1, r0, r1
    mov r0, #0x1F
    msr cpsr_cxsf, r0
    sub sp, r1, #4
    ldr r0, =IPL_PARAM_CARD_ROM_HEADER
    ldr r1, =HW_CARD_ROM_HEADER
    add r2, r1, #HW_CARD_ROM_HEADER_SIZE
@1_1:
    ldr r3, [r0], #4
    str r3, [r1], #4
    cmp r1, r2
    bmi @1_1
    ldr r0, =IPL_PARAM_DOWNLOAD_PARAMETER
    add r2, r1, #HW_DOWNLOAD_PARAMETER_SIZE
@1_2:
    ldr r3, [r0], #4
    str r3, [r1], #4
    cmp r1, r2
    bmi @1_2
    bl do_autoload
    ldr r0, =_start_ModuleParams
    ldr r1, [r0, #0xC]
    ldr r2, [r0, #0x10]
    mov r0, #0
@2:
    cmp r1, r2
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
	bcc @do_str
    b @skip_str
@do_str:
    str r0, [r1], #4
@skip_str:
#else
    strcc r0, [r1], #4
#endif
    bcc @2
    bl detect_main_memory_size
    ldr r1, =HW_INTR_VECTOR_BUF
    ldr r0, =OS_IrqHandler
    str r0, [r1]
#ifndef SDK_NOINIT
    bl _fp_init
    bl NitroSpStartUp
    bl __call_static_initializers
#endif
    ldr r1, =NitroSpMain
    ldr lr, =HW_RESET_VECTOR
    bx r1
}

void *const _start_ModuleParams[] = {
	(void *)SDK_AUTOLOAD_LIST,
	(void *)SDK_AUTOLOAD_LIST_END,
	(void *)SDK_AUTOLOAD_START,
	(void *)SDK_STATIC_BSS_START,
	(void *)SDK_STATIC_BSS_END,
};

static asm void do_autoload(void)
{
#define ptable          r0
#define infop           r1
#define infop_end       r2
#define src             r3
#define dest            r4
#define dest_size       r5
#define dest_end        r6
#define tmp             r7
	ldr ptable, = _start_ModuleParams
	ldr infop, [ptable, #0]
	ldr infop_end, [ptable, #4]
	ldr src, [ptable, #8]
@2:
	cmp infop, infop_end
	beq @skipout

    ldr dest, [infop], #4
    ldr dest_size, [infop], #4
    add dest_end, dest, dest_size
@1:
	cmp dest, dest_end
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
	bmi @ldrmi1
    b @ldrmi2
@ldrmi1:
    ldr tmp, [src], #4
@ldrmi2:
    bmi @strmi1
    b @strmi2
@strmi1:
    str tmp, [dest], #4
@strmi2:
#else
	ldrmi tmp, [src], #4
	strmi tmp, [dest], #4
#endif
	bmi @1

    ldr dest_size, [infop], #4
    add dest_end, dest, dest_size
	mov tmp, #0
@3:
	cmp dest, dest_end
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
	bcc @strcc1
    b @strcc2
@strcc1:
    str tmp, [dest], #4
@strcc2:
#else
	strcc tmp, [dest], #4
#endif
	bcc @3
    beq @2
@skipout:
    b _start_AutoloadDoneCallback
}

SDK_WEAK_SYMBOL asm void _start_AutoloadDoneCallback (void *argv[])
{
	bx lr
}

#define OSi_IMAGE_DIFFERENCE 0x400000

static asm void detect_main_memory_size(void)
{
    mov r0, #OS_CONSOLE_SIZE_4MB
    mov r1, #0
    ldr r2, =HW_MMEMCHECKER_SUB
    sub r3, r2, #OSi_IMAGE_DIFFERENCE
@1:
    strh r1, [r2]
    ldrh r12, [r3]
    cmp r1, r12
#ifdef SP1P3_BUG_FOR_CONDITIONAL_ASM_INSTRUCTIONS
    bne @do_mov
    b @skip_mov
@do_mov:
    mov r0, #OS_CONSOLE_SIZE_8MB
@skip_mov:
#else
    movne r0, #OS_CONSOLE_SIZE_8MB
#endif
    bne @2
    add r1, r1, #1
    cmp r1, #2
    bne @1
@2:
    strh r0, [r2]
    bx lr
}

SDK_WEAK_SYMBOL void NitroSpStartUp(void)
{
}
