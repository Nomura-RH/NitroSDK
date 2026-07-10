#ifndef _MY_ROM_FILE_H_
#define _MY_ROM_FILE_H_

#define FILE_NAME_LENGTH 128

typedef struct {
    u32 entry_start; // エントリ名の検索位置
    u16 entry_file_id; // 先頭エントリのファイル ID
    u16 parent_id; // 親ディレクトリの ID
} ROM_FNTDir;

typedef struct {
    u8 entry_type : 1; // ファイルエントリの場合は 0
    u8 entry_name_length : 7; // ファイル名の長さ (0-127)
    char entry_name[FILE_NAME_LENGTH]; // ファイル名 (終端 \0 は省く)
} ROM_FNTStrFile;

typedef struct {
    u8 entry_type : 1; // ディレクトリエントリの場合は 1
    u8 entry_name_length : 7; // ディレクトリ名の長さ (0-127)
    char entry_name[FILE_NAME_LENGTH]; // ディレクトリ名 (終端 \0 は省く)
    u8 dir_id_L; // ディレクトリ ID Low  8bit
    u8 dir_id_H; // ディレクトリ ID High 8bit
} ROM_FNTStrDir;

typedef struct {
    char entry_name[FILE_NAME_LENGTH]; // ファイル名 (終端 \0 は省く)
} ROM_FNT;

typedef struct {
    void *top; // ファイルの先頭 ROM アドレス
    void *bottom; // ファイルの最終 ROM アドレス
} ROM_FAT;

typedef struct {
    u32 id; // オーバーレイ ID
    void *ram_address; // ロード先頭位置
    u32 ram_size; // ロードサイズ
    u32 bss_size; // bss 領域サイズ
    void *sinit_init; // static initializer 先頭アドレス
    void *sinit_init_end; // static initializer 最終アドレス
    u32 file_id; // オーバーレイファイルID
    u32 rom_size; // オーバーレイファイルサイズ
} ROM_OVT;

typedef struct {
    u32 dummy1[32 / sizeof(u32)];

    // 0x020 for Static modules (Section:B)
    //
    //	ARM9
    u32 main_rom_offset; // ROM offset
    void *main_entry_address; // Entry point
    void *main_ram_address; // RAM address
    u32 main_size; // Module size

    //	ARM7
    u32 sub_rom_offset; // ROM offset
    void *sub_entry_address; // Entry point
    void *sub_ram_address; // RAM address
    u32 sub_size; // Module size

    // 0x040 c) ファイルネームテーブル用パラメータ
    //
    ROM_FNT *fnt_offset; // 先頭 ROM オフセット
    u32 fnt_size; // テーブルサイズ

    //
    // 0x048 e) ファイルアロケーションテーブル用パラメータ
    //
    ROM_FAT *fat_offset; // 先頭 ROM オフセット
    u32 fat_size; // テーブルサイズ

    //
    // 0x0050 d) オーバーレイヘッダテーブル用パラメータ
    //
    //	ARM9
    ROM_OVT *main_ovt_offset; // 先頭 ROM オフセット
    u32 main_ovt_size; // テーブルサイズ

    //	ARM7
    ROM_OVT *sub_ovt_offset; // 先頭 ROM オフセット
    u32 sub_ovt_size; // テーブルサイズ

    u32 dummy2[272 / sizeof(u32)];
} RomHeader;

static inline void *MASK_ROM_ADDR(u32 offset)
{
    return (void *)(0x08000000 + offset);
}

#define ROM_FILE_MAIN_PROCESSOR 0
#define ROM_FILE_SUB_PROCESSOR  1

#define ROM_FILE_CARD      0
#define ROM_FILE_CARTRIDGE 1

BOOL my_romfile_load_overlay_segment(int card_or_cart, int main_or_sub, u32 id);
BOOL get_overlay_info(int card_or_cart, int main_or_sub, u32 id, ROM_OVT *ovt);
BOOL my_romfile_load_static_segment(int card_or_cart, int main_or_sub, void **entry);
BOOL get_filenametabledir_info(u32 name_no, ROM_FNTDir *fntdir);
int get_filenametabledir_num(void);

#endif /* _MY_ROM_FILE_H_ */
