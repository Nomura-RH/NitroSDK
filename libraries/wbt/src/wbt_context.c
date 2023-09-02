#include <nitro.h>
#include <nitro/wbt.h>
#include <nitro/wbt/context.h>

#if defined(PRINTF_DEBUG)
    #define WBT_DEBUG_OUTPUT0       OS_TPrintf
#else
    #define WBT_DEBUG_OUTPUT0(...)  (void)0
#endif

#if defined(PRINTF_DEBUG_L1)
    #define WBT_DEBUG_OUTPUT1       OS_TPrintf
#else
    #define WBT_DEBUG_OUTPUT1(...)  (void)0
#endif

#if defined(PRINTF_DEBUG_L2)
    #define WBT_DEBUG_OUTPUT2       OS_TPrintf
#else
    #define WBT_DEBUG_OUTPUT2(...)  (void)0
#endif

#define WBT_CMD_0   WBT_CMD_REQ_NONE

#define WBT_COMMAND_REQ(type, pair_type, argument)                  \
    {                                                               \
        (u16) WBT_CMD_REQ_ ## type, (u16) WBT_CMD_ ## pair_type,    \
        sizeof(WBTPacketHeaderFormat) + argument,                   \
        TRUE, FALSE                                                 \
    }

#define WBT_COMMAND_RES(type, pair_type, argument)                  \
    {                                                               \
        (u16) WBT_CMD_RES_ ## type, (u16) WBT_CMD_ ## pair_type,    \
        sizeof(WBTPacketHeaderFormat) + argument,                   \
        FALSE, TRUE                                                 \
    }

#define WBT_COMMAND_MSG(type)                                       \
    {                                                               \
        (u16) WBT_CMD_ ## type, 0, 0, FALSE, FALSE                  \
    }

static const struct {
    u32 type : 8;
    u32 pair_type : 8;
    u32 packet;
    u32 is_req : 1;
    u32 is_res : 1;
} WBTi_CommandTable[] = {
    WBT_COMMAND_MSG(REQ_NONE),
    WBT_COMMAND_REQ(WAIT, 0, 0),
    WBT_COMMAND_REQ(SYNC, RES_SYNC, sizeof(WBTPacketRequestSyncFormat)),
    WBT_COMMAND_RES(SYNC, REQ_SYNC, sizeof(WBTPacketResponseSyncFormat)),
    WBT_COMMAND_REQ(GET_BLOCK, RES_GET_BLOCK, sizeof(WBTPacketRequestGetBlockFormat)),
    WBT_COMMAND_RES(GET_BLOCK, REQ_GET_BLOCK, sizeof(WBTPacketResponseGetBlockFormat)),
    WBT_COMMAND_REQ(GET_BLOCKINFO, RES_GET_BLOCKINFO, sizeof(WBTPacketRequestGetBlockFormat)),
    WBT_COMMAND_RES(GET_BLOCKINFO, REQ_GET_BLOCKINFO, sizeof(WBTPacketResponseGetBlockFormat)),
    WBT_COMMAND_REQ(GET_BLOCK_DONE, RES_GET_BLOCK_DONE, sizeof(WBTPacketRequestGetBlockDoneFormat)),
    WBT_COMMAND_RES(GET_BLOCK_DONE, REQ_GET_BLOCK_DONE, sizeof(WBTPacketResponseGetBlockDoneFormat)),
    WBT_COMMAND_REQ(USER_DATA, RES_USER_DATA, sizeof(WBTPacketRequestUserDataFormat)),
    WBT_COMMAND_RES(USER_DATA, REQ_USER_DATA, 0),
    WBT_COMMAND_MSG(SYSTEM_CALLBACK),
    WBT_COMMAND_MSG(PREPARE_SEND_DATA),
    WBT_COMMAND_MSG(REQ_ERROR),
    WBT_COMMAND_MSG(RES_ERROR),
    WBT_COMMAND_MSG(CANCEL),
};

enum {
    WBT_COMMAND_MAX = sizeof(WBTi_CommandTable) / sizeof(*WBTi_CommandTable)
};

#undef WBT_CMD_0
#undef WBT_COMMAND_REQ
#undef WBT_COMMAND_RES
#undef WBT_COMMAND_MSG

SDK_INLINE int div32 (int a)
{
    return (a >> 5);
}

SDK_INLINE int mod32 (int a)
{
    return (a & 0x1f);
}

PLATFORM_ATTRIBUTE_INLINE void WBTi_CopySafeMemory (const void * src, void * dst, u32 len)
{
    if (!src) {
        MI_CpuFill8(dst, 0x00, len);
    } else {
        MI_CpuCopy8(src, dst, len);
    }
}

PLATFORM_ATTRIBUTE_INLINE int WBTi_GetFirstIterationAID (WBTContext * context)
{
    const int mask = context->req_bitmap;

    if (((1 << context->last_target_aid) & mask) == 0) {
        context->last_target_aid = 31 - (int)MATH_CLZ((u32)mask);
    }

    return context->last_target_aid;
}

