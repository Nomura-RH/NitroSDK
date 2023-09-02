#include <nitro/wfs/server.h>

static void WFSi_NotifySegmentEvent (WFSServerContext * context, void * argument)
{
    if (context->thread_work) {
        (*context->thread_hook)(context->thread_work, argument);
    } else if (context->callback)   {
        (*context->callback)(context, WFS_EVENT_SERVER_SEGMENT_REQUEST, argument);
    }
}

static void WFSi_WBTCallback (void * userdata, WBTCommand * uc)
{
    WFSServerContext * work = (WFSServerContext *)userdata;
    const int aid = (int)MATH_CTZ(uc->peer_bmp);

    switch (uc->command) {
    default:
        WFS_DEBUG_OUTPUT(("invalid WBT callback (command = %d)", uc->command));
        break;
    case WBT_CMD_REQ_USER_DATA:
        if (!uc->target_bmp) {
            work->msg_busy = FALSE;
        }
        break;
    case WBT_CMD_SYSTEM_CALLBACK:
        switch (uc->event) {
        default:
            WFS_DEBUG_OUTPUT(("unknown WBT system callback (event = %d)", uc->event));
            break;
        case WBT_CMD_REQ_SYNC:
        case WBT_CMD_REQ_GET_BLOCK_DONE:
            break;
        case WBT_CMD_REQ_USER_DATA:
        {
            const WFSMessageFormat * const msg = (const WFSMessageFormat *)uc->user_data.data;
            work->recv_msg[aid] = *msg;

            switch (msg->type) {
            case WFS_MSG_LOCK_REQ:
            {
                const u32 offset = MI_LEToH32(msg->arg1);
                const u32 length = MI_LEToH32_BITFIELD(24, msg->arg2);
                const int packet = (int)((msg->packet_hi << 8) | msg->packet_lo);

                WFS_DEBUG_OUTPUT(("WBT-MSG(LOCK):recv [offset=0x%08X, length=0x%08X] (AID=%d, packet=%d)", offset, length, aid, packet));

                if (work->is_changing ||
                    (packet != WBT_GetParentPacketLength(work->wbt) + WBT_PACKET_SIZE_MIN)) {
                    work->is_changing = TRUE;
                    work->deny_bitmap |= (1 << aid);
                } else {
                    WFSLockInfo * file = NULL;
                    int index;
                    for (index = 0; (1 << index) <= work->use_bitmap; ++index) {
                        if(((1 << index) & work->use_bitmap) != 0) {
                            if ((work->list[index].offset == offset) &&
                                (work->list[index].length == length)) {
                                file = &work->list[index];
                                ++file->ref;
                                break;
                            }

                        }

                    }

                    if (!file) {
                        index = (int)MATH_CTZ((u32) ~work->use_bitmap);
                        if (index < WFS_LOCK_HANDLE_MAX) {
                            PLATFORM_ENTER_CRITICALSECTION();
                            work->use_bitmap |= (1 << index);
                            file = &work->list[index];
                            file->ref = 1;
                            file->offset = offset;
                            file->length = length;

                            WBT_RegisterBlockInfo(work->wbt, &file->info,
                                                  (u32)(WFS_LOCKED_BLOCK_INDEX + index),
                                                  NULL, NULL, (int)file->length);
                            file->ack_seq = 0;
                            PLATFORM_LEAVE_CRITICALSECTION();

                            {
                                WFSSegmentBuffer segment[1];
                                segment->offset = file->offset;
                                segment->length = (u32)WBT_GetParentPacketLength(work->wbt);
                                segment->buffer = NULL;
                                WFSi_NotifySegmentEvent(work, segment);
                            }
                        } else {
                            OS_TPanic("internal-error (no available lock handles)");
                        }
                    }
                    work->ack_bitmap |= (1 << aid);
                    work->recv_msg[aid].arg1 = MI_HToLE32((u32)(WFS_LOCKED_BLOCK_INDEX + index));
                }
                work->busy_bitmap |= (1 << aid);
            }
            break;
            case WFS_MSG_UNLOCK_REQ:
            {
                PLATFORM_ENTER_CRITICALSECTION();
                u32 id = MI_LEToH32(msg->arg1);
                u32 index = id - WFS_LOCKED_BLOCK_INDEX;
                if (index < WFS_LOCK_HANDLE_MAX) {
                    WFSLockInfo * file = &work->list[index];

                    if (--file->ref <= 0) {
                        (void)WBT_UnregisterBlockInfo(work->wbt, id);
                        work->use_bitmap &= ~(1 << index);
                    }
                }
                work->ack_bitmap |= (1 << aid);
                PLATFORM_LEAVE_CRITICALSECTION();
                WFS_DEBUG_OUTPUT(("WBT-MSG(UNLOCK):recv [id=0x%08X] (AID=%d)", id, aid));
            }
            break;
            }
        }
        break;
        case WBT_CMD_PREPARE_SEND_DATA:
        {
            WBTPrepareSendDataCallback * const p_prep = &uc->prepare_send_data;
            u32 id = p_prep->block_id;
            p_prep->data_ptr = NULL;

            id -= WFS_LOCKED_BLOCK_INDEX;
            if (id < WFS_LOCK_HANDLE_MAX) {
                WFSLockInfo * file = &work->list[id];

                WFSSegmentBuffer segment[1];
                const u32 length = p_prep->own_packet_size;
                const u32 current = file->ack_seq;
                const u32 next = (u32)p_prep->block_seq_no;
                file->ack_seq = next;

                segment->offset = file->offset + length * next;
                segment->length = length;
                segment->buffer = NULL;
                WFSi_NotifySegmentEvent(work, segment);

                segment->offset = file->offset + length * current;
                segment->length = length;
                segment->buffer = work->cache_hit_buf;
                WFSi_NotifySegmentEvent(work, segment);
                if (segment->buffer != NULL) {
                    p_prep->block_seq_no = (s32)current;
                    p_prep->data_ptr = segment->buffer;
                }
            }
        }
        break;
        }
        break;
    }
}

