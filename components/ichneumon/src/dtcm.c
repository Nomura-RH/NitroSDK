
#include <nitro.h>
#include <nitro/os/common/system.h>

extern void SDK_AUTOLOAD_START(void);
extern void SDK_AUTOLOAD_LIST(void);
extern void SDK_AUTOLOAD_LIST_END(void);
extern void SDK_AUTOLOAD_MAIN_BSS_END(void);

asm void WVR_ShelterExtWram(void)
{
    stmdb sp!, {r4, r5, r6, lr}

    // could be any of SDK_AUTOLOAD_START, SDK_STATIC_BSS_END, SDK_STATIC_BSS_START, SDK_STATIC_DATA_END, SDK_STATIC_DATA_START, SDK_STATIC_END, SDK_STATIC_SINIT_END, SDK_STATIC_TEXT_END
    // I assume it is SDK_AUTOLOAD_START because of the following two fields.
    ldr r6, =SDK_AUTOLOAD_START
    ldr r2, =SDK_AUTOLOAD_LIST
    ldr r0, =SDK_AUTOLOAD_LIST_END

    b _027E0094

_027E0014:
    ldmia r2, {r1, r4, r5}
    cmp r1, #0x06000000
    add r2, r2, #12
    bne _027E0090
    ldr r1, =SDK_AUTOLOAD_MAIN_BSS_END
    ldr r0, =0x03809990
    add r3, r4, r5
    str r1, [r0, #4]
    ldr r2, =0x027f9eec
    add r1, r1, r3
    str r3, [r0, #12]
    cmp r2, r1
    beq _027E004C
    bl OS_Terminate

_027E004C:
    ldr r0, =0x03809990
    mov r1, #0
    ldr r2, [r0, #4]
    b _027E0068

_027E005C:
    ldr r0, [r6], #4
    add r1, r1, #1
    str r0, [r2], #4

_027E0068:
    cmp r1, r4, lsr #2
    bcc _027E005C
    mov r1, #0
    mov r0, r1
    b _027E0084

_027E007C:
    str r0, [r2], #4
    add r1, r1, #1

_027E0084:
    cmp r1, r5, lsr #2
    bcc _027E007C
    b _027E009C

_027E0090:
    add r6, r6, r4

_027E0094:
    cmp r2, r0
    bne _027E0014

_027E009C:
    ldmia sp!, {r4, r5, r6, lr}
    bx lr
}