PLATFORM_ATTRIBUTE_INLINE int WBTi_GetNextIterationAID (int aid, int mask)
{
    ++aid;
    if ((1 << aid) > mask) {
        aid = (int)MATH_CTZ((u32)mask);
    }
    return aid;
}

static void WBTi_InitBitmap (WBTContext * work, WBTPacketBitmap * pkt_bmp, s32 length, u32 * bits, u8 * buffer, int packet)
{
    SDK_ASSERT(packet > 0);
    pkt_bmp->length = length;
    pkt_bmp->buffer = buffer;
    pkt_bmp->total = (length + packet - 1) / packet;
    pkt_bmp->count = 0;
    pkt_bmp->bitmap = bits;
    MI_CpuFill8(bits, 0x00, (u32)WBT_GetBitmapLength(work, length));
}

static BOOL WBTi_MergeBitmapIndex (WBTPacketBitmap * pkt_bmp, int index, u32 packet, const void * src)
{
    BOOL retval = FALSE;
    u32 pos = (u32)div32(index);
    u32 bit = (u32)mod32(index);

    if ((pkt_bmp->bitmap[pos] & (1 << bit)) == 0) {
        u8 * dst = pkt_bmp->buffer;
        const u32 offset = index * packet;
        const u32 total = (u32)pkt_bmp->length;
        u32 length = (u32)MATH_MIN(packet, total - offset);
        MI_CpuCopy8(src, dst + offset, length);
        pkt_bmp->bitmap[pos] |= (1 << bit);
        pkt_bmp->count += 1;
        retval = TRUE;
    }

    return retval;
}

static s32 WBTi_FindBitmapIndex (WBTPacketBitmap * pkt_bmp)
{
    int last_num;
    int bit_num;
    u32 * bmp;
    int num;

    num = pkt_bmp->current + 1;
    if (num >= pkt_bmp->total) {
        num = 0;
    }
    last_num = num;
    bmp = pkt_bmp->bitmap + div32(num);
    bit_num = mod32(num);

    for (;;) {
        if ((*bmp & (u32)((u32)1 << bit_num)) == 0) {
            break;
        } else {
            if (++num >= pkt_bmp->total) {
                num = 0;
                bit_num = 0;
                bmp = pkt_bmp->bitmap;
            } else if (++bit_num >= 32)   {
                bit_num = 0;
                ++bmp;
            }

            if (num == last_num) {
                num = -1;
                break;
            }
        }
    }
    return num;
}

static u8 * WBTi_GetPacketBuffer (WBTContext * work, u32 id, s32 index)
{
    u8 * ptr = NULL;
    WBTBlockInfoList * list = work->list;
    int count = 0;

    for (; list != NULL; list = list->next) {
        if (id < WBT_NUM_MAX_BLOCK_INFO_ID) {
            if (id == count) {
                static WBTBlockInfo tmp;
                tmp = list->data_info;
                tmp.id = MI_HToLE32(tmp.id);
                tmp.block_size = (s32)MI_HToLE32(tmp.block_size);
                ptr = (u8 *)&tmp;
                break;
            }
        } else {
            if (id == list->data_info.id) {
                ptr = (u8 *)list->data_ptr;
                break;
            }
        }
        ++count;
    }

    if (ptr) {
        ptr += index * work->my_data_packet_size;
    }
    return ptr;
}

static void WBTi_SwitchNextCommand (WBTContext * work)
{
    BOOL retval = FALSE;

    WBTCommand * current = WBT_GetCurrentCommand(work);

    if (current) {
        if (++work->my_command_counter == 0) {
            ++work->my_command_counter;
        }

        current->my_cmd_counter = work->my_command_counter;

        switch (current->command) {
        case WBT_CMD_REQ_GET_BLOCK:
        case WBT_CMD_REQ_GET_BLOCKINFO:
        {
            int aid;
            for (aid = 0; aid < 16; ++aid) {
                if ((current->target_bmp & (1 << aid)) != 0) {
                    WBTPacketBitmap * pkt_bmp = &work->peer_param[aid].pkt_bmp;
                    WBTi_InitBitmap(work, pkt_bmp, (int)current->get.recv_data_size,
                                    current->get.pkt_bmp_table.packet_bitmap[aid],
                                    current->get.recv_buf_table.recv_buf[aid],
                                    work->peer_data_packet_size);

                    pkt_bmp->current = 0;
                }
            }
        }
        break;
        case WBT_CMD_REQ_SYNC:
        case WBT_CMD_REQ_USER_DATA:
            break;
        default:
            current->command = WBT_CMD_REQ_NONE;
            break;

        }

        retval = (current->command != WBT_CMD_REQ_NONE);
    }
}

