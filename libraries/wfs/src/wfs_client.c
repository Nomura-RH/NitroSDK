#include <nitro/wfs/client.h>

inline static void WFSi_NotifyEvent (WFSClientContext * context, WFSEventType event, void * argument)
{
    if (context->callback) {
        context->callback(context, event, argument);
    }
}

static void WFSi_ReallocBitmap (WFSClientContext * context, int length)
{
    if (length < 0) {
        length = (int)context->max_file_size;
        context->max_file_size = 0;
    }

    if (context->max_file_size < length) {
        const int packet = WBT_GetParentPacketLength(context->wbt);
        const u32 newsize = WBT_PACKET_BITMAP_SIZE((u32)length, packet);
        context->max_file_size = (u32)length;
        MI_CallFree(context->allocator, context->recv_pkt_bmp_buf);
        context->recv_pkt_bmp_buf = (u32 *)MI_CallAlloc(context->allocator, sizeof(u32) * newsize, sizeof(u32));
        context->recv_buf_packet_bmp_table.packet_bitmap[0] = context->recv_pkt_bmp_buf;
    }

    SDK_ASSERT(context->recv_pkt_bmp_buf);
}

static void WFSi_ReceiveTableSequence (void * userdata, WBTCommand * callback)
{
    WFSClientContext * const context = (WFSClientContext *)userdata;
    WBTContext * const wbt = context->wbt;

    if ((callback == NULL) || (callback->result == WBT_RESULT_SUCCESS)) {
        BOOL post = FALSE;
        const int bitmap = 0x0001;

        if (callback == NULL) {
            WFS_DEBUG_OUTPUT(("WBT-SYNC():post"));
            post = WBT_PostCommandSYNC(wbt, bitmap, WFSi_ReceiveTableSequence);
        } else if (callback->event == WBT_CMD_RES_SYNC)   {
            WFS_DEBUG_OUTPUT(("WBT-SYNC():done [server = %d, client = %d]",
                              callback->sync.peer_packet_size + WBT_PACKET_SIZE_MIN,
                              callback->sync.my_packet_size + WBT_PACKET_SIZE_MIN));
            WFS_DEBUG_OUTPUT(("WBT-INFO(0):post"));
            post = WBT_PostCommandINFO(wbt, bitmap, WFSi_ReceiveTableSequence,
                                       0, &context->block_info_table);
        } else if (callback->event == WBT_CMD_RES_GET_BLOCKINFO)   {
            const int length = context->block_info_table.block_info[0]->block_size;
            WFS_DEBUG_OUTPUT(("WBT-INFO(0):done [table-length = %d]", length));
            context->table->length = (u32)length;
            context->table->buffer = (u8 *)MI_CallAlloc(context->allocator, (u32)length, 1);
            SDK_ASSERT(context->table->buffer);
            WFSi_ReallocBitmap(context, length);
            context->recv_buf_table.recv_buf[0] = context->table->buffer;
            WFS_DEBUG_OUTPUT(("WBT-GET(0x%08X):post", WFS_TABLE_BLOCK_INDEX));
            post = WBT_PostCommandGET(wbt, bitmap, WFSi_ReceiveTableSequence,
                                      WFS_TABLE_BLOCK_INDEX, context->table->length,
                                      &context->recv_buf_table,
                                      &context->recv_buf_packet_bmp_table);
        } else if (callback->event == WBT_CMD_RES_GET_BLOCK)   {
            WFS_DEBUG_OUTPUT(("WBT-GET(0x%08X):done [ready for mount]", WFS_TABLE_BLOCK_INDEX));
            WFS_ParseTable(context->table);
            context->fat_ready = TRUE;
            WFSi_NotifyEvent(context, WFS_EVENT_CLIENT_READY, NULL);
            post = TRUE;
        }

        if (!post) {
            WFS_DEBUG_OUTPUT(("internal-error (failed to post WBT command)"));
        }
    } else {
        if (callback->event == WBT_CMD_CANCEL) {
        } else {
            WFS_DEBUG_OUTPUT(("internal-error (unexpected WBT result)"));
            WFS_DEBUG_OUTPUT(("  command = %d", callback->command));
            WFS_DEBUG_OUTPUT(("  event   = %d", callback->event));
            WFS_DEBUG_OUTPUT(("  result  = %d", callback->result));
        }
    }
}

