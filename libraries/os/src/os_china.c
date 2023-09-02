#include <nitro/os.h>
#include <nitro/gx.h>
#include <nitro/spi.h>
#include <nitro/pad.h>
#include <nitro/os/ARM9/china.h>

#include <nitro/version_begin.h>
static char checkString[] = OS_BURY_STRING_FORCHINA;
#include <nitro/version_end.h>

#define OSi_FORCHINA_FADE_SPAN          32
#define OSi_FORCHINA_DISP_SPAN          64

typedef enum {
    IMAGE_NOTES_CHR,
    IMAGE_NOTES_SCR,
    IMAGE_NOTES_DIGIT,
    IMAGE_LOGO_CHR,
    IMAGE_LOGO_SCR,
    IMAGE_LOGO_PAL,
    IMAGE_LOGO_VOICE,
    IMAGE_MAX
} ImageIndex;

#define OSi_ONLY_FORCHINA_CHAR_WIDTH    15
#define OSi_ONLY_FORCHINA_CHAR_HEIGHT   2
#define OSi_ONLY_FORCHINA_DOT_WIDTH     117
#define OSi_ONLY_FORCHINA_DOT_HEIGHT    16
#define OSi_ONLY_FORCHINA_CHAR_SIZE     240

static u8 only_forChina_charData[OSi_ONLY_FORCHINA_CHAR_SIZE] ATTRIBUTE_ALIGN(4) = {
    0x00, 0x7c, 0xfe, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x8c, 0x8c, 0x9c, 0x9c, 0xbc, 0xbc, 0xec,
    0x00, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x00, 0xc3, 0xc3, 0x66, 0x66, 0x3c,
    0x3c, 0x18, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0xcf, 0xef, 0x60,
    0x60, 0x60, 0x67, 0x67, 0x00, 0xc7, 0xcf, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x0f,
    0x1f, 0x18, 0x18, 0x18, 0x1f, 0x0f, 0x00, 0x60, 0x60, 0x00, 0x00, 0x60, 0x60, 0x60,
    0x00, 0x7c, 0xfe, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x8c, 0x8c, 0x8c,
    0x8c, 0x8c, 0x00, 0x00, 0x00, 0xf1, 0xf9, 0x19, 0x19, 0xf9, 0x00, 0xc0, 0xc0, 0xc1,
    0xc3, 0xc3, 0xc3, 0xc3, 0x00, 0x87, 0xcf, 0xdc, 0xd8, 0xd8, 0xd8, 0x98, 0x00, 0x0f,
    0x0f, 0x00, 0x00, 0x00, 0x07, 0x0f, 0xc6, 0xc6, 0xc6, 0xfe, 0x7c, 0x00, 0x00, 0x00,
    0xec, 0xcc, 0xcc, 0x8c, 0x8c, 0x00, 0x00, 0x00, 0x19, 0x19, 0x19, 0xf9, 0xf9, 0x00,
    0x00, 0x00, 0x18, 0x18, 0x18, 0x19, 0x19, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0,
    0xc0, 0x00, 0x00, 0x00, 0x60, 0x60, 0x60, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0xcc, 0xcc,
    0xcc, 0xcf, 0xc7, 0x00, 0x00, 0x00, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x00, 0x00, 0x00,
    0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xfe, 0x7c, 0xe0,
    0xc0, 0x00, 0x8c, 0x8c, 0x8c, 0xfc, 0xf8, 0x01, 0x01, 0x00, 0xf9, 0x19, 0x19, 0xf9,
    0xf1, 0x00, 0x00, 0x00, 0xc3, 0xc0, 0xc0, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x18, 0x18,
    0x1c, 0xcf, 0xc7, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x0c, 0x0f, 0x07, 0x00, 0x00, 0x00
};

static u8 * LoadImage(ImageIndex index, u32 * p_size);
static void WaitForNextFrame(void);
static void VBlankIntr(void);
static void SetISBNString(const char ** isbn);
static void CheckLanguageCode(void);
static void CheckDetectFold(void);
static void DispExclusiveMessage(void);
static void DispLogoAndNotes(const char ** isbn);

