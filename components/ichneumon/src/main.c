#include <nitro/fs/file.h>
#include <nitro/os.h>
#include <nitro/pad/ARM7/xyButton.h>
#include <nitro/spi/ARM7/nvram.h>
#include <nitro/spi/ARM7/pm.h>
#include <nitro/spi/common/userInfo_ts_300.h>
#include <nitro/wm/ARM7/wm.h>

#define THREAD_PRIO_SPI       2
#define THREAD_PRIO_SND       6
#define THREAD_PRIO_RTC_ALARM 11
#define THREAD_PRIO_RTC       12

#define THREAD_PRIO_FS (OS_THREAD_LAUNCHER_PRIORITY - 1)

extern void WVR_ShelterExtWram(void);

static OSHeapHandle InitializeAllocateSystem(void);
static void ReadUserInfo(void);
#ifdef NVRAM_CONFIG_DATA_EX_VERSION
static u16 GetRomValidLanguage(void);
static s32 CheckCorrectNCDEx(NVRAMConfigEx *ncdsp);
#else
static s32 CheckCorrectNCD(NVRAMConfig *ncdsp);
#endif
static void VBlankIntr(void);

#ifdef MONGOOSE_USE_OVERLAY
FS_EXTERN_OVERLAY(mongoose_sub_mainmemory);
#endif

void NitroSpMain(void)
{
    OSHeapHandle heapHandle;

#ifdef SDK_WIRELESS_IN_VRAM
    WVR_ShelterExtWram();
#endif

    OS_Init();
    OS_InitThread();

    ReadUserInfo();

    PXI_Init();

    heapHandle = InitializeAllocateSystem();

    SND_Init(THREAD_PRIO_SND);

    PAD_InitXYButton();

    OS_SetIrqFunction(OS_IE_V_BLANK, VBlankIntr);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    GX_VBlankIntr(TRUE);
    OS_EnableIrq();
    OS_EnableInterrupts();

    FS_Init(FS_DMA_NOT_USE);
    FS_CreateReadServerThread(THREAD_PRIO_FS);

    RTC_Init(THREAD_PRIO_RTC);

#ifndef SDK_WIRELESS_IN_VRAM
#ifdef MONGOOSE_USE_OVERLAY
    FS_LoadOverlay(MI_PROCESSOR_ARM7, FS_OVERLAY_ID(mongoose_sub_mainmemory));
#endif
    WVR_Begin(heapHandle);
#else
    WVR_Init(heapHandle);
#endif

    SPI_Init(THREAD_PRIO_SPI);

    while (TRUE) {
        OS_Halt();

        if (OS_IsResetOccurred()) {
            CTRDG_VibPulseEdgeUpdate(NULL);

            OS_ResetSystem();
        }

        CTRDG_CheckPullOut_Polling();

#ifndef SDK_SMALL_BUILD
        CARD_CheckPullOut_Polling();
#endif
    }
}

static OSHeapHandle InitializeAllocateSystem(void)
{
    void *tempLo = OS_InitAlloc(OS_ARENA_WRAM_SUBPRIV, OS_GetWramSubPrivArenaLo(), OS_GetWramSubPrivArenaHi(), 1);

    MI_CpuClear8(tempLo, (u32)OS_GetWramSubPrivArenaHi() - (u32)tempLo);

    OS_SetArenaLo(OS_ARENA_WRAM_SUBPRIV, tempLo);

    OSHeapHandle hh = OS_CreateHeap(OS_ARENA_WRAM_SUBPRIV, OS_GetWramSubPrivArenaLo(), OS_GetWramSubPrivArenaHi());

    if (hh < 0) {
        OS_Panic("ARM7: Fail to create heap.\n");
    }

    OS_SetCurrentHeap(OS_ARENA_WRAM_SUBPRIV, hh);

    u32 heapSize = OS_CheckHeap(OS_ARENA_WRAM_SUBPRIV, hh);
    if (WM_WL_HEAP_SIZE > heapSize) {
        OS_Panic("Insufficient heap size. ( %xh < %xh )\n", heapSize, WM_WL_HEAP_SIZE);
    }

    return hh;
}