void WFS_CallServerConnectHook (WFSServerContext * context, const WFSPeerInfo * peer)
{
    (void)context;
    (void)peer;
}

void WFS_CallServerDisconnectHook (WFSServerContext * context, const WFSPeerInfo * peer)
{
    const int bit = (1 << peer->aid);
    context->all_bitmap &= ~bit;
    (void)WBT_CancelCommand(context->wbt, bit);
}

void WFS_CallServerPacketSendHook (WFSServerContext * context, WFSPacketBuffer * packet)
{
    if (!context->msg_busy) {
        context->ack_bitmap &= context->all_bitmap;
        context->sync_bitmap &= context->all_bitmap;
        context->busy_bitmap &= context->all_bitmap;
        context->deny_bitmap &= context->all_bitmap;

        if (context->is_changing && !context->use_bitmap) {
            context->is_changing = FALSE;
            (void)WBT_SetPacketLength(context->wbt, context->new_packet, WBT_PACKET_SIZE_MIN);

            if (context->deny_bitmap) {
                WFS_DEBUG_OUTPUT(("WBT-MSG(LOCK):deny [packet-length renewal] (BITMAP=%d)", context->deny_bitmap));
                (void)WFS_SendMessageLOCK_ACK(context->wbt, WFSi_WBTCallback, context->deny_bitmap, 0);
                context->msg_busy = TRUE;
                context->deny_bitmap = 0;
            }
        } else if (context->ack_bitmap)   {
            int bitmap = context->ack_bitmap;
            WFSMessageFormat * msg = NULL;
            int i;
            const int sync = context->sync_bitmap;
            const BOOL is_sync = (sync && ((bitmap & sync) == sync));

            if (is_sync) {
                bitmap = sync;
            } else {
                bitmap &= ~sync;
            }

            for (i = 0;; ++i) {
                const int bit = (1 << i);
                if (bit > bitmap) {
                    break;
                }
                if ((bit & bitmap) != 0) {

                    if (!msg) {
                        msg = &context->recv_msg[i];
                    } else if ((msg->type == context->recv_msg[i].type) &&
                               (msg->arg1 == context->recv_msg[i].arg1)) {
                    } else {
                        bitmap &= ~bit;
                    }
                }
            }

            if (is_sync && (bitmap != sync)) {
                context->sync_bitmap = 0;
                OS_TWarning("[WFS] specified synchronous-access failed! "
                            "(then synchronous-setting was reset)");
            }

            if (msg) {
                switch (msg->type) {
                case WFS_MSG_LOCK_REQ:
                    (void)WFS_SendMessageLOCK_ACK(context->wbt, WFSi_WBTCallback, bitmap,
                                                  MI_LEToH32(msg->arg1));
                    break;
                case WFS_MSG_UNLOCK_REQ:
                    (void)WFS_SendMessageUNLOCK_ACK(context->wbt, WFSi_WBTCallback, bitmap,
                                                    MI_LEToH32(msg->arg1));
                    context->busy_bitmap &= ~bitmap;
                    break;
                }
                context->msg_busy = TRUE;
                context->ack_bitmap &= ~bitmap;
            }
        }
    }

    packet->length = WBT_CallPacketSendHook(context->wbt, packet->buffer, packet->length, TRUE);
}