static u8 * LoadImage (ImageIndex index, u32 * p_size)
{
    void * ptr = NULL;
#if defined(SDK_CHINA_DATA_FROM_FILE)
    static const char * (path_table[IMAGE_MAX]) = {
        "notes_forChina_chrData.bin",
        "notes_forChina_scrData.bin",
        "digit_chrData.bin",
        "logo_forChina_chrData.bin",
        "logo_forChina_scrData.bin",
        "logo_forChina_palData.bin",
        "logo_forChina_voice.bin",
    };

    const char * path = path_table[index];

    FSFile file[1];

    FS_InitFile(file);
    if (!FS_OpenFile(file, path)) {
        OS_TPanic("failed to open file \"%s\"", path);
    } else {
        u32 len = FS_GetLength(file);
        ptr = OS_Alloc(len);
        if (!ptr) {
            OS_TPanic("failed to allocate memory for image \"%s\"", path);
        } else {
            if (p_size) {
                *p_size = len;
            }
            if (FS_ReadFile(file, ptr, (int)len) != len) {
                OS_TPanic("failed to read file \"%s\"", path);
            }
        }
        (void)FS_CloseFile(file);
    }

#else
    extern u8 _binary_notes_forChina_chrData_bin[];
    extern u8 _binary_notes_forChina_chrData_bin_end[];
    extern u8 _binary_notes_forChina_scrData_bin[];
    extern u8 _binary_notes_forChina_scrData_bin_end[];
    extern u8 _binary_digit_chrData_bin[];
    extern u8 _binary_digit_chrData_bin_end[];
    extern u8 _binary_logo_forChina_chrData_bin[];
    extern u8 _binary_logo_forChina_chrData_bin_end[];
    extern u8 _binary_logo_forChina_scrData_bin[];
    extern u8 _binary_logo_forChina_scrData_bin_end[];
    extern u8 _binary_logo_forChina_palData_bin[];
    extern u8 _binary_logo_forChina_palData_bin_end[];

    static u8 * (ptr_table[]) = {
        _binary_notes_forChina_chrData_bin,
        _binary_notes_forChina_scrData_bin,
        _binary_digit_chrData_bin,
        _binary_logo_forChina_chrData_bin,
        _binary_logo_forChina_scrData_bin,
        _binary_logo_forChina_palData_bin,
    };
    static u8 * (ptr_end_table[]) = {
        _binary_notes_forChina_chrData_bin_end,
        _binary_notes_forChina_scrData_bin_end,
        _binary_digit_chrData_bin_end,
        _binary_logo_forChina_chrData_bin_end,
        _binary_logo_forChina_scrData_bin_end,
        _binary_logo_forChina_palData_bin_end,
    };

    ptr = ptr_table[index];
    if (p_size) {
        *p_size = (u32)(ptr_end_table[index] - ptr_table[index]);
    }

#endif

    return (u8 *)ptr;
}

static void WaitForNextFrame (void)
{
    if (PAD_DetectFold()) {
        PM_GoSleepMode(PM_TRIGGER_COVER_OPEN | PM_TRIGGER_CARD, 0, 0);
    }

    SVC_WaitVBlankIntr();
}