static void WBTi_NotifySystemCallback (WBTContext * work, WBTCommandType event, int aid, WBTResult result)
{
    BOOL retval = TRUE;
    WBTRecvToken * peer = &work->peer_param[aid].recv_token;

    if ((event != WBT_CMD_PREPARE_SEND_DATA) &&
        (peer->token_peer_cmd_counter == peer->last_peer_cmd_counter)) {
        retval = FALSE;
    } else if ((event == WBT_CMD_REQ_GET_BLOCK_DONE) &&
               (peer->token_block_id < WBT_NUM_MAX_BLOCK_INFO_ID)) {
        retval = FALSE;
    }

    if (retval) {
        WBTCommand * cmd = &work->system_cmd;
        peer->last_peer_cmd_counter = peer->token_peer_cmd_counter;
        cmd->peer_cmd_counter = peer->token_peer_cmd_counter;
        cmd->result = result;
        cmd->event = event;
        cmd->command = WBT_CMD_SYSTEM_CALLBACK;
        cmd->peer_bmp = (u16)(1 << aid);

        if (work->callback) {
            work->callback(work->userdata, cmd);
        } else if (cmd->callback)   {
            (*cmd->callback)(cmd);
        }
    }
}

PLATFORM_ATTRIBUTE_INLINE WBTPacketFormat * WBTi_MakeCommandHeader (void * dst, u8 cmd, int target, u8 counter)
{
    WBTPacketFormat * format = (WBTPacketFormat *) dst;

    MI_StoreLE8(&format->header.command, cmd);
    MI_StoreLE16(&format->header.bitmap, (u16)target);
    MI_StoreLE8(&format->header.counter, counter);

    return format;
}

static int WBTi_TryCreateResponse (WBTContext * work, int aid, void * buf, int size, int command, BOOL foroce_blockinfo)
{
    int ret_size = 0;
    WBTRecvToken * token = &work->peer_param[aid].recv_token;
    BOOL done = FALSE;
    (void)size;

    if (command == WBT_CMD_REQ_SYNC) {
        WBTPacketFormat * format = WBTi_MakeCommandHeader(buf, WBT_CMD_RES_SYNC, (1 << aid), token->token_peer_cmd_counter);

        MI_StoreLE16(&format->res_sync.block_total, (u16)WBT_GetRegisteredCount(work));
        MI_StoreLE16(&format->res_sync.peer_packet, (u16)work->my_data_packet_size);
        MI_StoreLE16(&format->res_sync.own_packet, (u16)work->peer_data_packet_size);

        ret_size = (int)sizeof(format->header) + sizeof(format->res_sync);
        done = TRUE;
    } else if (command == WBT_CMD_REQ_GET_BLOCK_DONE)   {
        u32 id = token->token_block_id;
        WBTPacketFormat * format = WBTi_MakeCommandHeader(buf, WBT_CMD_RES_GET_BLOCK_DONE, (1 << aid), token->token_peer_cmd_counter);

        MI_StoreLE32(&format->res_getblock_done.id, id);

        ret_size = (int)sizeof(format->header) + sizeof(format->res_getblock_done);
        done = TRUE;
        WBT_DEBUG_OUTPUT1("send BlockDone to %d id = %d\n", aid, id);
    } else if (command == WBT_CMD_REQ_USER_DATA)   {
        WBTPacketFormat * format = WBTi_MakeCommandHeader(buf, WBT_CMD_RES_USER_DATA, (1 << aid), token->token_peer_cmd_counter);
        ret_size = (int)sizeof(format->header);
        done = TRUE;
    } else if (command == WBT_CMD_REQ_GET_BLOCK)   {
        u32 id = token->token_block_id;
        WBTBlockInfoList * list = work->list;

        for (; list != NULL; list = list->next) {
            if (list->data_info.id == id) {
                break;
            }
        }

        if (list) {
            s32 index = token->token_block_seq_no;
            s32 block_seq_no;

            if (id >= WBT_NUM_MAX_BLOCK_INFO_ID) {
                if (id == work->last_block_id) {
                    const int index_total =
                        (list->data_info.block_size + work->my_data_packet_size -
                         1) / work->my_data_packet_size;
                    int i;

                    for (i = 0; (i < 3) && ((index == work->last_seq_no_1) ||
                                            (index == work->last_seq_no_2)); ++i) {
                        if (++index >= index_total) {
                            index = 0;
                        }
                    }
                }

                work->last_block_id = id;
                work->last_seq_no_2 = work->last_seq_no_1;
                work->last_seq_no_1 = index;
            }

            block_seq_no = index;

            {
                u32 packet = work->my_data_packet_size;
                u8 * data_ptr = NULL;
                BOOL sendable = FALSE;
                if (list->data_ptr) {
                    data_ptr = WBTi_GetPacketBuffer(work, id, block_seq_no);
                    WBT_DEBUG_OUTPUT1("send BlockData to %d id = %d seq no = %d pktsize %d\n", aid,
                                      id, index, packet);
                    sendable = TRUE;
                } else if (list->block_type == WBT_BLOCK_LIST_TYPE_USER)   {

                    WBTCommand * system_cmd = &work->system_cmd;
                    system_cmd->prepare_send_data.block_id = id;
                    system_cmd->prepare_send_data.block_seq_no = block_seq_no;
                    system_cmd->prepare_send_data.own_packet_size = (s16)packet;
                    system_cmd->prepare_send_data.data_ptr = NULL;
                    WBTi_NotifySystemCallback(work, WBT_CMD_PREPARE_SEND_DATA, aid,
                                              WBT_RESULT_SUCCESS);
                    WBT_DEBUG_OUTPUT1("peer req seq no  = %d seq no = %d dataptr = %p\n", index,
                                      block_seq_no, system_cmd->prepare_send_data.data_ptr);
                    if (system_cmd->prepare_send_data.data_ptr != NULL) {
                        id = system_cmd->prepare_send_data.block_id;
                        block_seq_no = system_cmd->prepare_send_data.block_seq_no;
                        data_ptr = system_cmd->prepare_send_data.data_ptr;
                        packet = system_cmd->prepare_send_data.own_packet_size;
                        sendable = TRUE;
                    }
                }

                if (sendable) {
                    u16 target = (u16)((WBT_GetAid(work) == WBT_AID_PARENT) ? 0xFFFE : 0x0001);
                    WBTPacketFormat * format = WBTi_MakeCommandHeader(buf, WBT_CMD_RES_GET_BLOCK, target, token->token_peer_cmd_counter);

                    MI_StoreLE32(&format->res_getblock.id, id);
                    MI_StoreLE32(&format->res_getblock.index, (u32)block_seq_no);
                    WBTi_CopySafeMemory(data_ptr, &format->res_getblock + 1, (u32)packet);

                    ret_size = (int)(sizeof(format->header) + sizeof(format->res_getblock) + packet);
                    done = TRUE;
                }
            }
        }
    } else if (command == WBT_CMD_REQ_GET_BLOCKINFO)   {
        u32 id = token->token_block_id;
        s32 index = token->token_block_seq_no;
        int packet = work->my_data_packet_size;
        u8 * data_ptr = WBTi_GetPacketBuffer(work, id, index);
        WBT_DEBUG_OUTPUT1("send BlockData to %d id = %d seq no = %d pktsize %d\n", aid, id, index,
                          packet);

        if (foroce_blockinfo || data_ptr) {
            u16 target = (u16)((WBT_GetAid(work) == 0) ? 0xFFFF : 0x0001);
            WBTPacketFormat * format = WBTi_MakeCommandHeader(buf, WBT_CMD_RES_GET_BLOCKINFO, target, token->token_peer_cmd_counter);
            MI_StoreLE32(&format->res_getblock.id, id);
            MI_StoreLE32(&format->res_getblock.index, (u32)index);
            WBTi_CopySafeMemory(data_ptr, &format->res_getblock + 1, (u32)packet);
            ret_size = (int)(sizeof(format->header) + sizeof(format->res_getblock) + packet);
            done = TRUE;
        }
    }

    if (done) {
        work->req_bitmap &= ~(1 << aid);
    }

    return ret_size;
}

