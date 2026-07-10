#include "nitro/exi/ARM7/genPort.h"

asm void EXIi_SetBitRcnt0L(u16 mask, u16 data)
{
    ldr r2, =REG_RCNT0_L_ADDR
    mvn r3, r0
    ldrh r0, [r2]
    and r0, r3, r0
    orr r0, r1, r0
    strh r0, [r2]
    bx lr
}

asm void EXIi_SelectRcnt(EXIGpioIF type)
{
    ldr r12, =EXIi_SetBitRcnt0L
    mov r0, r0, lsl #0x10
    mov r1, r0, lsr #0x10
    mov r0, #0xc000
    bx r12
}
