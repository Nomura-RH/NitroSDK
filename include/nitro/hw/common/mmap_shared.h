#ifndef NITRO_HW_COMMON_MMAP_SHARED_H_
#define NITRO_HW_COMMON_MMAP_SHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HW_RED_RESERVED                 (HW_MAIN_MEM + 0x007ff800)
#define HW_RED_RESERVED_END             (HW_MAIN_MEM + 0x007ffa00)

#define HW_CARD_ROM_HEADER_SIZE           0x160

#define HW_DOWNLOAD_PARAMETER_SIZE        0x20

#define  HW_SHARED_ARENA_LO_DEFAULT       HW_MAIN_MEM_SHARED
#ifdef   HW_RED_RESERVED
# define HW_SHARED_ARENA_HI_DEFAULT       (HW_RED_RESERVED - HW_CARD_ROM_HEADER_SIZE - HW_DOWNLOAD_PARAMETER_SIZE)
#else
# define HW_SHARED_ARENA_HI_DEFAULT       (HW_MAIN_MEM_SYSTEM - HW_CARD_ROM_HEADER_SIZE - HW_DOWNLOAD_PARAMETER_SIZE)
#endif

#define HW_CARD_ROM_HEADER              (HW_MAIN_MEM + 0x007ffa80)

#define HW_DOWNLOAD_PARAMETER           (HW_MAIN_MEM + 0x007ffbe0)

#define HW_MAIN_MEM_SYSTEM_SIZE         0x400
#define HW_MAIN_MEM_SYSTEM              (HW_MAIN_MEM + 0x007ffc00)

#define HW_BOOT_CHECK_INFO_BUF          (HW_MAIN_MEM + 0x007ffc00)
#define HW_BOOT_CHECK_INFO_BUF_END      (HW_MAIN_MEM + 0x007ffc20)

#define HW_RESET_PARAMETER_BUF          (HW_MAIN_MEM + 0x007ffc20)

#define HW_ROM_BASE_OFFSET_BUF          (HW_MAIN_MEM + 0x007ffc2c)
#define HW_ROM_BASE_OFFSET_BUF_END      (HW_MAIN_MEM + 0x007ffc30)

#define HW_CTRDG_MODULE_INFO_BUF        (HW_MAIN_MEM + 0x007ffc30)
#define HW_CTRDG_MODULE_INFO_BUF_END    (HW_MAIN_MEM + 0x007ffc3c)

#define HW_VBLANK_COUNT_BUF             (HW_MAIN_MEM + 0x007ffc3c)

#define HW_WM_BOOT_BUF                  (HW_MAIN_MEM + 0x007ffc40)
#define HW_WM_BOOT_BUF_END              (HW_MAIN_MEM + 0x007ffc80)

#define HW_NVRAM_USER_INFO              (HW_MAIN_MEM + 0x007ffc80)
#define HW_NVRAM_USER_INFO_END          (HW_MAIN_MEM + 0x007ffd80)

#define HW_BIOS_EXCP_STACK_MAIN         (HW_MAIN_MEM + 0x007ffd80)
#define HW_BIOS_EXCP_STACK_MAIN_END     (HW_MAIN_MEM + 0x007ffd9c)
#define HW_EXCP_VECTOR_MAIN             (HW_MAIN_MEM + 0x007ffd9c)

#define HW_ARENA_INFO_BUF               (HW_MAIN_MEM + 0x007ffda0)
#define HW_REAL_TIME_CLOCK_BUF          (HW_MAIN_MEM + 0x007ffde8)

#define HW_DMA_CLEAR_DATA_BUF           (HW_MAIN_MEM + 0x007ffdf0)
#define HW_DMA_CLEAR_DATA_BUF_END       (HW_MAIN_MEM + 0x007ffe00)

#define HW_ROM_HEADER_BUF               (HW_MAIN_MEM + 0x007ffe00)
#define HW_ROM_HEADER_BUF_END           (HW_MAIN_MEM + 0x007fff60)
#define HW_ISD_RESERVED                 (HW_MAIN_MEM + 0x007fff60)
#define HW_ISD_RESERVED_END             (HW_MAIN_MEM + 0x007fff80)

#define HW_PXI_SIGNAL_PARAM_ARM9        (HW_MAIN_MEM + 0x007fff80)
#define HW_PXI_SIGNAL_PARAM_ARM7        (HW_MAIN_MEM + 0x007fff84)
#define HW_PXI_HANDLE_CHECKER_ARM9      (HW_MAIN_MEM + 0x007fff88)
#define HW_PXI_HANDLE_CHECKER_ARM7      (HW_MAIN_MEM + 0x007fff8c)

#define HW_MIC_LAST_ADDRESS             (HW_MAIN_MEM + 0x007fff90)
#define HW_MIC_SAMPLING_DATA            (HW_MAIN_MEM + 0x007fff94)

#define HW_WM_CALLBACK_CONTROL          (HW_MAIN_MEM + 0x007fff96)
#define HW_WM_RSSI_POOL                 (HW_MAIN_MEM + 0x007fff98)

#define HW_SET_CTRDG_MODULE_INFO_ONCE   (HW_MAIN_MEM + 0x007fff9a)
#define HW_IS_CTRDG_EXIST               (HW_MAIN_MEM + 0x007fff9b)

#define HW_COMPONENT_PARAM              (HW_MAIN_MEM + 0x007fff9c)

#define HW_THREADINFO_MAIN              (HW_MAIN_MEM + 0x007fffa0)
#define HW_THREADINFO_SUB               (HW_MAIN_MEM + 0x007fffa4)
#define HW_BUTTON_XY_BUF                (HW_MAIN_MEM + 0x007fffa8)
#define HW_TOUCHPANEL_BUF               (HW_MAIN_MEM + 0x007fffaa)
#define HW_AUTOLOAD_SYNC_BUF            (HW_MAIN_MEM + 0x007fffae)

#define HW_LOCK_ID_FLAG_MAIN            (HW_MAIN_MEM + 0x007fffb0)
#define HW_LOCK_ID_FLAG_SUB             (HW_MAIN_MEM + 0x007fffb8)

#define HW_VRAM_C_LOCK_BUF              (HW_MAIN_MEM + 0x007fffc0)
#define HW_VRAM_D_LOCK_BUF              (HW_MAIN_MEM + 0x007fffc8)
#define HW_WRAM_BLOCK0_LOCK_BUF         (HW_MAIN_MEM + 0x007fffd0)
#define HW_WRAM_BLOCK1_LOCK_BUF         (HW_MAIN_MEM + 0x007fffd8)
#define HW_CARD_LOCK_BUF                (HW_MAIN_MEM + 0x007fffe0)
#define HW_CTRDG_LOCK_BUF               (HW_MAIN_MEM + 0x007fffe8)
#define HW_INIT_LOCK_BUF                (HW_MAIN_MEM + 0x007ffff0)

#define HW_MMEMCHECKER_MAIN             (HW_MAIN_MEM + 0x007ffff8)
#define HW_MMEMCHECKER_SUB              (HW_MAIN_MEM + 0x007ffffa)

#define HW_CMD_AREA                     (HW_MAIN_MEM + 0x007ffffe)

#define HW_SHARED_LOCK_BUF              (HW_VRAM_C_LOCK_BUF)
#define HW_SHARED_LOCK_BUF_END          (HW_INIT_LOCK_BUF + 8)

#define HW_CHECK_DEBUGGER_SW     0x027ffc10
#define HW_CHECK_DEBUGGER_BUF1   0x027ff814
#define HW_CHECK_DEBUGGER_BUF2   0x027ffc14

#ifdef __cplusplus
}
#endif

#endif