static int WBTi_CheckRequest (WBTContext * work, void * send_buf, int size)
{
    int send_size = 0;
    WBTCommand * current = WBT_GetCurrentCommand(work);
    (void)size;

    if (current) {
        switch (current->command) {
        case WBT_CMD_REQ_SYNC:
        {
            WBTPacketFormat * format = WBTi_MakeCommandHeader(send_buf, WBT_CMD_REQ_SYNC, current->target_bmp, current->my_cmd_counter);
            MI_StoreLE16(&format->req_sync.peer_packet, (u16)work->my_data_packet_size);
            MI_StoreLE16(&format->req_sync.own_packet, (u16)work->peer_data_packet_size);

            send_size = (int)sizeof(format->header) + sizeof(format->req_sync);
            WBT_DEBUG_OUTPUT0("send ReqSync to 0x%04x cmd counter %d\n", current->target_bmp, current->my_cmd_counter);
        }
        break;
        case WBT_CMD_REQ_GET_BLOCK:
        case WBT_CMD_REQ_GET_BLOCKINFO:
        {
            int aid;
            for (aid = 0; aid < 16; ++aid) {

                if ((current->target_bmp & (1 << aid)) != 0) {
                    WBTPacketBitmap * pkt_bmp = &work->peer_param[aid].pkt_bmp;
                    s32 next_seq_no = WBTi_FindBitmapIndex(pkt_bmp);

                    if (next_seq_no == -1) {
                        WBTPacketFormat * format = WBTi_MakeCommandHeader(send_buf, WBT_CMD_REQ_GET_BLOCK_DONE, (1 << aid), current->my_cmd_counter);
                        MI_StoreLE32(&format->req_getblock_done.id, current->get.block_id);
                        send_size = (int)sizeof(format->header) + sizeof(format->req_getblock_done);
                        WBT_DEBUG_OUTPUT0("send ReqBlockDone to %d 0x%04x\n", aid, (1 << aid));
                    } else {
                        WBTPacketFormat * format =
                            WBTi_MakeCommandHeader(send_buf, current->command,
                                                   current->target_bmp,
                                                   current->my_cmd_counter);
                        MI_StoreLE32(&format->req_getblock.id, current->get.block_id);
                        MI_StoreLE32(&format->req_getblock.index, (u32)next_seq_no);
                        send_size = (int)sizeof(format->header) +
                                    sizeof(format->req_getblock);
                        WBT_DEBUG_OUTPUT1("send ReqBlock id=%d seq=%d\n", current->get.block_id,
                                          next_seq_no);
                    }
                    if (send_size) {
                        break;
                    }
                }
            }
        }
        break;
        case WBT_CMD_REQ_USER_DATA:
        {
            WBTPacketFormat * format = WBTi_MakeCommandHeader(send_buf, WBT_CMD_REQ_USER_DATA, current->target_bmp, current->my_cmd_counter);

            MI_StoreLE8(&format->req_userdata.length, current->user_data.size);
            MI_CpuCopy8(current->user_data.data, &format->req_userdata.buffer, WBT_SIZE_USER_DATA);

            send_size = (int)sizeof(format->header) + sizeof(format->req_userdata);
        }
        break;
        default:
            WBT_DEBUG_OUTPUT0("Unknown User Command:Error %s %s %d\n", __FILE__, __FUNCTION__,
                              __LINE__);
            break;
        }
    }
    return send_size;
}

