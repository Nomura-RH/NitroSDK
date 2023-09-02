#include <nitro/mi/stream.h>
#include <nitro/mi/uncompress.h>
#include <nitro/mi/uncomp_stream.h>

void MI_InitUncompContextRL (MIUncompContextRL * context, u8 * dest, const MICompressionHeader * header)
{
    context->destp = dest;
    context->destCount = (s32)header->destSize;
    context->flags = 0;
    context->length = 0;
    context->destTmp = 0;
    context->destTmpCnt = 0;
}

void MI_InitUncompContextLZ (MIUncompContextLZ * context, u8 * dest, const MICompressionHeader * header)
{
    context->destp = dest;
    context->destCount = (s32)header->destSize;
    context->flags = 0;
    context->flagIndex = 0;
    context->length = 0;
    context->lengthFlg = 3;
    context->destTmp = 0;
    context->destTmpCnt = 0;
    context->exFormat = (u8)((header->compParam == 0)? 0U : 1U);
}

void MI_InitUncompContextHuffman (MIUncompContextHuffman * context, u8 * dest, const MICompressionHeader * header)
{
    context->destp = dest;
    context->destCount = (s32)header->destSize;
    context->bitSize = (u8)header->compParam;
    context->treeSize = -1;
    context->treep = &context->tree[0];
    context->destTmp = 0;
    context->destTmpCnt = 0;
    context->srcTmp = 0;
    context->srcTmpCnt = 0;
}

#include <nitro/code32.h>