static void VBlankIntr (void)
{
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void SetISBNString (const char ** isbn)
{
    s32 i, j;
    struct {
        u8 x, y;
        u8 length;
    } pos[] = {
        {
            80, 144, 13
        }
        ,
        {
            120, 158, 12
        }
        ,
        {
            103, 175, 4
        }
        ,
        {
            144, 175, 4
        }
        ,
    };

    const int count = sizeof(pos) / sizeof(*pos);
    const int digit_id = 0;

    GXOamAttr * dst = (GXOamAttr *)HW_DB_OAM;

    for (i = 0; i < count; i++) {
        for (j = 0; j < pos[i].length; ++j) {
            dst->attr01 = (u32)(((pos[i].x + j * 8) << 16) | (pos[i].y << 0));
            if (isbn[i][j] == '-') {
                dst->attr2 = (u16)(digit_id + 10);
            } else if ((isbn[i][j] >= '0') && (isbn[i][j] <= '9'))   {
                dst->attr2 = (u16)(digit_id + isbn[i][j] - '0');
            } else {
                dst->attr2 = (u16)(digit_id + 11);
            }
            ++dst;
        }
    }
}

void OS_InitChina (const char ** isbn)
{
    SDK_REFER_SYMBOL(checkString);

    OS_Init();
    CheckLanguageCode();
    OS_ShowAttentionChina(isbn);
}

static void CheckLanguageCode (void)
{
    NVRAMConfig * src;

    src = (NVRAMConfig *)(OS_GetSystemWork()->nvramUserInfo);

    if ((src->ncd.version == 0xff) && (src->ncd.option.language == 7)) {
        DispExclusiveMessage();
    }
}

static void DispExclusiveMessage (void)
{

    reg_GX_DISPCNT = 0;
    reg_GXS_DB_DISPCNT = 0;

    if (!(reg_GX_POWCNT & REG_GX_POWCNT_LCD_MASK)) {

        SVC_WaitByLoop(HW_CPU_CLOCK_ARM9 / 40);
    }
    reg_GX_POWCNT = (u16)(REG_GX_POWCNT_DSEL_MASK | REG_GX_POWCNT_E2DG_MASK |
                          REG_GX_POWCNT_E2DGB_MASK | REG_GX_POWCNT_LCD_MASK);

    reg_GX_MASTER_BRIGHT = 0;
    reg_GXS_DB_MASTER_BRIGHT = 0;

    {
        reg_GX_VRAMCNT_A = (u8)((1 << REG_GX_VRAMCNT_A_MST_SHIFT) | (1 << REG_GX_VRAMCNT_A_E_SHIFT));
        reg_G2_BG0CNT =
            (u16)((GX_BG_SCRSIZE_TEXT_256x256 << REG_G2_BG0CNT_SCREENSIZE_SHIFT) |
                  (GX_BG_COLORMODE_16 << REG_G2_BG0CNT_COLORMODE_SHIFT) | (GX_BG_SCRBASE_0xf000 <<
                                                                           REG_G2_BG0CNT_SCREENBASE_SHIFT)
                  | (GX_BG_CHARBASE_0x00000 << REG_G2_BG0CNT_CHARBASE_SHIFT) | (GX_BG_EXTPLTT_01 <<
                                                                                REG_G2_BG0CNT_BGPLTTSLOT_SHIFT)
                  | (0 << REG_G2_BG0CNT_PRIORITY_SHIFT));
        reg_G2_BG0HOFS = (u16)(-(256 - OSi_ONLY_FORCHINA_DOT_WIDTH) / 2);
        reg_G2_BG0VOFS = (u16)(-(192 - OSi_ONLY_FORCHINA_DOT_HEIGHT) / 2 + 2);
        reg_GX_DISPCNT |= ((GX_BGMODE_0 << REG_GX_DISPCNT_BGMODE_SHIFT) |
                           (GX_PLANEMASK_BG0 << REG_GX_DISPCNT_DISPLAY_SHIFT));

        {
            MIUnpackBitsParam param = { OSi_ONLY_FORCHINA_CHAR_SIZE, 1, 4, 0, 0 };

            SVC_UnpackBits(only_forChina_charData, (u32 *)(HW_BG_VRAM + 0x20), &param);
        }

        {
            s32 i;
            s32 j;
            u16 code = 1;
            u16 * dst = (u16 *)(HW_BG_VRAM + 0xf000);

            for (i = 0; i < OSi_ONLY_FORCHINA_CHAR_HEIGHT; i++) {
                for (j = 0; j < OSi_ONLY_FORCHINA_CHAR_WIDTH; j++) {
                    *dst++ = code++;
                }
                dst += (0x20 - OSi_ONLY_FORCHINA_CHAR_WIDTH);
            }
        }
    }

    {
        reg_GX_VRAMCNT_C = (u8)((4 << REG_GX_VRAMCNT_C_MST_SHIFT) | (1 << REG_GX_VRAMCNT_C_E_SHIFT));
        reg_G2S_DB_BG0CNT =
            (u16)((GX_BG_SCRSIZE_TEXT_256x256 << REG_G2S_DB_BG0CNT_SCREENSIZE_SHIFT) |
                  (GX_BG_COLORMODE_16 << REG_G2S_DB_BG0CNT_COLORMODE_SHIFT) | (GX_BG_SCRBASE_0xf000
                                                                               <<
                                                                               REG_G2S_DB_BG0CNT_SCREENBASE_SHIFT)
                  | (GX_BG_CHARBASE_0x00000 << REG_G2S_DB_BG0CNT_CHARBASE_SHIFT) | (GX_BG_EXTPLTT_01
                                                                                    <<
                                                                                    REG_G2S_DB_BG0CNT_BGPLTTSLOT_SHIFT)
                  | (0 << REG_G2S_DB_BG0CNT_PRIORITY_SHIFT));
        reg_G2S_DB_BG0OFS = 0;
        reg_GXS_DB_DISPCNT |= ((GX_BGMODE_0 << REG_GXS_DB_DISPCNT_BGMODE_SHIFT) |
                               (GX_PLANEMASK_BG0 << REG_GXS_DB_DISPCNT_DISPLAY_SHIFT));
    }

    (void)OS_SetIrqFunction(OS_IE_V_BLANK, VBlankIntr);
    (void)OS_EnableIrqMask(OS_IE_V_BLANK);
    (void)OS_EnableIrq();

    reg_GX_DISPSTAT |= REG_GX_DISPSTAT_VBI_MASK;

    reg_GX_DISPCNT |= (u32)(GX_DISPMODE_GRAPHICS << REG_GX_DISPCNT_MODE_SHIFT);
    reg_GXS_DB_DISPCNT |= (u32)(REG_GXS_DB_DISPCNT_MODE_MASK);

    {
        u16 data = 0x001f;
        u16 target = 0x0001 << 5;
        s32 i;

        while (TRUE) {
            for (i = 0; i < 31; i++) {

                if (TRUE == PAD_DetectFold()) {

                    (void)PM_ForceToPowerOff();
                }
                OS_WaitVBlankIntr();
                *((u16 *)(HW_BG_PLTT + 2)) = data;
                data += target;
            }
            target = (u16)(~target + 1);
        }
    }
}

SDK_WEAK_SYMBOL void OS_ShowAttentionChina (const char ** isbn)
{
    struct {
        u32 gx_dispcnt;
        u32 gxs_dispcnt;
        u16 gx_powcnt;
        u16 gx_dispstat;
        u16 gx_bright;
        u16 gxs_bright;
        u16 gx_bg0cnt;
        u16 gxs_bg0cnt;
        u32 gx_bg0ofs;
        u32 gxs_bg0ofs;
        u8 gx_vramcnt_a;
        u8 gx_vramcnt_c;
        u8 gx_vramcnt_d;
        u8 reserved[1];
        OSIrqFunction irqFunction;
        OSIrqMask irqMask;
        BOOL irq;
        OSIntrMode interrupts;
    } shelter;

    {
        shelter.gx_dispstat = reg_GX_DISPSTAT;
        shelter.gx_dispcnt = reg_GX_DISPCNT;
        shelter.gxs_dispcnt = reg_GXS_DB_DISPCNT;
        shelter.gx_bright = reg_GX_MASTER_BRIGHT;
        shelter.gxs_bright = reg_GXS_DB_MASTER_BRIGHT;
        shelter.gx_powcnt = reg_GX_POWCNT;
        shelter.gx_vramcnt_a = reg_GX_VRAMCNT_A;
        shelter.gx_vramcnt_c = reg_GX_VRAMCNT_C;
        shelter.gx_vramcnt_d = reg_GX_VRAMCNT_D;
        shelter.gx_bg0cnt = reg_G2_BG0CNT;
        shelter.gxs_bg0cnt = reg_G2S_DB_BG0CNT;
        shelter.gx_bg0ofs = reg_G2_BG0OFS;
        shelter.gxs_bg0ofs = reg_G2S_DB_BG0OFS;

        reg_GX_DISPCNT = 0;
        reg_GXS_DB_DISPCNT = 0;

        if (!(shelter.gx_powcnt & REG_GX_POWCNT_LCD_MASK)) {
            SVC_WaitByLoop(HW_CPU_CLOCK_ARM9 / 40);
        }

        reg_GX_POWCNT = (u16)(REG_GX_POWCNT_DSEL_MASK | REG_GX_POWCNT_E2DG_MASK |
                              REG_GX_POWCNT_E2DGB_MASK | REG_GX_POWCNT_LCD_MASK);

        reg_GX_MASTER_BRIGHT = (u16)((1 << REG_GX_MASTER_BRIGHT_E_MOD_SHIFT) | 16);
        reg_GXS_DB_MASTER_BRIGHT = reg_GX_MASTER_BRIGHT;

        {
            reg_GX_VRAMCNT_A = (u8)((1 << REG_GX_VRAMCNT_A_MST_SHIFT) | (1 << REG_GX_VRAMCNT_A_E_SHIFT));
            reg_G2_BG0CNT =
                (u16)((GX_BG_SCRSIZE_TEXT_256x256 << REG_G2_BG0CNT_SCREENSIZE_SHIFT) |
                      (GX_BG_COLORMODE_256 << REG_G2_BG0CNT_COLORMODE_SHIFT) | (GX_BG_SCRBASE_0xf000
                                                                                <<
                                                                                REG_G2_BG0CNT_SCREENBASE_SHIFT)
                      | (GX_BG_CHARBASE_0x00000 << REG_G2_BG0CNT_CHARBASE_SHIFT) | (GX_BG_EXTPLTT_01
                                                                                    <<
                                                                                    REG_G2_BG0CNT_BGPLTTSLOT_SHIFT)
                      | (0 << REG_G2_BG0CNT_PRIORITY_SHIFT));
            reg_G2_BG0OFS = 0;
            reg_GX_DISPCNT |= ((GX_BGMODE_0 << REG_GX_DISPCNT_BGMODE_SHIFT) |
                               (GX_PLANEMASK_BG0 << REG_GX_DISPCNT_DISPLAY_SHIFT));

            {
                void * const src = LoadImage(IMAGE_LOGO_CHR, NULL);
                MI_UncompressLZ16(src, (u32 *)HW_BG_VRAM);
            }

            {
                u8 * const src = LoadImage(IMAGE_LOGO_SCR, NULL);
                s32 i;
                s32 j;
                u16 temp;

                SVC_CpuClearFast(0, (u32 *)(HW_BG_VRAM + 0xf000), 1536);
                for (i = 0; i < 12; i++) {
                    for (j = 0; j < 12; j++) {
                        temp = (u16)(src[(i * 12) + j]);
                        *(u16 *)(HW_BG_VRAM + 0xf194 + (i * 64) + (j * 2)) = temp;
                    }
                }
            }

            {
                u32 size;
                void * const src = LoadImage(IMAGE_LOGO_PAL, &size);
                SVC_CpuCopyFast(src, (u32 *)(HW_BG_PLTT), size);
            }
        }

        {
            reg_GX_VRAMCNT_C = (u8)((4 << REG_GX_VRAMCNT_C_MST_SHIFT) | (1 << REG_GX_VRAMCNT_C_E_SHIFT));
            reg_GX_VRAMCNT_D = (u8)((4 << REG_GX_VRAMCNT_D_MST_SHIFT) | (1 << REG_GX_VRAMCNT_D_E_SHIFT));
            reg_G2S_DB_BG0CNT =
                (u16)((GX_BG_SCRSIZE_TEXT_256x256 << REG_G2S_DB_BG0CNT_SCREENSIZE_SHIFT) |
                      (GX_BG_COLORMODE_16 << REG_G2S_DB_BG0CNT_COLORMODE_SHIFT) |
                      (GX_BG_SCRBASE_0xf000 << REG_G2S_DB_BG0CNT_SCREENBASE_SHIFT) |
                      (GX_BG_CHARBASE_0x00000 << REG_G2S_DB_BG0CNT_CHARBASE_SHIFT) |
                      (GX_BG_EXTPLTT_01 << REG_G2S_DB_BG0CNT_BGPLTTSLOT_SHIFT) | (0 <<
                                                                                  REG_G2S_DB_BG0CNT_PRIORITY_SHIFT));
            reg_G2S_DB_BG0OFS = 0;
            reg_GXS_DB_DISPCNT |= ((GX_BGMODE_0 << REG_GXS_DB_DISPCNT_BGMODE_SHIFT) |
                                   ((GX_PLANEMASK_BG0 | GX_PLANEMASK_OBJ) <<
                                    REG_GXS_DB_DISPCNT_DISPLAY_SHIFT));

            {
                u32 size;
                void * src;
                MIUnpackBitsParam param = { 0, 1, 4, 0, 0 };

                src = LoadImage(IMAGE_NOTES_CHR, &size);
                param.srcNum = (u16)size;
                SVC_UnpackBits(src, (u32 *)(HW_DB_BG_VRAM), &param);

                src = LoadImage(IMAGE_NOTES_DIGIT, &size);
                param.srcNum = (u16)size;
                SVC_UnpackBits(src, (u32 *)(HW_DB_OBJ_VRAM), &param);
                SVC_CpuClear(0xC0, (void *)HW_DB_OAM, HW_OAM_SIZE, 32);
            }

            {
                void * const src = LoadImage(IMAGE_NOTES_SCR, NULL);
                MI_UncompressLZ16(src, (u32 *)(HW_DB_BG_VRAM + 0xf000));
            }

            SetISBNString(isbn);

            *((u16 *)(HW_DB_BG_PLTT + 0)) = 0x0000;
            *((u16 *)(HW_DB_BG_PLTT + 2)) = 0x7fff;
            *((u16 *)(HW_DB_OBJ_PLTT + 0)) = 0x7fff;
            *((u16 *)(HW_DB_OBJ_PLTT + 2)) = 0x0000;
        }

        reg_GX_DISPCNT |= (u32)(GX_DISPMODE_GRAPHICS << REG_GX_DISPCNT_MODE_SHIFT);
        reg_GXS_DB_DISPCNT |= (u32)(REG_GXS_DB_DISPCNT_MODE_MASK);
        reg_GX_DISPSTAT |= REG_GX_DISPSTAT_VBI_MASK;

        shelter.irqFunction = OS_GetIrqFunction(OS_IE_V_BLANK);
        OS_SetIrqFunction(OS_IE_V_BLANK, VBlankIntr);
        shelter.irqMask = OS_EnableIrqMask(OS_IE_V_BLANK);
        shelter.irq = OS_EnableIrq();
        shelter.interrupts = OS_EnableInterrupts();

    }

    {
        s32 i;
        u16 pad_old;
        u16 pad;

        for (i = 0; i < OSi_FORCHINA_FADE_SPAN; i++) {
            reg_GX_MASTER_BRIGHT = (u16)((1 << REG_GX_MASTER_BRIGHT_E_MOD_SHIFT) |
                                         16 - ((i + 1) * 16 / OSi_FORCHINA_FADE_SPAN));
            reg_GXS_DB_MASTER_BRIGHT = reg_GX_MASTER_BRIGHT;
            WaitForNextFrame();
        }

        pad_old = PAD_Read();
        for (i = 0; i < OSi_FORCHINA_DISP_SPAN; i++) {
            pad = PAD_Read();
            if (~pad_old & pad) {

                break;
            }
            pad_old = pad;
            WaitForNextFrame();
        }

        for (i = 0; i < OSi_FORCHINA_FADE_SPAN; i++) {
            reg_GX_MASTER_BRIGHT = (u16)((1 << REG_GX_MASTER_BRIGHT_E_MOD_SHIFT) |
                                         (i + 1) * 16 / OSi_FORCHINA_FADE_SPAN);
            reg_GXS_DB_MASTER_BRIGHT = reg_GX_MASTER_BRIGHT;
            WaitForNextFrame();
        }
    }

    {

        reg_GX_DISPCNT &= ~(GX_DISPMODE_GRAPHICS << REG_GX_DISPCNT_MODE_SHIFT);
        reg_GXS_DB_DISPCNT &= ~REG_GXS_DB_DISPCNT_MODE_MASK;

        (void)OS_RestoreInterrupts(shelter.interrupts);
        (void)OS_RestoreIrq(shelter.irq);
        (void)OS_SetIrqMask(shelter.irqMask);
        OS_SetIrqFunction(OS_IE_V_BLANK, shelter.irqFunction);
        reg_GX_DISPSTAT &= ~REG_GX_DISPSTAT_VBI_MASK;

        SVC_CpuClearFast(0, (u32 *)HW_BG_VRAM, 0x10000);
        SVC_CpuClearFast(0, (u32 *)HW_BG_PLTT, 0x200);
        SVC_CpuClearFast(0, (u32 *)HW_DB_BG_VRAM, 0x10000);
        SVC_CpuClearFast(0, (u32 *)HW_DB_BG_PLTT, 0x04);
        SVC_CpuClearFast(0, (u32 *)HW_DB_OAM, HW_OAM_SIZE);

        reg_G2S_DB_BG0OFS = shelter.gxs_bg0ofs;
        reg_G2_BG0OFS = shelter.gx_bg0ofs;
        reg_G2S_DB_BG0CNT = shelter.gxs_bg0cnt;
        reg_G2_BG0CNT = shelter.gx_bg0cnt;
        reg_GX_VRAMCNT_D = shelter.gx_vramcnt_d;
        reg_GX_VRAMCNT_C = shelter.gx_vramcnt_c;
        reg_GX_VRAMCNT_A = shelter.gx_vramcnt_a;
        reg_GX_POWCNT = shelter.gx_powcnt;
        if (!(shelter.gx_powcnt & REG_GX_POWCNT_LCD_MASK)) {

            SVC_WaitByLoop(HW_CPU_CLOCK_ARM9 / 40);
        }
        reg_GXS_DB_MASTER_BRIGHT = shelter.gxs_bright;
        reg_GX_MASTER_BRIGHT = shelter.gx_bright;
        reg_GXS_DB_DISPCNT = shelter.gxs_dispcnt;
        reg_GX_DISPCNT = shelter.gx_dispcnt;
        reg_GX_DISPSTAT = shelter.gx_dispstat;
    }
}