static int WBTi_CheckBlockResponse (WBTContext * work, void * buffer, int length)
{
    int retval = 0;

    int mask = work->req_bitmap;
    if (!retval && (mask != 0)) {
        int aid = WBTi_GetFirstIterationAID(work);
        do {
            aid = WBTi_GetNextIterationAID(aid, mask);
            if ((work->req_bitmap & (1 << aid)) != 0) {
                WBTRecvToken * token = &work->peer_param[aid].recv_token;
                if (WBT_CMD_REQ_GET_BLOCK == token->token_command) {
                    retval = WBTi_TryCreateResponse(work, aid, buffer, length, WBT_CMD_REQ_GET_BLOCK, FALSE);
                }
                if (retval) {
                    work->last_target_aid = aid;
                }
            }
        } while (aid != work->last_target_aid);
    }

    if (!retval && (mask != 0)) {
        int aid = WBTi_GetFirstIterationAID(work);
        do {
            aid = WBTi_GetNextIterationAID(aid, mask);
            if ((work->req_bitmap & (1 << aid)) != 0) {
                WBTRecvToken * token = &work->peer_param[aid].recv_token;
                if (WBT_CMD_REQ_GET_BLOCKINFO == token->token_command) {
                    retval = WBTi_TryCreateResponse(work, aid, buffer, length, WBT_CMD_REQ_GET_BLOCKINFO, TRUE);
                }
                if (retval) {
                    work->last_target_aid = aid;
                }
            }
        } while (aid != work->last_target_aid);
    }

    return retval;
}

int WBT_CallPacketSendHook (WBTContext * work, void * buffer, int length, BOOL is_parent)
{
    int retval = 0;

    if (work->last_target_aid == -1) {
        work->last_target_aid = (is_parent ? 1 : 0);
    }

    if (!buffer || (length <= 0)) {
        return 0;
    }

    {
        static const WBTCommandType tbl[] = {
            WBT_CMD_REQ_USER_DATA,
            WBT_CMD_REQ_SYNC,
            WBT_CMD_REQ_GET_BLOCKINFO,
            WBT_CMD_REQ_GET_BLOCK_DONE,
            WBT_CMD_REQ_NONE,
        };

        int mask = work->req_bitmap;
        if (mask != 0) {
            int aid = WBTi_GetFirstIterationAID(work);
            do {
                aid = WBTi_GetNextIterationAID(aid, mask);
                if ((mask & (1 << aid)) != 0) {

                    WBTRecvToken * token = &work->peer_param[aid].recv_token;
                    int i;
                    for (i = 0; !retval && (tbl[i] != WBT_CMD_REQ_NONE); ++i) {

                        if (tbl[i] == token->token_command) {
                            retval =
                                WBTi_TryCreateResponse(work, aid, buffer, length, tbl[i], FALSE);
                        }
                    }
                    if (retval) {
                        work->last_target_aid = aid;
                    }
                }
            } while (aid != work->last_target_aid);
        }
    }

    if (!retval) {
        if (is_parent) {
            retval = WBTi_CheckRequest(work, buffer, length);
            if (!retval) {
                retval = WBTi_CheckBlockResponse(work, buffer, length);
            }
        } else {
            retval = WBTi_CheckBlockResponse(work, buffer, length);
            if (!retval) {
                retval = WBTi_CheckRequest(work, buffer, length);
            }
        }
        if (!retval) {
            int mask = (is_parent ? 0xFFFE : 0x0001);
            WBTPacketFormat * format = WBTi_MakeCommandHeader(buffer, WBT_CMD_REQ_WAIT, (u16)mask, (WBTCommandCounter)0);
            retval = (int)sizeof(format->header);
        }
    }

    return retval;
}

