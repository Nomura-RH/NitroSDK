
#include "nitro/rtc/ARM7/gpio.h"
asm void RTCi_GpioStart(void)
{
    mov r12, #0x4000000
    add r12, r12, #0x138
    ldrh r0, [r12]
    bic r0, r0, #0x77
    orr r0, r0, #0x72
    strh r0, [r12]
    mov r3, #2

_03805788:
    subs r3, r3, #1
    bne _03805788

    bic r0, r0, #4
    orr r0, r0, #4
    strh r0, [r12]
    mov r3, #2

_038057A0:
    subs r3, r3, #1
    bne _038057A0
    bx lr
}

asm void RTCi_GpioEnd(void)
{
    mov r12, #0x4000000
    add r12, r12, #0x138
    mov r3, #2

_038057B8:
    subs r3, r3, #1
    bne _038057B8
    ldrh r0, [r12]
    bic r0, r0, #4
    orr r0, r0, #0
    strh r0, [r12]
    mov r3, #2

_038057D4:
    subs r3, r3, #1
    bne _038057D4
    bx lr
}

asm void RTCi_GpioSendCommand(u32 param1, u32 param2)
{
    mov r12, #0x4000000
    add r12, r12, #0x138
    orr r1, r0, r1
    ldrh r0, [r12]
    bic r0, r0, #0x77
    orr r0, r0, #0x74
    mov r2, #0

_038057FC:
    bic r0, r0, #3
    orr r0, r0, #0
    mov r3, #1
    tst r3, r1, lsr r2
    bne _03805814
    b _03805818

_03805814:
    mov r3, #1

_03805818:
    beq _03805820
    b _03805824

_03805820:
    mov r3, #0

_03805824:
    orr r0, r0, r3
    strh r0, [r12]
    mov r3, #9

_03805830:
    subs r3, r3, #1
    bne _03805830
    bic r0, r0, #2
    orr r0, r0, #2
    strh r0, [r12]
    mov r3, #9

_03805848:
    subs r3, r3, #1
    bne _03805848
    add r2, r2, #1
    cmp r2, #8
    bne _038057FC
    bx lr
}


asm void RTCi_GpioSendData(void *param1, u32 param2)
{
    mov r12, #0x4000000
    add r12, r12, #0x138

_03805868:
    stmdb sp!, {r0, r1}
    tst r0, #1
    beq _03805878
    b _0380587C

_03805878:
    ldrh r1, [r0]

_0380587C:
    bne _03805884
    b _03805888

_03805884:
    ldrh r1, [r0, #-1]

_03805888:
    bne _03805890
    b _03805894

_03805890:
    mov r1, r1, lsr #8

_03805894:
    ldrh r0, [r12]
    bic r0, r0, #0x77
    orr r0, r0, #0x74
    mov r2, #0

_038058A4:
    bic r0, r0, #3
    orr r0, r0, #0
    mov r3, #1
    tst r3, r1, lsr r2
    bne _038058BC
    b _038058C0

_038058BC:
    mov r3, #1

_038058C0:
    beq _038058C8
    b _038058CC

_038058C8:
    mov r3, #0

_038058CC:
    orr r0, r0, r3
    strh r0, [r12]
    mov r3, #9

_038058D8:
    subs r3, r3, #1
    bne _038058D8
    bic r0, r0, #2
    orr r0, r0, #2
    strh r0, [r12]
    mov r3, #9

_038058F0:
    subs r3, r3, #1
    bne _038058F0
    add r2, r2, #1
    cmp r2, #8
    bne _038058A4
    ldmia sp!, {r0, r1}
    add r0, r0, #1
    subs r1, r1, #1
    bne _03805868
    bx lr
}

asm void RTCi_GpioReceiveData(void *param1, u32 param2)
{
    mov r12, #0x4000000
    add r12, r12, #0x138

_03805920:
    stmdb sp!, {r0, r1}
    ldrh r0, [r12]
    bic r0, r0, #0x77
    orr r0, r0, #0x64
    mov r2, #0
    mov r1, #0

_03805938:
    bic r0, r0, #3
    orr r0, r0, #0
    strh r0, [r12]
    mov r3, #9

_03805948:
    subs r3, r3, #1
    bne _03805948
    ldrh r0, [r12]
    and r3, r0, #1
    cmp r3, #1
    beq _03805964
    b _03805968

_03805964:
    mov r3, #0x80

_03805968:
    bne _03805970
    b _03805974

_03805970:
    mov r3, #0

_03805974:
    orr r2, r3, r2, lsr #1
    bic r0, r0, #2
    orr r0, r0, #2
    strh r0, [r12]
    mov r3, #9

_03805988:
    subs r3, r3, #1
    bne _03805988
    add r1, r1, #1
    cmp r1, #8
    bne _03805938
    ldmia sp!, {r0, r1}
    tst r0, #1
    beq _038059C0
    ldrh r3, [r0, #-1]
    bic r3, r3, #0xff00
    mov r2, r2, lsl #0x8
    orr r3, r2, r3
    strh r3, [r0, #-1]
    b _038059D0

_038059C0:
    ldrh r3, [r0]
    bic r3, r3, #0xff
    orr r3, r3, r2
    strh r3, [r0]

_038059D0:
    add r0, r0, #1
    subs r1, r1, #1
    bne _03805920
    bx lr
}
