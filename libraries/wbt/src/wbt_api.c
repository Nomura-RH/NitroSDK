#include <nitro.h>
#include <nitro/wbt.h>
#include <nitro/wbt/context.h>

static BOOL wbt_initialize_flag = FALSE;
static WBTContext wbti_command_work[1];
static WBTCommandList cmd_q[2];

void WBT_PrintBTList (void)
{
    WBTBlockInfoList * list = wbti_command_work->list;

    for (; list != NULL; list = list->next) {
        OS_Printf("BTList id = %d\n", list->data_info.id);
        OS_Printf("  data size %d\n", list->data_info.block_size);
        OS_Printf("  uid %s\n", list->data_info.user_id);
        OS_Printf("  info ptr = %p\n", &(list->data_info));
    }
}

int WBT_AidbitmapToAid (WBTAidBitmap abmp)
{
    return abmp ? (int)MATH_CTZ(abmp) : -1;
}

void WBT_InitParent (int send_packet_size, int recv_packet_size, WBTCallback callback)
{
    PLATFORM_ENTER_CRITICALSECTION();

    if (!wbt_initialize_flag) {
        wbt_initialize_flag = TRUE;
        WBT_InitContext(wbti_command_work, NULL, NULL);
        wbti_command_work->system_cmd.callback = callback;

        MI_CpuFill8(cmd_q, 0x00, sizeof(cmd_q));
        WBT_AddCommandPool(wbti_command_work, cmd_q, sizeof(cmd_q) / sizeof(*cmd_q));
        WBT_StartParent(wbti_command_work, send_packet_size, recv_packet_size);
    }

    PLATFORM_LEAVE_CRITICALSECTION();
}

void WBT_InitChild (WBTCallback callback)
{
    PLATFORM_ENTER_CRITICALSECTION();

    if (!wbt_initialize_flag) {
        wbt_initialize_flag = TRUE;
        WBT_InitContext(wbti_command_work, NULL, NULL);
        wbti_command_work->system_cmd.callback = callback;

        MI_CpuFill8(cmd_q, 0x00, sizeof(cmd_q));
        WBT_AddCommandPool(wbti_command_work, cmd_q, sizeof(cmd_q) / sizeof(*cmd_q));
    }

    PLATFORM_LEAVE_CRITICALSECTION();
}

void WBT_End (void)
{
    PLATFORM_ENTER_CRITICALSECTION();

    if (wbt_initialize_flag) {
        wbt_initialize_flag = FALSE;
        wbti_command_work->system_cmd.callback = NULL;
        WBT_ResetContext(wbti_command_work, NULL);
    }

    PLATFORM_LEAVE_CRITICALSECTION();
}

void WBT_SetOwnAid (int aid)
{
    WBTContext * const work = wbti_command_work;

    if (WBT_GetAid(work) == -1) {
        WBT_StartChild(work, aid);
    }
}

int WBT_GetOwnAid (void)
{
    const WBTContext * const work = wbti_command_work;
    return WBT_GetAid(work);
}

int WBT_CalcPacketbitmapSize (int block_size)
{
    return WBT_GetBitmapLength(wbti_command_work, block_size);
}

BOOL WBT_GetCurrentDownloadProgress (u32 block_id, int aid, int * current_count, int * total_count)
{
    WBT_GetDownloadProgress(wbti_command_work, block_id, aid, current_count, total_count);
    return (*total_count != 0);
}

BOOL WBT_SetPacketSize (int send_packet_size, int recv_packet_size)
{
    return WBT_SetPacketLength(wbti_command_work, send_packet_size, recv_packet_size);
}

int WBT_NumOfRegisteredBlock (void)
{
    return WBT_GetRegisteredCount(wbti_command_work);
}

int WBT_MpParentSendHook (void * sendbuf, int send_size)
{
    SDK_ASSERT(wbt_initialize_flag);
    return WBT_CallPacketSendHook(wbti_command_work, sendbuf, send_size, TRUE);
}

int WBT_MpChildSendHook (void * sendbuf, int send_size)
{
    SDK_ASSERT(wbt_initialize_flag);
    return WBT_CallPacketSendHook(wbti_command_work, sendbuf, send_size, FALSE);
}

void WBT_MpParentRecvHook (const void * buf, int size, int aid)
{
    SDK_ASSERT(wbt_initialize_flag);
    WBT_CallPacketRecvHook(wbti_command_work, aid, buf, size);
}

void WBT_MpChildRecvHook (const void * buf, int size)
{
    SDK_ASSERT(wbt_initialize_flag);
    WBT_CallPacketRecvHook(wbti_command_work, 0, buf, size);
}

BOOL WBT_RegisterBlock (WBTBlockInfoList * block_info_list, WBTBlockId block_id, const void * user_id, const void * data_ptr, int data_size, WBTAidBitmap permission_bmp)
{
    (void)permission_bmp;
    SDK_ASSERT(wbt_initialize_flag);
    WBT_RegisterBlockInfo(wbti_command_work, block_info_list, block_id, user_id, data_ptr, data_size);
    return TRUE;
}

WBTBlockInfoList * WBT_UnregisterBlock (WBTBlockId block_id)
{
    SDK_ASSERT(wbt_initialize_flag);
    return WBT_UnregisterBlockInfo(wbti_command_work, block_id);
}

BOOL WBT_RequestSync (WBTAidBitmap target, WBTCallback callback)
{
    WBTCommandList * list = WBT_AllocCommandList(wbti_command_work);

    if (list) {
        list->command.callback = callback;
        WBT_CreateCommandSYNC(wbti_command_work, list);
        WBT_PostCommand(wbti_command_work, list, target, NULL);
    }

    return (list != NULL);
}

BOOL WBT_GetBlockInfo (WBTAidBitmap target, int block_info_no, WBTBlockInfoTable * block_info_table, WBTCallback callback)
{
    WBTCommandList * list = WBT_AllocCommandList(wbti_command_work);

    if (list) {
        list->command.callback = callback;
        WBT_CreateCommandINFO(wbti_command_work, list, block_info_no, block_info_table);
        WBT_PostCommand(wbti_command_work, list, target, NULL);
    }

    return (list != NULL);
}

BOOL WBT_GetBlock (WBTAidBitmap target, WBTBlockId block_id, WBTRecvBufTable * recv_buf_table, u32 recv_size, WBTPacketBitmapTable * p_bmp_table, WBTCallback callback)
{
    WBTCommandList * list = WBT_AllocCommandList(wbti_command_work);

    if (list) {
        list->command.callback = callback;
        WBT_CreateCommandGET(wbti_command_work, list, block_id, recv_size, recv_buf_table, p_bmp_table);
        WBT_PostCommand(wbti_command_work, list, target, NULL);
    }

    return (list != NULL);
}

BOOL WBT_PutUserData (WBTAidBitmap target, const void * user_data, int size, WBTCallback callback)
{
    WBTCommandList * list = WBT_AllocCommandList(wbti_command_work);

    if (list) {
        list->command.callback = callback;
        WBT_CreateCommandMSG(wbti_command_work, list, user_data, (u32)size);
        WBT_PostCommand(wbti_command_work, list, target, NULL);
    }

    return (list != NULL);
}

BOOL WBT_CancelCurrentCommand (WBTAidBitmap cancel_target)
{
    SDK_ASSERT(wbt_initialize_flag);
    return (WBT_CancelCommand(wbti_command_work, cancel_target) != 0);
}
