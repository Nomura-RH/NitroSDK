#include <nitro/wfs/client.h>

static FSResult WFSi_ArchiveReadCallback (FSArchive * archive, void * buffer, u32 offset, u32 length)
{
    FSResult result = FS_RESULT_ERROR;
    WFSClientContext * const context = (WFSClientContext *)FS_GetArchiveBase(archive);
    const WFSTableFormat * const table = WFS_GetTableFormat(context);

    if (table) {
        MI_CpuCopy8(&table->buffer[offset], buffer, length);
        result = FS_RESULT_SUCCESS;
    }

    return result;
}

static void WFSi_ArchiveReadDoneCallback (WFSClientContext * context, BOOL succeeded, void * arg)
{
    FSFile * const file = (FSFile *)arg;
    FSResult result = FS_RESULT_ERROR;

    if (succeeded) {
        file->prop.file.pos += file->arg.readfile.len;
        result = FS_RESULT_SUCCESS;
    }

    FS_NotifyArchiveAsyncEnd(FS_GetAttachedArchive(file), result);
    (void)context;
}

static FSResult WFSi_RomArchiveProc (FSFile * file, FSCommandType command)
{
    FSResult result = FS_RESULT_ERROR;

    switch (command) {
    case FS_COMMAND_READFILE:
    {
        void * buffer = file->arg.readfile.dst;
        u32 offset = file->prop.file.pos;
        u32 length = file->arg.readfile.len;
        if (length == 0) {
            result = FS_RESULT_SUCCESS;
        } else {
            FSArchive * const archive = FS_GetAttachedArchive(file);
            WFSClientContext * const context = (WFSClientContext *)FS_GetArchiveBase(archive);
            const WFSTableFormat * const table = WFS_GetTableFormat(context);

            if (table != NULL) {
                WFS_RequestClientRead(context, buffer, offset, length, WFSi_ArchiveReadDoneCallback, file);
                result = FS_RESULT_PROC_ASYNC;
            }
        }
    }
    break;
    case FS_COMMAND_WRITEFILE:
        result = FS_RESULT_UNSUPPORTED;
        break;
    default:
        result = FS_RESULT_PROC_UNKNOWN;
        break;
    }

    return result;
}

void WFS_ReplaceRomArchive (WFSClientContext * context)
{
    const WFSTableFormat * const table = WFS_GetTableFormat(context);

    if (table != NULL) {
        FSArchive * archive = FS_FindArchive("rom", 3);
        if (FS_IsArchiveLoaded(archive)) {
            (void)FS_UnloadArchive(archive);
        }
        FS_SetArchiveProc(archive, WFSi_RomArchiveProc, (u32)FS_ARCHIVE_PROC_ALL);
        (void)FS_LoadArchive(archive, (u32)context,
                             table->region[WFS_TABLE_REGION_FAT].offset,
                             table->region[WFS_TABLE_REGION_FAT].length,
                             table->region[WFS_TABLE_REGION_FNT].offset,
                             table->region[WFS_TABLE_REGION_FNT].length,
                             WFSi_ArchiveReadCallback, NULL);
        FS_AttachOverlayTable(MI_PROCESSOR_ARM9,
                              table->buffer +
                              table->region[WFS_TABLE_REGION_OV9].offset,
                              table->region[WFS_TABLE_REGION_OV9].length);
        FS_AttachOverlayTable(MI_PROCESSOR_ARM7,
                              table->buffer +
                              table->region[WFS_TABLE_REGION_OV7].offset,
                              table->region[WFS_TABLE_REGION_OV7].length);
    }
}