#ifdef WM_PRECALC_ALLOWEDCHANNEL
extern u16 WMSP_GetAllowedChannel(u16 bitField);
#endif

static void ReadUserInfo(void)
{
    s32 offset;
#ifdef NVRAM_CONFIG_DATA_EX_VERSION
    NVRAMConfigEx temp[2];
#else
    NVRAMConfig temp[2];
#endif
    s32 check;
    u8 *p = OS_GetSystemWork()->nvramUserInfo;

#ifdef NVRAM_CONFIG_CONST_ADDRESS
    offset = NVRAM_CONFIG_DATA_ADDRESS_DUMMY;
#else
    NVRAM_ReadDataBytes(NVRAM_CONFIG_DATA_OFFSET_ADDRESS, NVRAM_CONFIG_DATA_OFFSET_SIZE, &offset);
    offset <<= NVRAM_CONFIG_DATA_OFFSET_SHIFT;
#endif

#ifdef NVRAM_CONFIG_DATA_EX_VERSION
    NVRAM_ReadDataBytes(offset, sizeof(NVRAMConfigEx), (u8 *)&temp[0]);
    NVRAM_ReadDataBytes(offset + SPI_NVRAM_PAGE_SIZE, sizeof(NVRAMConfigEx), (u8 *)&temp[1]);

    check = CheckCorrectNCDEx(temp);
#else
    NVRAM_ReadDataBytes(offset, sizeof(NVRAMConfig), (u8 *)&temp[0]);
    NVRAM_ReadDataBytes(offset + SPI_NVRAM_PAGE_SIZE, sizeof(NVRAMConfig), (u8 *)&temp[1]);

    check = CheckCorrectNCD(temp);
#endif

    if (check >= 3) {
        MI_CpuFill32(p, 0xFFFFFFFF, sizeof(NVRAMConfig));
    } else if (check) {
#ifdef SDK_TS
#if (SDK_TS_VERSION >= 200 || SDK_NVRAM_FORMAT >= 100)
        s32 i;

        if (temp[check - 1].ncd.owner.nickname.length < NVRAM_CONFIG_NICKNAME_LENGTH) {
            for (i = NVRAM_CONFIG_NICKNAME_LENGTH; i > temp[check - 1].ncd.owner.nickname.length; i--) {
                temp[check - 1].ncd.owner.nickname.str[i - 1] = 0;
            }
        }

        if (temp[check - 1].ncd.owner.comment.length < NVRAM_CONFIG_COMMENT_LENGTH) {
            for (i = NVRAM_CONFIG_COMMENT_LENGTH; i > temp[check - 1].ncd.owner.comment.length; i--) {
                temp[check - 1].ncd.owner.comment.str[i - 1] = 0;
            }
        }
#endif
#endif

        MI_CpuCopy32(&temp[check - 1], p, sizeof(NVRAMConfig));
    } else {
        MI_CpuClear32(p, sizeof(NVRAMConfig));
    }

    u8 wMac[6];

    NVRAM_ReadDataBytes(NVRAM_CONFIG_MACADDRESS_ADDRESS, 6, wMac);

    p = (u8 *)((u32)p + ((sizeof(NVRAMConfig) + 3) & ~3));

    MI_CpuCopy8(wMac, p, 6);

#ifdef WM_PRECALC_ALLOWEDCHANNEL
    u16 enableChannel;
    NVRAM_ReadDataBytes(NVRAM_CONFIG_ENABLECHANNEL_ADDRESS, 2, &enableChannel);

    u16 allowedChannel = WMSP_GetAllowedChannel(enableChannel >> 1);

    p = (u8 *)((u32)p + 6);

    *((u16 *)p) = allowedChannel;
#endif
}

#ifdef NVRAM_CONFIG_DATA_EX_VERSION
static BOOL IsValidConfigEx(void)
{
    u8 ipl2_type;
    NVRAM_ReadDataBytes(NVRAM_CONFIG_IPL2_TYPE_ADDRESS, NVRAM_CONFIG_IPL2_TYPE_SIZE, &ipl2_type);

    if (ipl2_type == NVRAM_CONFIG_IPL2_TYPE_NORMAL) {
        return FALSE;
    }
    if (ipl2_type & NVRAM_CONFIG_IPL2_TYPE_EX_MASK) {
        return TRUE;
    }
    return FALSE;
}

