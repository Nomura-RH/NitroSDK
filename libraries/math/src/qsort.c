#include <nitro.h>

#include <nitro/code32.h>

asm void MATH_QSort (register void *head, register u32 num, register u32 width, register MATHCompareFunc comp, void *stackBuf)
{
#define stack       r4
#define stackBuf_   r5
#define left        r6
#define right       r7
#define somewhere   r6
#define somewhere_l r9
#define somewhere_r r10
#define tmp         r2
#define tmp2        r3
#define comp_       r11
#define width_      r8

	stmfd sp !, {r4 - r11, lr}
	cmp num, #1
	ble @fin
	ldr stack, [sp, #36]
	mov comp_, comp
	mov width_, width
	cmp stack, #0
	bne @00
	clz tmp, num
	rsb tmp, tmp, #32
	mov tmp, tmp, lsl #3
	sub sp, sp, tmp
	mov stack, sp
	str tmp, [sp, #- 4] !
@00:
	sub num, num, #1
	mla num, num, width_, head
	mov stackBuf_, stack
	str head, [stack], #4
	str num, [stack], #4
	clz tmp, width_
	rsb tmp, tmp, #32
	str tmp, [sp, #- 4] !
@01:
	cmp stack, stackBuf_
	beq @end
	ldr right, [stack, #- 4]
	ldr left, [stack, #- 8] !
	sub tmp, right, left
	cmp tmp, width_
	bne @02
	mov r0, left
	mov r1, right
	blx comp_
	cmp r0, #0
	ble @01
	mov r0, width_
	tst r0, #3
	beq @012
@011:
	ldrb r1, [left]
	subs r0, r0, #1
	swpb r1, r1, [right]
	add right, right, #1
	strb r1, [left], #1
	bne @011
	b @01
@012:
	ldr r1, [left]
	subs r0, r0, #4
	swp r1, r1, [right]
	add right, right, #4
	str r1, [left], #4
	bne @012
	b @01
@02:
	ldr tmp2, [sp]
	sub tmp, right, left
	mov tmp, tmp, lsr tmp2
	mla tmp, tmp, width_, left
	mov r3, left
	mov r0, width_
	mov r2, tmp
	tst r0, #3
	beq @022
@021:
	ldrb r1, [r2]
	subs r0, r0, #1
	swpb r1, r1, [r3]
	add r3, r3, #1
	strb r1, [r2], #1
	bne @021
	b @023
@022:
	ldr r1, [r2]
	subs r0, r0, #4
	swp r1, r1, [r3]
	add r3, r3, #4
	str r1, [r2], #4
	bne @022
@023:
	mov somewhere_l, left
	mov somewhere_r, right
	add somewhere_l, somewhere_l, width_
@03:
	cmp somewhere_l, right
	bge @04
	mov r1, somewhere
	mov r0, somewhere_l
	blx comp_
	cmp r0, #0
	addlt somewhere_l, somewhere_l, width_
	blt @03
@04:
	mov r1, somewhere
	mov r0, somewhere_r
	blx comp_
	cmp r0, #0
	subgt somewhere_r, somewhere_r, width_
	bgt @04
	cmp somewhere_l, somewhere_r
	bge @05
	mov r2, somewhere_l
	mov r3, somewhere_r
	mov r0, width_
	tst r0, #3
	beq @042
@041:
	ldrb r1, [r2]
	subs r0, r0, #1
	swpb r1, r1, [r3]
	add r3, r3, #1
	strb r1, [r2], #1
	bne @041
	b @043
@042:
	ldr r1, [r2]
	subs r0, r0, #4
	swp r1, r1, [r3]
	add r3, r3, #4
	str r1, [r2], #4
	bne @042
@043:
	add somewhere_l, somewhere_l, width_
	sub somewhere_r, somewhere_r, width_
	cmp somewhere_l, somewhere_r
	ble @03
@05:
	mov r2, left
	mov r3, somewhere_r
	mov r0, width_
	tst r0, #3
	beq @052
@051:
	ldrb r1, [r2]
	subs r0, r0, #1
	swpb r1, r1, [r3]
	add r3, r3, #1
	strb r1, [r2], #1
	bne @051
	b @053
@052:
	ldr r1, [r2]
	subs r0, r0, #4
	swp r1, r1, [r3]
	add r3, r3, #4
	str r1, [r2], #4
	bne @052
@053:
	sub tmp, somewhere_r, left
	sub tmp2, right, somewhere_r
	cmp tmp, tmp2
	ble @06
	sub tmp, somewhere_r, width_
	cmp left, tmp
	strlt left, [stack], #4
	strlt tmp, [stack], #4
	add tmp, somewhere_r, width_
	cmp tmp, right
	strlt tmp, [stack], #4
	strlt right, [stack], #4
	b @01
@06:
	add tmp, somewhere_r, width_
	cmp tmp, right
	strlt tmp, [stack], #4
	strlt right, [stack], #4
	sub tmp, somewhere_r, width_
	cmp left, tmp
	strlt left, [stack], #4
	strlt tmp, [stack], #4
	b @01
@end:
	add sp, sp, #4
	sub stack, stack, #4
	cmp stack, sp
	ldreq r0, [sp]
	addeq r0, r0, #4
	addeq sp, sp, r0
@fin:
	ldmfd sp !, {r4 - r11, lr}
	bx lr

#undef stack
#undef stackBuf_
#undef left
#undef right
#undef somewhere
#undef somewhere_l
#undef somewhere_r
#undef tmp
#undef tmp2
#undef comp_
#undef width_

}

#include <nitro/codereset.h>