asm s32 MI_ReadUncompRL8 (register MIUncompContextRL * context, register const u8 * data, register u32 len)
{
    stmfd sp !, {r4 - r8}
    ldr r3, [r0, #MIUncompContextRL.destp]
    ldr r4, [r0, #MIUncompContextRL.destCount]
    ldrb r5, [r0, #MIUncompContextRL.flags]
    ldrh r6, [r0, #MIUncompContextRL.length]
@41:
    cmp r4, #0
    ble @49
    tst r5, #0x80
    bne @43
@42:
    cmp r6, #0
    ble @45
    sub r6, r6, #1
    ldrb r7, [r1], #1
    sub r4, r4, #1
#ifdef MI_USE_STRB
    strb r7, [r3], #1
#else
    swpb r8, r7, [r3]
    add r3, r3, #1
#endif
    subs r2, r2, #1
    beq @49
    b @42
@43:
    cmp r6, #0
    ble @45
    ldrb r7, [r1], #1
@44:
    sub r4, r4, #1
#ifdef MI_USE_STRB
    strb r7, [r3], #1
#else
    swpb r8, r7, [r3]
    add r3, r3, #1
#endif
    subs r6, r6, #1
    bgt @44
    subs r2, r2, #1
    beq @49
@45:
    ldrb r5, [r1], #1
    and r6, r5, #0x7F
    tst r5, #0x80
    addne r6, r6, #2
    add r6, r6, #1
    subs r2, r2, #1
    bne @41
@49:
    str r3, [r0, #MIUncompContextRL.destp]
    str r4, [r0, #MIUncompContextRL.destCount]
    strb r5, [r0, #MIUncompContextRL.flags]
    strh r6, [r0, #MIUncompContextRL.length]
    mov r0, r4
    ldmfd sp !, {r4 - r8}
    bx lr
}

asm s32 MI_ReadUncompRL16 (register MIUncompContextRL * context, register const u8 * data, register u32 len)
{
    stmfd sp !, {r4 - r9}
    ldr r3, [r0, #MIUncompContextRL.destp]
    ldr r4, [r0, #MIUncompContextRL.destCount]
    ldrb r5, [r0, #MIUncompContextRL.flags]
    ldrh r6, [r0, #MIUncompContextRL.length]
    ldrh r7, [r0, #MIUncompContextRL.destTmp]
    ldrb r8, [r0, #MIUncompContextRL.destTmpCnt]
@41:
    cmp r4, #0
    ble @49
    tst r5, #0x80
    bne @44
@42:
    cmp r6, #0
    ble @47
    ldrb r9, [r1], #1
    sub r6, r6, #1
    orr r7, r7, r9, lsl r8
    add r8, r8, #8
    cmp r8, #16
    bne @43
    strh r7, [r3], #2
    mov r7, #0
    mov r8, #0
    sub r4, r4, #2
@43:
    subs r2, r2, #1
    beq @49
    b @42
@44:
    cmp r6, #0
    ble @47
    ldrb r9, [r1], #1
@45:
    orr r7, r7, r9, lsl r8
    add r8, r8, #8
    cmp r8, #16
    bne @46
    strh r7, [r3], #2
    mov r7, #0
    mov r8, #0
    sub r4, r4, #2
@46:
    subs r6, r6, #1
    bgt @45
    subs r2, r2, #1
    beq @49
@47:
    ldrb r5, [r1], #1
    and r6, r5, #0x7F
    tst r5, #0x80
    addne r6, r6, #2
    add r6, r6, #1
    subs r2, r2, #1
    bne @41
@49:
    str r3, [r0, #MIUncompContextRL.destp]
    str r4, [r0, #MIUncompContextRL.destCount]
    strb r5, [r0, #MIUncompContextRL.flags]
    strh r6, [r0, #MIUncompContextRL.length]
    strh r7, [r0, #MIUncompContextRL.destTmp]
    strb r8, [r0, #MIUncompContextRL.destTmpCnt]
    mov r0, r4
    ldmfd sp !, {r4 - r9}
    bx lr
}

asm s32 MI_ReadUncompLZ8 (register MIUncompContextLZ * context, register const u8 * data, register u32 len)
{
    stmfd sp !, {r4 - r11}
    ldr r3, [r0, #MIUncompContextLZ.destp]
    ldr r4, [r0, #MIUncompContextLZ.destCount]
    ldrb r5, [r0, #MIUncompContextLZ.flags]
    ldrb r6, [r0, #MIUncompContextLZ.flagIndex]
    ldr r7, [r0, #MIUncompContextLZ.length]
    ldrb r8, [r0, #MIUncompContextLZ.lengthFlg]
    ldrb r11, [r0, #MIUncompContextLZ.exFormat]
@21:
    cmp r4, #0
    ble @29
    cmp r6, #0
    beq @28
@22
    cmp r2, #0
    beq @29
    tst r5, #0x80
    bne @23
    ldrb r9, [r1], #1
    sub r4, r4, #1
    sub r2, r2, #1
#ifdef MI_USE_STRB
    strb r9, [r3], #1
#else
    swpb r9, r9, [r3]
    add r3, r3, #1
#endif
    b @26
@23:
    cmp r8, #0
    beq @24
    cmp r11, #1
    bne @23_9
    subs r8, r8, #1
    beq @23_7
    cmp r8, #1
    beq @23_6
    ldrb r7, [r1], #1
    tst r7, #0xE0
    beq @23_4
    add r7, r7, #0x10
    mov r8, #0
    b @23_10
@23_4:
    mov r10, #0x110
    tst r7, #0x10
    beq @23_5
    and r7, r7, #0xF
    add r10, r10, #0x1000
    add r7, r10, r7, lsl #16
    b @23_8
@23_5:
    and r7, r7, #0xF
    add r7, r10, r7, lsl #8
    mov r8, #1
    b @23_8
@23_6:
    ldrb r10, [r1], #1
    add r7, r7, r10, lsl #8
    b @23_8
@23_7:
    ldrb r10, [r1], #1
    add r7, r7, r10
    b @23_10
@23_8:
    subs r2, r2, #1
    beq @29
    b @23
@23_9:
    ldrb r7, [r1], #1
    add r7, r7, #0x30
    mov r8, #0
@23_10:
    subs r2, r2, #1
    beq @29
@24:
    and r9, r7, #0xF
    mov r10, r9, lsl #8
    ldrb r9, [r1], #1
    mov r8, #3
    sub r2, r2, #1
    orr r9, r9, r10
    add r9, r9, #1
    movs r7, r7, asr #4
    beq @26
@25:
    ldrb r10, [r3, -r9]
    sub r4, r4, #1
#ifdef MI_USE_STRB
    strb r10, [r3], #1
#else
    swpb r10, r10, [r3]
    add r3, r3, #1
#endif
    subs r7, r7, #1
    bgt @25
@26:
    cmp r4, #0
    beq @29
    mov r5, r5, lsl #1
    subs r6, r6, #1
    bne @22
@28:
    cmp r2, #0
    beq @29
    ldrb r5, [r1], #1
    mov r6, #8
    sub r2, r2, #1
    b @21
@29:
    str r3, [r0, #MIUncompContextLZ.destp]
    str r4, [r0, #MIUncompContextLZ.destCount]
    strb r5, [r0, #MIUncompContextLZ.flags]
    strb r6, [r0, #MIUncompContextLZ.flagIndex]
    str r7, [r0, #MIUncompContextLZ.length]
    strb r8, [r0, #MIUncompContextLZ.lengthFlg]
    strb r11, [r0, #MIUncompContextLZ.exFormat]
    mov r0, r4
    ldmfd sp !, {r4 - r11}
    bx lr
}

asm s32 MI_ReadUncompLZ16 (register MIUncompContextLZ * context, register const u8 * data, register u32 len)
{
    stmfd sp !, {r4 - r12, lr}
    ldr r3, [r0, #MIUncompContextLZ.destp]
    ldr r4, [r0, #MIUncompContextLZ.destCount]
    ldrb r5, [r0, #MIUncompContextLZ.flags]
    ldrb r6, [r0, #MIUncompContextLZ.flagIndex]
    ldr r7, [r0, #MIUncompContextLZ.length]
    ldrb r8, [r0, #MIUncompContextLZ.lengthFlg]
    ldrh r9, [r0, #MIUncompContextLZ.destTmp]
    ldrb r10, [r0, #MIUncompContextLZ.destTmpCnt]
    ldrb r14, [r0, #MIUncompContextLZ.exFormat]
    stmfd sp !, {r0}
@21:
    cmp r4, #0
    ble @29
    cmp r6, #0
    beq @28
@22
    cmp r2, #0
    beq @29
    tst r5, #0x80
    bne @23
    ldrb r11, [r1], #1
    sub r2, r2, #1
    orr r9, r9, r11, lsl r10
    add r10, r10, #8
    cmp r10, #16
    bne @26
    strh r9, [r3], #2
    sub r4, r4, #2
    mov r9, #0
    mov r10, #0
    b @26
@23:
    cmp r8, #0
    beq @24
    cmp r14, #1
    bne @23_9
    subs r8, r8, #1
    beq @23_7
    cmp r8, #1
    beq @23_6
    ldrb r7, [r1], #1
    tst r7, #0xE0
    beq @23_4
    add r7, r7, #0x10
    mov r8, #0
    b @23_10
@23_4:
    mov r11, #0x110
    tst r7, #0x10
    beq @23_5
    and r7, r7, #0xF
    add r11, r11, #0x1000
    add r7, r11, r7, lsl #16
    b @23_8
@23_5:
    and r7, r7, #0xF
    add r7, r11, r7, lsl #8
    mov r8, #1
    b @23_8
@23_6:
    ldrb r11, [r1], #1
    add r7, r7, r11, lsl #8
    b @23_8
@23_7:
    ldrb r11, [r1], #1
    add r7, r7, r11
    b @23_10
@23_8:
    subs r2, r2, #1
    beq @29
    b @23
@23_9:
    ldrb r7, [r1], #1
    add r7, r7, #0x30
    mov r8, #0
@23_10:
    subs r2, r2, #1
    beq @29
@24:
    and r11, r7, #0xF
    mov r12, r11, lsl #8
    ldrb r11, [r1], #1
    mov r8, #3
    sub r2, r2, #1
    orr r11, r11, r12
    add r11, r11, #1
    movs r7, r7, asr #4
    beq @26
@25:
    subs r12, r11, r10, lsr #3
    bne @25_1
    and r0, r9, #0xF
    orr r9, r9, r0, lsl #8
    b @25_2
@25_1:
    add r0, r12, #1
    mov r0, r0, lsr #1
    sub r0, r3, r0, lsl #1
    ldrh r0, [r0, #0]
    tst r12, #1
    movne r0, r0, lsr #8
    andeq r0, r0, #0xFF
    orr r9, r9, r0, lsl r10
@25_2:
    add r10, r10, #8
    cmp r10, #16
    bne @25_3
    strh r9, [r3], #2
    sub r4, r4, #2
    mov r9, #0
    mov r10, #0
@25_3:
    subs r7, r7, #1
    bgt @25
@26:
    cmp r4, #0
    beq @29
    mov r5, r5, lsl #1
    subs r6, r6, #1
    bne @22
@28:
    cmp r2, #0
    beq @29
    ldrb r5, [r1], #1
    mov r6, #8
    sub r2, r2, #1
    b @21
@29:
    ldmfd sp !, {r0}
    str r3, [r0, #MIUncompContextLZ.destp]
    str r4, [r0, #MIUncompContextLZ.destCount]
    strb r5, [r0, #MIUncompContextLZ.flags]
    strb r6, [r0, #MIUncompContextLZ.flagIndex]
    str r7, [r0, #MIUncompContextLZ.length]
    strb r8, [r0, #MIUncompContextLZ.lengthFlg]
    strh r9, [r0, #MIUncompContextLZ.destTmp]
    strb r10, [r0, #MIUncompContextLZ.destTmpCnt]
    strb r14, [r0, #MIUncompContextLZ.exFormat]
    mov r0, r4
    ldmfd sp !, {r4 - r12, lr}
    bx lr
}

#define TREE_END_MASK 0x80

asm s32 MI_ReadUncompHuffman (register MIUncompContextHuffman * context, register const u8 * data, register u32 len)
{
    stmfd sp !, {r4 - r12, lr}
    ldr r3, [r0, #MIUncompContextHuffman.destp]
    ldr r4, [r0, #MIUncompContextHuffman.destCount]
    ldr r5, [r0, #MIUncompContextHuffman.treep]
    ldr r6, [r0, #MIUncompContextHuffman.srcTmp]
    ldr r7, [r0, #MIUncompContextHuffman.destTmp]
    ldrsh r8, [r0, #MIUncompContextHuffman.treeSize]
    ldrb r9, [r0, #MIUncompContextHuffman.srcTmpCnt]
    ldrb r10, [r0, #MIUncompContextHuffman.destTmpCnt]
    ldrb r11, [r0, #MIUncompContextHuffman.bitSize]
    cmp r8, #0
    beq @12
    bgt @11
    ldrb r8, [r1], #1
    sub r2, r2, #1
    strb r8, [r5], #1
    add r8, r8, #1
    mov r8, r8, lsl #1
    sub r8, r8, #1
@11:
    cmp r2, #0
    beq @19
    ldrb r12, [r1], #1
    sub r2, r2, #1
    strb r12, [r5], #1
    subs r8, r8, #1
    addeq r5, r0, #MIUncompContextHuffman.tree[1]
    bgt @11
@12:
    cmp r4, #0
    ble @19
@13:
    cmp r9, #32
    bge @15
@14:
    cmp r2, #0
    beq @19
    ldr r12, [r1], #1
    sub r2, r2, #1
    orr r6, r6, r12, lsl r9
    add r9, r9, #8
    cmp r9, #32
    blt @14
@15:
    mov r12, r6, lsr #31
    ldrb r14, [r5, #0]
    mov r5, r5, lsr #1
    mov r5, r5, lsl #1
    and r8, r14, #0x3F
    add r8, r8, #1
    add r5, r5, r8, lsl #1
    add r5, r5, r12
    mov r8, #0
    mov r14, r14, lsl r12
    ands r14, r14, #TREE_END_MASK
    mov r6, r6, lsl #1
    sub r9, r9, #1
    beq @17
    mov r7, r7, lsr r11
    ldrb r12, [r5, #0]
    rsb r14, r11, #32
    orr r7, r7, r12, lsl r14
    add r5, r0, #MIUncompContextHuffman.tree[1]
    add r10, r10, r11
    cmp r4, r10, asr #3
    bgt @16
    rsb r12, r10, #32
    mov r7, r7, asr r12
    mov r10, #32
@16:
    cmp r10, #32
    bne @17
    str r7, [r3], #4
    mov r10, #0
    subs r4, r4, #4
    movle r4, #0
    ble @19
@17:
    cmp r9, #0
    bgt @15
    cmp r4, #0
    bgt @13
@19:
    str r3, [r0, #MIUncompContextHuffman.destp]
    str r4, [r0, #MIUncompContextHuffman.destCount]
    str r5, [r0, #MIUncompContextHuffman.treep]
    str r6, [r0, #MIUncompContextHuffman.srcTmp]
    str r7, [r0, #MIUncompContextHuffman.destTmp]
    strh r8, [r0, #MIUncompContextHuffman.treeSize]
    strb r9, [r0, #MIUncompContextHuffman.srcTmpCnt]
    strb r10, [r0, #MIUncompContextHuffman.destTmpCnt]
    strb r11, [r0, #MIUncompContextHuffman.bitSize]
    mov r0, r4
    ldmfd sp !, {r4 - r12, lr}
    bx lr
}

#include <nitro/codereset.h>