static void WBTi_NotifyCompletionCallback (WBTContext * work, WBTCommandType event, int aid)
{
    WBTCommandList * list = WBT_GetCurrentCommandList(work);
    WBTCommand * current = WBT_GetCurrentCommand(work);
    WBTRecvToken * token = &work->peer_param[aid].recv_token;

    if (current->my_cmd_counter == token->token_peer_cmd_counter) {
        int peer_bmp = (1 << aid);
        current->peer_cmd_counter = token->token_peer_cmd_counter;
        current->peer_bmp = (u16)peer_bmp;

        if ((current->target_bmp & peer_bmp) != 0) {
            current->target_bmp &= ~peer_bmp;
            current->event = event;
            current->result = WBT_RESULT_SUCCESS;

            if (list->callback) {
                list->callback(work->userdata, current);
            } else if (current->callback)   {
                current->callback(current);
            }
        }

        if (current->target_bmp == 0) {
            WBTCommandList * list = work->command;
            work->command = list->next;
            WBT_AddCommandPool(work, list, 1);
            WBTi_SwitchNextCommand(work);
        }
    }
}

void WBT_CallPacketRecvHook (WBTContext * work, int aid, const void * buffer, int length)
{
    WBTRecvToken * token = &work->peer_param[aid].recv_token;

    work->req_bitmap &= ~(1 << aid);

    if (buffer && (length >= sizeof(WBTPacketHeaderFormat))) {
        const WBTPacketFormat * format = (const WBTPacketFormat *)buffer;
        u8 command;
        u16 bitmap;

        command = MI_LoadLE8(&format->header.command);
        bitmap = MI_LoadLE16(&format->header.bitmap);

        token->token_peer_cmd_counter = MI_LoadLE8(&format->header.counter);
        token->token_command = command;

        if ((WBT_GetAid(work) != -1) && ((bitmap & (1 << WBT_GetAid(work))) != 0)) {

            if (command >= WBT_COMMAND_MAX) {
            } else if (length < WBTi_CommandTable[command].packet)   {
            } else if (WBTi_CommandTable[command].is_req)   {
                if (command == WBT_CMD_REQ_WAIT) {
                } else if (command == WBT_CMD_REQ_SYNC)   {
                    WBTRequestSyncCallback * cb = &work->system_cmd.sync;
                    cb->peer_packet_size = (s16)MI_LoadLE16(&format->req_sync.peer_packet);
                    cb->my_packet_size = (s16)MI_LoadLE16(&format->req_sync.own_packet);
                    cb->num_of_list = 0;

                    if (WBT_GetAid(work) != 0) {
                        work->my_data_packet_size = cb->my_packet_size;
                        work->peer_data_packet_size = cb->peer_packet_size;
                    }
                    work->req_bitmap |= (1 << aid);
                } else if (command == WBT_CMD_REQ_USER_DATA)   {
                    WBTRecvUserDataCallback * cb = &work->system_cmd.user_data;
                    cb->size = MI_LoadLE8(&format->req_userdata.length);
                    if (cb->size > WBT_SIZE_USER_DATA) {
                        cb->size = 0;
                    }
                    MI_CpuCopy8(format->req_userdata.buffer, cb->data, cb->size);
                    work->req_bitmap |= (1 << aid);
                } else if ((command == WBT_CMD_REQ_GET_BLOCK) ||
                           (command == WBT_CMD_REQ_GET_BLOCKINFO)) {
                    token->token_block_id = MI_LoadLE32(&format->req_getblock.id);
                    token->token_block_seq_no = (s32)MI_LoadLE32(&format->req_getblock.index);
                    work->req_bitmap |= (1 << aid);
                    WBT_DEBUG_OUTPUT1("get req Block from %d id = %d seq no = %d\n", aid, token->token_block_id,
                                      token->token_block_seq_no);
                } else if (command == WBT_CMD_REQ_GET_BLOCK_DONE)   {
                    WBTGetBlockDoneCallback * cb = &work->system_cmd.blockdone;
                    cb->block_id = MI_LoadLE32(&format->req_getblock_done.id);

                    token->token_block_id = MI_LoadLE32(&format->req_getblock_done.id);
                    work->req_bitmap |= (1 << aid);
                }
            } else if (WBTi_CommandTable[command].is_res)   {
                WBTCommand * current = WBT_GetCurrentCommand(work);

                if (!current) {
                } else {
                    if (command == WBT_CMD_RES_SYNC) {
                        if (current->command == WBT_CMD_REQ_SYNC) {
                            current->sync.num_of_list =
                                (s16)MI_LoadLE16(&format->res_sync.block_total);
                            current->sync.peer_packet_size =
                                (s16)MI_LoadLE16(&format->res_sync.peer_packet);
                            current->sync.my_packet_size =
                                (s16)MI_LoadLE16(&format->res_sync.own_packet);

                            if (WBT_GetAid(work) != 0) {
                                work->my_data_packet_size = current->sync.my_packet_size;
                                work->peer_data_packet_size = current->sync.peer_packet_size;
                            }
                            WBT_DEBUG_OUTPUT0("Get res Sync from %d my %d peer %d\n", aid,
                                              current->sync.my_packet_size,
                                              current->sync.peer_packet_size);
                            WBTi_NotifyCompletionCallback(work, (WBTCommandType)command, aid);
                        }
                    } else if (command == WBT_CMD_RES_USER_DATA)   {
                        if (current->command == WBT_CMD_REQ_USER_DATA) {
                            WBTi_NotifyCompletionCallback(work, (WBTCommandType)command, aid);
                        }
                    } else if ((command == WBT_CMD_RES_GET_BLOCK) ||
                               (command == WBT_CMD_RES_GET_BLOCKINFO)) {
                        if ((current->command == WBT_CMD_REQ_GET_BLOCK) ||
                            (current->command == WBT_CMD_REQ_GET_BLOCKINFO)) {
                            u32 id = MI_LoadLE32(&format->res_getblock.id);
                            s32 index = (s32)MI_LoadLE32(&format->res_getblock.index);

                            if (id == current->get.block_id) {
                                if ((current->target_bmp & (1 << aid)) != 0) {
                                    WBTPacketBitmap * pkt_bmp = &work->peer_param[aid].pkt_bmp;
                                    if (index >= pkt_bmp->total) {
                                        WBT_DEBUG_OUTPUT1
                                            ("%s num of seq over seq no = %d total = %d\n",
                                            __FUNCTION__, index, pkt_bmp->total);

                                        WBTi_NotifySystemCallback(work, WBT_CMD_RES_ERROR, aid,
                                                                  WBT_RESULT_ERROR_SAVE_FAILURE);
                                    } else {
                                        const void * src = (const u8 *)format +
                                                           sizeof(format->header) + sizeof(format->res_getblock);
                                        u32 packet = (u32)work->peer_data_packet_size;
                                        if (WBTi_MergeBitmapIndex(pkt_bmp, index, packet, src)) {
                                            pkt_bmp->current = index;
                                        }
                                    }
                                }
                            }
                        }
                    } else if (command == WBT_CMD_RES_GET_BLOCK_DONE)   {

                        if ((current->command == WBT_CMD_REQ_GET_BLOCK) ||
                            (current->command == WBT_CMD_REQ_GET_BLOCKINFO)) {
                            u32 id = MI_LoadLE32(&format->res_getblock_done.id);
                            if (current->get.block_id == id) {
                                WBT_DEBUG_OUTPUT1
                                    ("get block my cmd counter = %d peer cmd counter = %d\n",
                                    current->my_cmd_counter, token->token_peer_cmd_counter);
                                WBTi_NotifyCompletionCallback(work,
                                                              (current->command ==
                                                               WBT_CMD_REQ_GET_BLOCK) ?
                                                              WBT_CMD_RES_GET_BLOCK :
                                                              WBT_CMD_RES_GET_BLOCKINFO, aid);
                            }
                            WBT_DEBUG_OUTPUT0("c usr cmd tbmp 0x%x\n", current->target_bmp);
                        }
                    }
                }
            } else {
                WBTi_NotifySystemCallback(work, WBT_CMD_RES_ERROR, aid,
                                          WBT_RESULT_ERROR_UNKNOWN_PACKET_COMMAND);
            }

        }

        if ((work->req_bitmap & (1 << aid)) != 0) {
            if ((command != WBT_CMD_REQ_GET_BLOCK) && (command != WBT_CMD_REQ_GET_BLOCKINFO)) {
                WBTi_NotifySystemCallback(work, (WBTCommandType)command, aid, WBT_RESULT_SUCCESS);
            }
        }

    }
}

