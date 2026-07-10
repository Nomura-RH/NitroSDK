#include <asm/macros/function.inc>

    .section .text
    non_word_aligned_thumb_func_start SVC_SoftReset
SVC_SoftReset:
    swi 0
    bx lr

    thumb_func_end SVC_SoftReset

    non_word_aligned_thumb_func_start SVC_WaitByLoop
SVC_WaitByLoop:
    swi 3
    bx lr

    thumb_func_end SVC_WaitByLoop

    non_word_aligned_thumb_func_start SVC_WaitIntr
SVC_WaitIntr:
    ldr r2, SVC_WaitIntr.HW_REG_BASE
    mov r12, r2
    mov r2, #0
    swi 4
    bx lr

    .balign 4, 0
SVC_WaitIntr.HW_REG_BASE:
    .word 0x04000000

    thumb_func_end SVC_WaitIntr

    non_word_aligned_thumb_func_start SVC_WaitVBlankIntr
SVC_WaitVBlankIntr:
    mov r2, #0
    swi 5
    bx lr

    thumb_func_end SVC_WaitVBlankIntr

    non_word_aligned_thumb_func_start SVC_Halt
SVC_Halt:
    swi 6
    bx lr

    thumb_func_end SVC_Halt

    non_word_aligned_thumb_func_start SVC_Sleep
SVC_Sleep:
    swi 7
    bx lr

    thumb_func_end SVC_Sleep

    non_word_aligned_thumb_func_start SVC_ChangeSoundBias
SVC_ChangeSoundBias:
    swi 8
    bx lr

    thumb_func_end SVC_ChangeSoundBias

    non_word_aligned_thumb_func_start SVC_SetSoundBias
SVC_SetSoundBias:
    add r1, r0, #0
    mov r0, #1
    swi 8
    bx lr

    thumb_func_end SVC_SetSoundBias

    non_word_aligned_thumb_func_start SVC_ResetSoundBias
SVC_ResetSoundBias:
    add r1, r0, #0
    mov r0, #0
    swi 8
    bx lr

    thumb_func_end SVC_ResetSoundBias

    non_word_aligned_thumb_func_start SVC_Div
SVC_Div:
    swi 9
    bx lr

    thumb_func_end SVC_Div

    non_word_aligned_thumb_func_start SVC_DivRem
SVC_DivRem:
    swi 9
    add r0, r1, #0
    bx lr

    thumb_func_end SVC_DivRem

    non_word_aligned_thumb_func_start SVC_CpuSet
SVC_CpuSet:
    swi 0xb
    bx lr

    thumb_func_end SVC_CpuSet

    non_word_aligned_thumb_func_start SVC_CpuSetFast
SVC_CpuSetFast:
    swi 0xc
    bx lr

    thumb_func_end SVC_CpuSetFast

    non_word_aligned_thumb_func_start SVC_Sqrt
SVC_Sqrt:
    swi 0xd
    bx lr

    thumb_func_end SVC_Sqrt

    non_word_aligned_thumb_func_start SVC_GetCRC16
SVC_GetCRC16:
    swi 0xe
    bx lr

    thumb_func_end SVC_GetCRC16

    non_word_aligned_thumb_func_start IsMmemExpanded
IsMmemExpanded:
    swi 0xf
    bx lr

    thumb_func_end IsMmemExpanded

    non_word_aligned_thumb_func_start SVC_UnpackBits
SVC_UnpackBits:
    swi 0x10
    bx lr

    thumb_func_end SVC_UnpackBits

    non_word_aligned_thumb_func_start SVC_UncompressLZ8
SVC_UncompressLZ8:
    swi 0x11
    bx lr

    thumb_func_end SVC_UncompressLZ8

    non_word_aligned_thumb_func_start SVC_UncompressLZ16FromDevice
SVC_UncompressLZ16FromDevice:
    swi 0x12
    bx lr

    thumb_func_end SVC_UncompressLZ16FromDevice

    non_word_aligned_thumb_func_start SVC_UncompressHuffmanFromDevice
SVC_UncompressHuffmanFromDevice:
    swi 0x13
    bx lr

    thumb_func_end SVC_UncompressHuffmanFromDevice

    non_word_aligned_thumb_func_start SVC_UncompressRL8
SVC_UncompressRL8:
    swi 0x14
    bx lr

    thumb_func_end SVC_UncompressRL8

    non_word_aligned_thumb_func_start SVC_UncompressRL16FromDevice
SVC_UncompressRL16FromDevice:
    swi 0x15
    bx lr

    thumb_func_end SVC_UncompressRL16FromDevice

    non_word_aligned_thumb_func_start SVC_GetSinTable
SVC_GetSinTable:
    swi 0x1a
    bx lr

    thumb_func_end SVC_GetSinTable

    non_word_aligned_thumb_func_start SVC_GetPitchTable
SVC_GetPitchTable:
    swi 0x1b
    bx lr

    thumb_func_end SVC_GetPitchTable

    non_word_aligned_thumb_func_start SVC_GetVolumeTable
SVC_GetVolumeTable:
    swi 0x1c
    bx lr

    thumb_func_end SVC_GetVolumeTable

