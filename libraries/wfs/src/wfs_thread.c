#include <nitro.h>

#define WFS_FILE_CACHE_SIZE     1024UL
#define WFS_CACHE_PAGE_TOTAL    8

typedef struct WFSServerThread {
    WFSEventCallback hook;
    MIDevice device[1];
    FSFile file[1];
    MICache cache[1];
    u8 cache_buf[MI_CACHE_BUFFER_WORKSIZE(WFS_FILE_CACHE_SIZE, WFS_CACHE_PAGE_TOTAL)];
    OSMessageQueue msg_queue[1];
    OSMessage msg_array[1];
    OSThread thread[1];
    u8 thread_stack[0x400];
} WFSServerThread;

static int WFSi_ReadRomCallback (void * userdata, void * buffer, u32 offset, u32 length)
{
    WFSServerThread * const thread = (WFSServerThread *)userdata;

    if(offset < sizeof(CARDRomRegion)) {
        const u8 * header = CARD_GetRomHeader();
        if (length > sizeof(CARDRomHeader) - offset) {
            length = sizeof(CARDRomHeader) - offset;
        }
        MI_CpuCopy8(header + offset, buffer, length);
    } else {
        if (offset < 0x8000) {
            u32 memlen = length;
            if (memlen > 0x8000 - offset) {
                memlen = 0x8000 - offset;
            }
            MI_CpuFill8(buffer, 0x00, length);
            offset += memlen;
            length -= memlen;
        }
        if (length > 0) {
            (void)FS_SeekFile(thread->file, (int)offset, FS_SEEK_SET);
            (void)FS_ReadFile(thread->file, buffer, (int)length);
        }
    }
    return (int)length;
}

static int WFSi_WriteRomCallback (void * userdata, const void * buffer, u32 offset, u32 length)
{
    (void)userdata;
    (void)buffer;
    (void)offset;
    (void)length;
    return -1;
}

static void WFSi_ServerThreadProc (void * arg)
{
    WFSServerThread * const thread = (WFSServerThread *)arg;
    for (;;) {
        BOOL busy = FALSE;
        (void)OS_ReceiveMessage(thread->msg_queue, (OSMessage *)&busy, OS_MESSAGE_BLOCK);
        if (!busy) {
            break;
        }
        MI_LoadCache(thread->cache, thread->device);
    }
}

static void WFSi_ThreadHook (void * work, void * argument)
{
    WFSServerThread * const thread = (WFSServerThread *)work;
    WFSSegmentBuffer * const segment = (WFSSegmentBuffer *)argument;

    if (!segment) {
        (void)OS_SendMessage(thread->msg_queue, (OSMessage)FALSE, OS_MESSAGE_BLOCK);
        OS_JoinThread(thread->thread);
        (void)FS_CloseFile(thread->file);
    } else if (!MI_ReadCache(thread->cache, segment->buffer, segment->offset, segment->length))   {

        segment->buffer = NULL;
        (void)OS_SendMessage(thread->msg_queue, (OSMessage)TRUE, OS_MESSAGE_NOBLOCK);
    }
}

BOOL WFS_ExecuteRomServerThread (WFSServerContext * context, FSFile * file, BOOL sharedFS)
{
    BOOL retval = FALSE;

    WFSServerThread * thread = MI_CallAlloc(context->allocator, sizeof(WFSServerThread), 32);
    if (!thread) {
        OS_TWarning("failed to allocate memory enough to create internal thread.");
    } else {

        u32 pos = file ? (FS_GetFileImageTop(file) + FS_GetPosition(file)) : 0;
        u32 fatbase = (u32)((file && !sharedFS) ? pos : 0);
        u32 overlay = (u32)(file ? pos : 0);

        FS_InitFile(thread->file);
        if (!FS_CreateFileFromRom(thread->file, 0, 0x7FFFFFFF)) {
            OS_TPanic("failed to create ROM-file!");
        }

        MI_InitDevice(thread->device, thread, WFSi_ReadRomCallback, WFSi_WriteRomCallback);
        MI_InitCache(thread->cache, WFS_FILE_CACHE_SIZE, thread->cache_buf, sizeof(thread->cache_buf));

        if (WFS_RegisterServerTable(context, thread->device, fatbase, overlay)) {
            context->thread_work = thread;
            context->thread_hook = WFSi_ThreadHook;

            OS_InitMessageQueue(thread->msg_queue, thread->msg_array, 1);
            OS_CreateThread(thread->thread, WFSi_ServerThreadProc, thread,
                            thread->thread_stack + sizeof(thread->thread_stack),
                            sizeof(thread->thread_stack), 15);
            OS_WakeupThreadDirect(thread->thread);
            retval = TRUE;
        } else {
            MI_CallFree(context->allocator, thread);
        }
    }
    return retval;
}