void WBT_InitContext (WBTContext * work, void * userdata, WBTEventCallback callback)
{
    work->userdata = userdata;
    work->callback = callback;

    work->last_block_id = (u32) - 1;
    work->last_seq_no_1 = -1;
    work->last_seq_no_2 = -1;

    work->command = NULL;
    work->command_pool = NULL;
    work->my_command_counter = 0;
    work->last_target_aid = -1;
    work->req_bitmap = 0;

    MI_CpuFill8(&work->system_cmd, 0x00, sizeof(work->system_cmd));
    MI_CpuFill8(work->peer_param, 0x00, sizeof(work->peer_param));

    WBT_ResetContext(work, callback);
}

void WBT_ResetContext (WBTContext * work, WBTEventCallback callback)
{
    int i;

    work->my_aid = -1;
    work->peer_data_packet_size = 0;
    work->my_data_packet_size = 0;

    work->list = NULL;
    work->callback = callback;

    while (work->command) {
        WBTCommandList * list = work->command;
        work->command = list->next;
        list->command.command = WBT_CMD_REQ_NONE;
    }

    work->system_cmd.command = WBT_CMD_REQ_NONE;
    work->system_cmd.target_bmp = 0;
    work->system_cmd.peer_bmp = 0;

    for (i = 0; i < 16; ++i) {
        work->peer_param[i].recv_token.last_peer_cmd_counter = 0;
    }

}

void WBT_PostCommand (WBTContext * work, WBTCommandList * list, u16 bitmap, WBTEventCallback callback)
{
    PLATFORM_ENTER_CRITICALSECTION();

    {
        if (list) {

            WBTCommandList ** pp;
            for (pp = &work->command; *pp; pp = &(*pp)->next) {
            }
            *pp = list;
            list->next = NULL;
            list->command.target_bmp = bitmap;
            list->callback = callback;

            if (work->command == list) {
                WBTi_SwitchNextCommand(work);
            }
        }
    }

    PLATFORM_LEAVE_CRITICALSECTION();
}