void WFS_CallServerPacketRecvHook (WFSServerContext * context, const WFSPacketBuffer * packet)
{
    int aid = (int)MATH_CTZ((u32)packet->bitmap);
    const void * buffer = packet->buffer;
    int length = packet->length;

    context->all_bitmap |= (1 << aid);
    WBT_CallPacketRecvHook(context->wbt, aid, buffer, length);
}

void WFS_InitServer (WFSServerContext * context, void * userdata, WFSEventCallback callback, MIAllocator * allocator, int packet)
{
    MI_CpuClear8(context, sizeof(*context));

    context->userdata = userdata;
    context->callback = callback;
    context->allocator = allocator;
    context->new_packet = packet;
    context->table->buffer = NULL;
    context->table->length = 0;
    context->sync_bitmap = 0;
    context->ack_bitmap = 0;
    context->msg_busy = FALSE;
    context->all_bitmap = 1;
    context->busy_bitmap = 0;
    context->is_changing = FALSE;
    context->deny_bitmap = 0;
    context->use_bitmap = 0;
    context->thread_work = NULL;
    context->thread_hook = NULL;

    WBT_InitContext(context->wbt, context, WFSi_WBTCallback);
    WBT_AddCommandPool(context->wbt, context->wbt_list, sizeof(context->wbt_list) / sizeof(*context->wbt_list));
    WBT_StartParent(context->wbt, packet, WBT_PACKET_SIZE_MIN);
}

void WFS_EndServer (WFSServerContext * context)
{
    if (context->thread_work) {
        (*context->thread_hook)(context->thread_work, NULL);
        MI_CallFree(context->allocator, context->thread_work);
        context->thread_work = NULL;
    }

    WBT_ResetContext(context->wbt, NULL);

    if (context->table->buffer) {
        MI_CallFree(context->allocator, context->table->buffer);
        context->table->buffer = NULL;
        context->table->length = 0;
    }
}

BOOL WFS_RegisterServerTable (WFSServerContext * context, MIDevice * device, u32 fatbase, u32 overlay)
{
    BOOL retval = FALSE;

    if (context->table->buffer) {
        OS_TWarning("table is already registered.\n");
    } else if (WFS_LoadTable(context->table, context->allocator, device, fatbase, overlay))   {

        WBT_RegisterBlockInfo(context->wbt, context->table_info,
                              WFS_TABLE_BLOCK_INDEX, NULL,
                              context->table->buffer,
                              (int)context->table->length);
        retval = TRUE;
    }

    return retval;
}

int WFS_GetServerPacketLength (const WFSServerContext * context)
{
    return context->new_packet;
}

void WFS_SetServerPacketLength (WFSServerContext * context, int length)
{
    SDK_ASSERT(length >= WBT_PACKET_SIZE_MIN);

    {
        PLATFORM_ENTER_CRITICALSECTION();
        context->new_packet = length;
        context->is_changing = TRUE;
        PLATFORM_LEAVE_CRITICALSECTION();
    }
}

void WFS_SetServerSync (WFSServerContext * context, int bitmap)
{
    PLATFORM_ENTER_CRITICALSECTION();
    context->sync_bitmap = (bitmap & ~1);
    PLATFORM_LEAVE_CRITICALSECTION();
}