static u16 GetRomValidLanguage(void)
{
    u16 ret = 0;

    if (OS_GetSystemWork()->rom_header[0x1D] == 0x80) {
        ret |= 1 << NVRAM_CONFIG_LANG_CHINESE;
    } else if (OS_GetSystemWork()->rom_header[0x1D] == 0x40) {
        ret |= 1 << NVRAM_CONFIG_LANG_HANGUL;
    }

    return ret;
}

static s32 CheckCorrectNCDEx(NVRAMConfigEx *ncdsp)
{
    u16 i;
    u16 calc_crc;
    s32 crc_flag = 0;
    u16 saveCount;

    if (IsValidConfigEx()) {
        u16 rom_valid_language = GetRomValidLanguage();

        for (i = 0; i < 2; i++) {
            calc_crc = SVC_GetCRC16(0xFFFF, &ncdsp[i].ncd, sizeof(NVRAMConfigData));
            if (ncdsp[i].crc16 == calc_crc && ncdsp[i].saveCount < NVRAM_CONFIG_SAVE_COUNT_MAX) {
                calc_crc = SVC_GetCRC16(0xFFFF, &ncdsp[i].ncd_ex, sizeof(NVRAMConfigDataEx));
                if (ncdsp[i].crc16_ex == calc_crc && (1 << ncdsp[i].ncd_ex.language & ncdsp[i].ncd_ex.valid_language_bitmap)) {
                    if (rom_valid_language & ncdsp[i].ncd_ex.valid_language_bitmap) {
                        ncdsp[i].ncd.option.language = ncdsp[i].ncd_ex.language;
                    }
                    if (rom_valid_language & (1 << NVRAM_CONFIG_LANG_CHINESE) & ~ncdsp[i].ncd_ex.valid_language_bitmap) {
                        return 3;
                    }
                    crc_flag |= (1 << i);
                }
            }
        }
    } else {
        u16 rom_valid_language = GetRomValidLanguage();
        if (rom_valid_language & (1 << NVRAM_CONFIG_LANG_CHINESE)) {
            return 3;
        }

        for (i = 0; i < 2; i++) {
            calc_crc = SVC_GetCRC16(0xFFFF, &ncdsp[i].ncd, sizeof(NVRAMConfigData));
            if (ncdsp[i].crc16 == calc_crc && ncdsp[i].saveCount < NVRAM_CONFIG_SAVE_COUNT_MAX) {
                crc_flag |= (1 << i);
            }
        }
    }

    switch (crc_flag) {
    case 1:
    case 2:
        return crc_flag;
    case 3:
        saveCount = (u8)((ncdsp[0].saveCount + 1) & NVRAM_CONFIG_SAVE_COUNT_MASK);
        if (saveCount == ncdsp[1].saveCount) {
            return 2;
        }
        return 1;
    }

    return 0;
}
#else
static s32 CheckCorrectNCD(NVRAMConfig *ncdsp)
{
    u16 i;
    u16 calc_crc;
    s32 crc_flag = 0;
    u16 saveCount;

    for (i = 0; i < 2; i++) {
        calc_crc = SVC_GetCRC16(0xFFFF, &ncdsp[i].ncd, sizeof(NVRAMConfigData));

        if (ncdsp[i].crc16 == calc_crc && ncdsp[i].saveCount < NVRAM_CONFIG_SAVE_COUNT_MAX) {
            crc_flag |= (1 << i);
        }
    }

    switch (crc_flag) {
    case 1:
    case 2:
        return crc_flag;
    case 3:
        saveCount = (u8)((ncdsp[0].saveCount + 1) & NVRAM_CONFIG_SAVE_COUNT_MASK);
        if (saveCount == ncdsp[1].saveCount) {
            return 2;
        }
        return 1;
    }

    return 0;
}
#endif

#ifndef SDK_TEG
static void VBlankIntr(void)
{
    if (PM_IsAvailable()) {
        PM_SelfBlinkProc();
    }
}
#else
static void VBlankIntr(void)
{
}
#endif