int WBT_CancelCommand (WBTContext * work, int bitmap)
{
    PLATFORM_ENTER_CRITICALSECTION();

    {
        WBTCommandList * list = WBT_GetCurrentCommandList(work);
        WBTCommand * current = WBT_GetCurrentCommand(work);
        if (current) {
            int aid;

            bitmap &= current->target_bmp;
            for (aid = 0;; ++aid) {
                int bit = (1 << aid);
                if (bit > bitmap) {
                    break;
                }
                if ((bit & bitmap) == 0) {
                    bitmap &= ~bit;
                } else if (list->callback)   {
                    current->event = WBT_CMD_CANCEL;
                    current->target_bmp &= ~bit;
                    current->peer_bmp = (u16)bit;
                    list->callback(work->userdata, current);
                } else if (current->callback)   {
                    current->event = WBT_CMD_CANCEL;
                    current->target_bmp &= ~bit;
                    current->peer_bmp = (u16)bit;
                    (*current->callback)(current);
                }
            }

            if (current->target_bmp == 0) {
                WBTCommandList * list = work->command;
                work->command = list->next;
                WBT_AddCommandPool(work, list, 1);
                WBTi_SwitchNextCommand(work);
            }
        }
    }

    PLATFORM_LEAVE_CRITICALSECTION();
    return bitmap;
}

int WBT_GetBitmapLength (const WBTContext * work, int length)
{
    int packet = work->peer_data_packet_size;
    SDK_ASSERT(packet > 0);
    return (int)(sizeof(u32) * MATH_ROUNDUP(((length + packet - 1) / packet), 32));
}

void WBT_GetDownloadProgress (const WBTContext * work, u32 id, int aid, int * count, int * total)
{
    const WBTCommand * current = WBT_GetCurrentCommand(work);
    if ((current != NULL) &&
        (current->command == WBT_CMD_REQ_GET_BLOCK) && (current->get.block_id == id)) {
        const WBTPacketBitmap * pkt_bmp = &work->peer_param[aid].pkt_bmp;
        *count = pkt_bmp->count;
        *total = pkt_bmp->total;
    } else {
        *count = 0;
        *total = 0;
    }
}

BOOL WBT_SetPacketLength (WBTContext * work, int own, int peer)
{
    BOOL retval = FALSE;
    WBTCommand * current;

    SDK_ASSERT(own >= WBT_PACKET_SIZE_MIN);
    SDK_ASSERT(peer >= WBT_PACKET_SIZE_MIN);

    current = WBT_GetCurrentCommand(work);

    if ((current == NULL) ||
        ((current->command != WBT_CMD_REQ_GET_BLOCK) &&
         (current->command != WBT_CMD_REQ_GET_BLOCKINFO))) {
        work->my_data_packet_size = (s16)(own - WBT_PACKET_SIZE_MIN);
        work->peer_data_packet_size = (s16)(peer - WBT_PACKET_SIZE_MIN);
    }

    return retval;
}

void WBT_RegisterBlockInfo (WBTContext * work, WBTBlockInfoList * list, u32 id, const void * userinfo, const void * buffer, int length)
{
    PLATFORM_ENTER_CRITICALSECTION();

    {
        WBTBlockInfoList ** pp;
        for (pp = &work->list; *pp; pp = &((*pp)->next)) {
        }
        *pp = list;
        list->next = NULL;
        list->data_info.id = id;
        list->data_info.block_size = length;
        WBTi_CopySafeMemory(userinfo, list->data_info.user_id, WBT_USER_ID_LEN);
        list->data_ptr = (void *)buffer;

        list->permission_bmp = 0;
        list->block_type = (u16)(buffer ? WBT_BLOCK_LIST_TYPE_COMMON : WBT_BLOCK_LIST_TYPE_USER);
    }

    PLATFORM_LEAVE_CRITICALSECTION();
}

WBTBlockInfoList * WBT_UnregisterBlockInfo (WBTContext * work, u32 id)
{
    WBTBlockInfoList * retval = NULL;
    {
        PLATFORM_ENTER_CRITICALSECTION();
        WBTBlockInfoList ** pp;
        for (pp = &work->list; *pp; pp = &(*pp)->next) {
            if ((*pp)->data_info.id == id) {
                retval = *pp;
                *pp = (*pp)->next;
                break;
            }
        }
        PLATFORM_LEAVE_CRITICALSECTION();
    }

    return retval;
}

int WBT_GetRegisteredCount (const WBTContext * work)
{
    int n = 0;
    {
        PLATFORM_ENTER_CRITICALSECTION();
        WBTBlockInfoList * list = work->list;
        for (list = work->list; list; list = list->next) {
            ++n;
        }
        PLATFORM_LEAVE_CRITICALSECTION();
    }

    return n;
}