static void WFSi_ReadRomSequence (void * userdata, WBTCommand * callback)
{
    WFSClientContext * const context = (WFSClientContext *)userdata;
    WBTContext * const wbt = context->wbt;

    if ((callback == NULL) || (callback->result == WBT_RESULT_SUCCESS)) {
        BOOL post = FALSE;
        const int bitmap = 0x0001;

        if (callback == NULL) {
            WFS_DEBUG_OUTPUT(("WBT-MSG(LOCK):post"));
            post = WFS_SendMessageLOCK_REQ(wbt, WFSi_ReadRomSequence, bitmap,
                                           context->request_region.offset + context->table->origin,
                                           context->request_region.length);
        } else if (callback->event == WBT_CMD_RES_USER_DATA)   {
            post = TRUE;
        } else {
            const WFSMessageFormat * const msg = (const WFSMessageFormat *)callback->user_data.data;

            if ((callback->event == WBT_CMD_REQ_USER_DATA) &&
                (msg->type == WFS_MSG_LOCK_ACK)) {
                BOOL accepted = (BOOL)MI_LEToH32(msg->arg2);

                if (!accepted) {
                    WFS_DEBUG_OUTPUT(("WBT-MSG(LOCK):failed [packet-length renewal]"));
                    WFS_DEBUG_OUTPUT(("WBT-SYNC():post"));
                    post = WBT_PostCommandSYNC(wbt, bitmap, WFSi_ReadRomSequence);
                } else {
                    u32 id = MI_LEToH32(msg->arg1);
                    WFS_DEBUG_OUTPUT(("WBT-MSG(LOCK):done [lock-id = 0x%08X]", id));
                    context->block_id = id;
                    context->recv_buf_table.recv_buf[0] = context->request_buffer;
                    WFS_DEBUG_OUTPUT(("WBT-GET(0x%08X):post", id));
                    post = WBT_PostCommandGET(wbt, bitmap, WFSi_ReadRomSequence,
                                              context->block_id, context->request_region.length,
                                              &context->recv_buf_table,
                                              &context->recv_buf_packet_bmp_table);
                }
            } else if (callback->event == WBT_CMD_RES_SYNC)   {
                WFS_DEBUG_OUTPUT(("WBT-SYNC():done [server = %d, client = %d]",
                                  callback->sync.peer_packet_size + WBT_PACKET_SIZE_MIN,
                                  callback->sync.my_packet_size + WBT_PACKET_SIZE_MIN));
                WFSi_ReallocBitmap(context, -1);
                WFSi_ReadRomSequence(context, NULL);
                post = TRUE;
            } else if (callback->event == WBT_CMD_RES_GET_BLOCK)   {
                u32 id = context->block_id;
                WFS_DEBUG_OUTPUT(("WBT-GET(0x%08X):done", id));
                WFS_DEBUG_OUTPUT(("WBT-MSG(UNLOCK,0x%08X):post", id));
                post = WFS_SendMessageUNLOCK_REQ(wbt, WFSi_ReadRomSequence, bitmap, id);
            } else if ((callback->event == WBT_CMD_REQ_USER_DATA) &&
                       (msg->type == WFS_MSG_UNLOCK_ACK)) {
                u32 id = context->block_id;
                WFS_DEBUG_OUTPUT(("WBT-MSG(UNLOCK,0x%08X):done [read-operation completed]", id));
                context->request_buffer = NULL;
                {
                    WFSRequestClientReadDoneCallback callback = context->request_callback;
                    void * argument = context->request_argument;
                    context->request_callback = NULL;
                    context->request_argument = NULL;
                    if (callback) {
                        (*callback)(context, TRUE, argument);
                    }
                }
                post = TRUE;
            }
        }

        if (!post) {
            WFS_DEBUG_OUTPUT(("internal-error (failed to post WBT command)"));
        }
    } else {

        if (callback->event == WBT_CMD_CANCEL) {
        } else {
            WFS_DEBUG_OUTPUT(("internal-error (unexpected WBT result)"));
            WFS_DEBUG_OUTPUT(("  command = %d", callback->command));
            WFS_DEBUG_OUTPUT(("  event   = %d", callback->event));
            WFS_DEBUG_OUTPUT(("  result  = %d", callback->result));
        }
    }
}

