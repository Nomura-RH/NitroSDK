#include <nitro/wfs/format.h>

BOOL WFS_LoadTable (WFSTableFormat * archive, MIAllocator * allocator, MIDevice * device, u32 origin_a, u32 origin_b)
{
    BOOL retval = FALSE;
    const u32 diff = origin_b - origin_a;
    const BOOL mixed = (diff != 0);
    u32 mixed_overlay_count = 0;
    u32 origin_b_fat = 0;
    CARDRomRegion mem_region[WFS_TABLE_REGION_MAX];
    int i;

    archive->origin = origin_a;
    archive->buffer = NULL;
    archive->length = 0;

    {
        u8 buf[0x60];
        const CARDRomHeader * header = (const CARDRomHeader *)buf;
        (void)MI_ReadDevice(device, buf, archive->origin, sizeof(buf));
        archive->region[WFS_TABLE_REGION_FAT].offset = MI_LoadLE32(&header->fat.offset);
        archive->region[WFS_TABLE_REGION_FAT].length = MI_LoadLE32(&header->fat.length);
        archive->region[WFS_TABLE_REGION_FNT].offset = MI_LoadLE32(&header->fnt.offset);
        archive->region[WFS_TABLE_REGION_FNT].length = MI_LoadLE32(&header->fnt.length);
    }

    {
        u8 buf[0x60];
        const CARDRomHeader * header = (const CARDRomHeader *)buf;
        (void)MI_ReadDevice(device, buf, origin_b, sizeof(buf));
        archive->region[WFS_TABLE_REGION_OV9].offset = MI_LoadLE32(&header->main_ovt.offset) + diff;
        archive->region[WFS_TABLE_REGION_OV9].length = MI_LoadLE32(&header->main_ovt.length);
        archive->region[WFS_TABLE_REGION_OV7].offset = MI_LoadLE32(&header->sub_ovt.offset) + diff;
        archive->region[WFS_TABLE_REGION_OV7].length = MI_LoadLE32(&header->sub_ovt.length);
        origin_b_fat = header->fat.offset + diff;
    }

    if (mixed) {
        mixed_overlay_count = (archive->region[WFS_TABLE_REGION_OV9].length +
                               archive->region[WFS_TABLE_REGION_OV7].length)
                              / sizeof(WFSOVLFormat);
    }

    {
        u32 offset = 0;
        offset += sizeof(u32);
        for (i = 0; i < WFS_TABLE_REGION_MAX; ++i) {
            mem_region[i].offset = offset;
            mem_region[i].length = archive->region[i].length;
            if (i == WFS_TABLE_REGION_FAT) {
                mem_region[i].length += mixed_overlay_count * sizeof(WFSFATFormat);
            }
            mem_region[i].length = MATH_ROUNDUP(mem_region[i].length, 32);
            offset += sizeof(u32) + mem_region[i].length;
        }
        archive->length = offset;
    }

    if (allocator) {
        archive->buffer = MI_CallAlloc(allocator, archive->length, sizeof(u32));
        if (archive->buffer) {
            {
                u8 * dst = archive->buffer;
                MI_StoreLE32(dst, archive->origin);
                dst += sizeof(u32);
                for (i = 0; i < WFS_TABLE_REGION_MAX; ++i) {
                    const u32 length = archive->region[i].length;
                    const u32 padding = mem_region[i].length - length;
                    MI_StoreLE32(dst, length);
                    dst += sizeof(u32);
                    (void)MI_ReadDevice(device, dst, archive->origin + archive->region[i].offset, length);
                    dst += length;
                    MI_CpuFill8(dst, 0x00, padding);
                    dst += padding;
                }
            }

            if (mixed) {
                u32 count = archive->region[WFS_TABLE_REGION_FAT].length / sizeof(WFSFATFormat);
                u32 total = count + mixed_overlay_count;
                u8 * dst = &archive->buffer[mem_region[WFS_TABLE_REGION_FAT].offset];
                WFSFATFormat * fat = (WFSFATFormat *)(dst + sizeof(u32));
                MI_StoreLE32(dst, total * sizeof(WFSFATFormat));
                for (i = WFS_TABLE_REGION_OV9; i < WFS_TABLE_REGION_MAX; ++i) {
                    WFSOVLFormat * ovl = (WFSOVLFormat *)&archive->buffer[mem_region[i].offset + sizeof(u32)];
                    u32 num = archive->region[i].length / sizeof(WFSOVLFormat);
                    u32 j;
                    for (j = 0; j < num; ++j) {
                        if (count >= total) {
                            OS_TWarning("exceeded FAT iterator! (internal logic error)");
                        } else {
                            WFSFATFormat entry;
                            (void)MI_ReadDevice(device, &entry,
                                                archive->origin + origin_b_fat + ovl[j].file_id * sizeof(WFSFATFormat),
                                                sizeof(entry));
                            MI_StoreLE32(&fat[count].top, MI_LEToH32(entry.top) + diff);
                            MI_StoreLE32(&fat[count].bottom, MI_LEToH32(entry.bottom) + diff);
                            MI_StoreLE32(&ovl[j].file_id, count);
                            ++count;
                        }
                    }

                }
            }
        }
    }
    return (archive->buffer != NULL);
}

void WFS_ParseTable (WFSTableFormat * archive)
{
    const u8 * src = archive->buffer;
    u32 pos = 0;
    int i;

    archive->origin = MI_LoadLE32(&src[pos]);
    pos += sizeof(u32);

    for (i = 0; i < WFS_TABLE_REGION_MAX; ++i) {
        u32 len = MI_LoadLE32(&src[pos]);
        pos += sizeof(u32);
        archive->region[i].offset = pos;
        archive->region[i].length = len;
        pos += MATH_ROUNDUP(len, 32);
        if (pos > archive->length) {
            OS_TPanic("specified invalid archive buffer!");
        }
    }
}