static void WFSi_WBTSystemCallback (void * userdata, WBTCommand * callback)
{
    WFSClientContext * const context = (WFSClientContext *)userdata;

    if ((callback->event == WBT_CMD_REQ_USER_DATA) &&
        (context->request_buffer)) {
        WFSi_ReadRomSequence(context, callback);
    }
}

void WFS_CallClientConnectHook (WFSClientContext * context, const WFSPeerInfo * peer)
{
    (void)context;
    (void)peer;
}

void WFS_CallClientDisconnectHook (WFSClientContext * context, const WFSPeerInfo * peer)
{
    (void)context;
    (void)peer;
}

void WFS_CallClientPacketSendHook (WFSClientContext * context, WFSPacketBuffer * packet)
{
    packet->length = WBT_CallPacketSendHook(context->wbt, packet->buffer, packet->length, FALSE);
}

void WFS_CallClientPacketRecvHook (WFSClientContext * context, const WFSPacketBuffer * packet)
{
    int aid = (int)MATH_CTZ((u32)packet->bitmap);
    WBT_CallPacketRecvHook(context->wbt, aid, packet->buffer, packet->length);
}

void WFS_InitClient (WFSClientContext * context, void * userdata, WFSEventCallback callback, MIAllocator * allocator)
{
    int i;

    context->userdata = userdata;
    context->callback = callback;
    context->allocator = allocator;
    context->fat_ready = FALSE;

    for (i = 0; i < WBT_NUM_OF_AID; ++i) {
        context->block_info_table.block_info[i] = &context->block_info[i];
        context->recv_buf_table.recv_buf[i] = NULL;
        context->recv_buf_packet_bmp_table.packet_bitmap[i] = NULL;
    }

    context->recv_pkt_bmp_buf = NULL;
    context->max_file_size = 0;
    context->block_id = 0;
    context->request_buffer = NULL;
    context->table->length = 0;
    context->table->buffer = NULL;

    WBT_InitContext(context->wbt, context, WFSi_WBTSystemCallback);
    WBT_AddCommandPool(context->wbt, context->wbt_list, sizeof(context->wbt_list) / sizeof(*context->wbt_list));
}

void WFS_EndClient (WFSClientContext * context)
{
    MI_CallFree(context->allocator, context->recv_pkt_bmp_buf);
    WBT_ResetContext(context->wbt, NULL);

    if (context->table->buffer) {
        MI_CallFree(context->allocator, context->table->buffer);
        context->table->buffer = NULL;
    }

    if (context->request_callback) {
        (*context->request_callback)(context->request_argument, FALSE, context->request_argument);
    }
}

void WFS_StartClient (WFSClientContext * context, const WFSPeerInfo * peer)
{
    WBT_StartChild(context->wbt, peer->aid);
    WFSi_ReceiveTableSequence(context, NULL);
}

void WFS_RequestClientRead (WFSClientContext * context, void * buffer, u32 offset, u32 length, WFSRequestClientReadDoneCallback callback, void * arg)
{
    if (context->fat_ready) {
        context->request_buffer = buffer;
        context->request_region.offset = offset;
        context->request_region.length = length;
        context->request_callback = callback;
        context->request_argument = arg;

        WFSi_ReallocBitmap(context, (int)length);
        WFSi_ReadRomSequence(context, NULL);
    }
}

void WFS_GetClientReadProgress (WFSClientContext * context, int * current, int * total)
{
    WBT_GetDownloadProgress(context->wbt, context->block_id, 0, current, total);
}
