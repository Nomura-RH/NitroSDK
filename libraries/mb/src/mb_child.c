#include "mb_private.h"

#define MY_ROUND(n, a)    (((u32) (n) + (a) - 1) & ~((a) - 1))

static int MBi_CommRequestFile(u8 file_no, u32 ggid);
static void MBi_CommChangeChildState(int state, void * arg);
static void MBi_CommChangeChildStateCallbackOnly(int state, void * arg);
static void MBi_CommChildRecvData(void * arg);
static int MBi_CommChildSendData(void);
static void MBi_CommBeaconRecvCallback(MbBeaconMsg msg, MBGameInfoRecvList * gInfop, int index);

static BOOL checkRecvFlag(int x);
static void setRecvFlag(int x);
static u16 countRemainingblocks(void);
static u16 get_next_blockno(void);
static BOOL checkPollbmp(u16 pollbmp, u16 child_aid);
static void clearRecvRegion(void);
static void MBi_CommCallChildError(u16 errcode);

int MB_CommDownloadRequest (int index)
{
	const MbBeaconRecvStatus * mbrsp;
	const WMBssDesc * bssDescp;
	u8 fileNo;
	int ret;
	OSIntrMode enabled = OS_DisableInterrupts();

	mbrsp = MB_GetBeaconRecvStatus();
	bssDescp = &(mbrsp->list[index].bssDesc);
	fileNo = mbrsp->list[index].gameInfo.fileNo;

	pCwork->connectTargetNo = index;
	pCwork->fileid = fileNo;
	pCwork->last_recv_seq_no = -1;

	MI_CpuCopy16(bssDescp, &pCwork->bssDescbuf, WM_BSS_DESC_SIZE);

	ret = MBi_CommConnectToParent((const WMBssDesc *)&pCwork->bssDescbuf);
	(void)OS_RestoreInterrupts(enabled);

	return ret;
}

static int MBi_CommRequestFile (u8 file_no, u32 ggid)
{
	int errcode;
	MBCommChildBlockHeader hd;
	u8 * databuf;
	MBCommRequestData req_data;

	req_data.ggid = ggid;
	MI_CpuCopy8(&pCwork->common.user, &req_data.userinfo, sizeof(MBUserInfo));
	req_data.version = MB_IPL_VERSION;
	req_data.fileid = file_no;

	hd.type = MB_COMM_TYPE_CHILD_FILEREQ;
	hd.req_data.piece = MBi_SendRequestDataPiece(hd.req_data.data, &req_data);

	databuf = MBi_MakeChildSendBuffer(&hd, (u8 *)pCwork->common.sendbuf);

	if (!databuf) {
		return MB_SENDFUNC_STATE_ERR;
	}

	errcode = MBi_BlockHeaderEnd(MB_COMM_CHILD_HEADER_SIZE, 0xFFFF, pCwork->common.sendbuf);
	return errcode;
}

void MB_CommSetChildStateCallback (MBCommCStateCallbackFunc callback)
{
	OSIntrMode enabled;

	SDK_ASSERT(pCwork != 0);

	enabled = OS_DisableInterrupts();
	pCwork->child_callback = callback;

	(void)OS_RestoreInterrupts(enabled);
}

int MB_CommGetChildState (void)
{
	if (pCwork) {
		return pCwork->c_comm_state;
	}

	return 0;
}

u16 MB_GetChildProgressPercentage (void)
{
	OSIntrMode enabled;
	u16 ret = 0;

	enabled = OS_DisableInterrupts();

	if (pCwork->total_block > 0) {
		ret = (u16)((u32)(pCwork->got_block * 100) / pCwork->total_block);
	}

	(void)OS_RestoreInterrupts(enabled);

	return ret;
}

BOOL MB_CommStartDownload (void)
{
	OSIntrMode enabled;
	BOOL ret = FALSE;

	enabled = OS_DisableInterrupts();

	if (pCwork
	    && pCwork->c_comm_state == MB_COMM_CSTATE_DLINFO_ACCEPTED
	    && pCwork->user_req == MB_COMM_USER_REQ_NONE) {

		pCwork->user_req = MB_COMM_USER_REQ_DL_START;
		ret = TRUE;
	}

	(void)OS_RestoreInterrupts(enabled);
	return ret;
}

static void MBi_CommChangeChildState (int state, void * arg)
{
	pCwork->c_comm_state = state;
	MBi_CommChangeChildStateCallbackOnly(state, arg);
}

static void MBi_CommChangeChildStateCallbackOnly (int state, void * arg)
{
	if (pCwork->child_callback) {
		(*pCwork->child_callback)((u32)state, arg);
	}
}

void MBi_CommChildCallback (u16 type, void * arg)
{
	MB_COMM_WMEVENT_OUTPUT(type, arg);

	switch (type) {
	case MB_CALLBACK_INIT_COMPLETE:
		MBi_CommChangeChildState(MB_COMM_CSTATE_INIT_COMPLETE, arg);
		break;
	case MB_CALLBACK_PARENT_FOUND:
	{
		WMStartScanCallback * pSsc = (WMStartScanCallback *)arg;
		int parent_no = MBi_GetLastFountParent();
		WMBssDesc * bssDescp = MBi_GetParentBssDesc(parent_no);

		(void)MB_RecvGameInfoBeacon(MBi_CommBeaconRecvCallback, pSsc->linkLevel, bssDescp);
		MB_CountGameInfoLifetime(MBi_CommBeaconRecvCallback, TRUE);
	}
	break;
	case MB_CALLBACK_PARENT_NOT_FOUND:
		MB_CountGameInfoLifetime(MBi_CommBeaconRecvCallback, FALSE);
		break;
	case MB_CALLBACK_CONNECTED_TO_PARENT:
		MBi_CommChangeChildState(MB_COMM_CSTATE_CONNECT, arg);
		break;
	case MB_CALLBACK_MP_CHILD_RECV:
		MBi_CommChildRecvData(arg);
		break;
	case MB_CALLBACK_MP_SEND_ENABLE:
		(void)MBi_CommChildSendData();
		break;
	case MB_CALLBACK_END_COMPLETE:
		if (pCwork->c_comm_state == MB_COMM_CSTATE_BOOTREQ_ACCEPTED && pCwork->boot_end_flag == 1) {
			MI_CpuCopy16(&pCwork->dl_fileinfo, (void *)MB_DOWNLOAD_FILEINFO_ADDRESS, sizeof(MBDownloadFileInfo));
			MI_CpuCopy16(&pCwork->bssDescbuf, (void *)MB_BSSDESC_ADDRESS, MB_BSSDESC_SIZE);
			MBi_CommChangeChildState(MB_COMM_CSTATE_BOOT_READY, NULL);
		} else {
			MBi_CommChangeChildState(MB_COMM_CSTATE_CANCELLED, NULL);
			pCwork->common.user.playerNo = 0;
			clearRecvRegion();
			pCwork->c_comm_state = MB_COMM_CSTATE_NONE;
		}
		break;
	case MB_CALLBACK_CONNECT_FAILED:
		MBi_CommChangeChildState(MB_COMM_CSTATE_CONNECT_FAILED, arg);
		MB_DeleteRecvGameInfo(pCwork->connectTargetNo);
		pCwork->connectTargetNo = 0;
		(void)MBi_RestartScan();
		(void)MBi_CommEnd();
		break;
	case MB_CALLBACK_DISCONNECTED_FROM_PARENT:
		MBi_CommChangeChildState(MB_COMM_CSTATE_DISCONNECTED_BY_PARENT, arg);
		(void)MBi_RestartScan();
		(void)MBi_CommEnd();
		break;
	case MB_CALLBACK_API_ERROR:
	{
		u16 apiid, errcode;

		apiid = ((u16 *)arg)[0];
		errcode = ((u16 *)arg)[1];

		switch (errcode) {
		case WM_ERRCODE_INVALID_PARAM:
		case WM_ERRCODE_FAILED:
		case WM_ERRCODE_WM_DISABLE:
		case WM_ERRCODE_NO_DATASET:
		case WM_ERRCODE_FIFO_ERROR:
		case WM_ERRCODE_TIMEOUT:
			MBi_CommCallChildError(MB_ERRCODE_FATAL);
			break;
		case WM_ERRCODE_OPERATING:
		case WM_ERRCODE_ILLEGAL_STATE:
		case WM_ERRCODE_NO_CHILD:
		case WM_ERRCODE_OVER_MAX_ENTRY:
		case WM_ERRCODE_NO_ENTRY:
		case WM_ERRCODE_INVALID_POLLBITMAP:
		case WM_ERRCODE_NO_DATA:
		case WM_ERRCODE_SEND_QUEUE_FULL:
		case WM_ERRCODE_SEND_FAILED:
		default:
			MBi_CommCallChildError(MB_ERRCODE_WM_FAILURE);
			break;
		}
	}
	break;
	case MB_CALLBACK_ERROR:
	{
		WMCallback * pWmcb = (WMCallback *)arg;

		switch (pWmcb->apiid) {
		case WM_APIID_INITIALIZE:
		case WM_APIID_SET_LIFETIME:
		case WM_APIID_SET_P_PARAM:
		case WM_APIID_SET_BEACON_IND:
		case WM_APIID_START_PARENT:
		case WM_APIID_START_MP:
		case WM_APIID_SET_MP_DATA:
		case WM_APIID_START_DCF:
		case WM_APIID_SET_DCF_DATA:
		case WM_APIID_DISCONNECT:
		case WM_APIID_START_KS:
			MBi_CommCallChildError(MB_ERRCODE_FATAL);
			break;
		case WM_APIID_RESET:
		case WM_APIID_END:
		default:
			MBi_CommCallChildError(MB_ERRCODE_WM_FAILURE);
			break;
		}
	}
	break;
	default:
		break;
	}

#if (CALLBACK_WM_STATE == 1)
	if (pCwork->child_callback) {
		(*pCwork->child_callback)((u32)(MB_COMM_CSTATE_WM_EVENT | type), arg);
	}
#endif

	if (type == MB_CALLBACK_END_COMPLETE) {
		MI_CpuClear16(pCwork, sizeof(MB_CommCWork));
		pCwork = NULL;
	}
}

static void MBi_CommChildRecvData (void * arg)
{
	WMPortRecvCallback * bufp = (WMPortRecvCallback *)arg;
	MB_CommCWork * const p_child = pCwork;
	MBCommParentBlockHeader hd;
	u8 * databuf;
	u16 aid = MBi_GetAid();

	databuf = MBi_SetRecvBufferFromParent(&hd, (u8 *)bufp->data);

	MB_DEBUG_OUTPUT("RECV ");
	MB_COMM_TYPE_OUTPUT(hd.type);

	switch (hd.type) {
	case MB_COMM_TYPE_PARENT_SENDSTART:
		if (p_child->c_comm_state == MB_COMM_CSTATE_CONNECT) {
			MB_DEBUG_OUTPUT("Allowed to request file from parent!\n");
			MBi_CommChangeChildState(MB_COMM_CSTATE_REQ_ENABLE, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_KICKREQ:
		if (p_child->c_comm_state == MB_COMM_CSTATE_REQ_ENABLE) {
			MB_DEBUG_OUTPUT("Kicked from parent!\n");
			MBi_CommChangeChildState(MB_COMM_CSTATE_REQ_REFUSED, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_MEMBER_FULL:
		if (p_child->c_comm_state == MB_COMM_CSTATE_REQ_ENABLE) {
			MB_DEBUG_OUTPUT("Member full!\n");
			MBi_CommChangeChildState(MB_COMM_CSTATE_MEMBER_FULL, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_DL_FILEINFO:
		if (p_child->c_comm_state == MB_COMM_CSTATE_REQ_ENABLE) {
			MI_CpuCopy8(databuf, &p_child->dl_fileinfo, sizeof(MBDownloadFileInfo));
			MB_DEBUG_OUTPUT("Download File Info has received (Total block num = %d)\n",
			                p_child->total_block);
			if (!MBi_MakeBlockInfoTable(&p_child->blockinfo_table,
			                            (MbDownloadFileInfoHeader *) &p_child->dl_fileinfo)) {

				MBi_CommCallChildError(MB_ERRCODE_INVALID_DLFILEINFO);
				OS_TWarning("The received DownloadFileInfo is illegal.\n");
				return;
			}

			p_child->common.user.playerNo = aid;
			p_child->total_block = MBi_get_blocknum(&p_child->blockinfo_table);

			MBi_CommChangeChildState(MB_COMM_CSTATE_DLINFO_ACCEPTED, (void *)&p_child->dl_fileinfo);
		}
		break;
	case MB_COMM_TYPE_PARENT_DATA:
		if (p_child->c_comm_state == MB_COMM_CSTATE_DLINFO_ACCEPTED
		    && p_child->user_req == MB_COMM_USER_REQ_DL_START) {
			MBi_CommChangeChildState(MB_COMM_CSTATE_RECV_PROCEED, NULL);
			p_child->user_req = MB_COMM_USER_REQ_NONE;

		}
		if (p_child->c_comm_state == MB_COMM_CSTATE_RECV_PROCEED) {
			u16 block_num;
			MB_BlockInfo bi;

			block_num = p_child->total_block;

			if (block_num == 0 || block_num >= MB_MAX_BLOCK) {
				MBi_CommCallChildError(MB_ERRCODE_INVALID_BLOCK_NUM);
				OS_TWarning("Illegal Number of Block! [%d]\n", block_num);
				return;
			}

			if (hd.seqno < 0 ||
			    hd.seqno >= block_num ||
			    MBi_get_blockinfo(&bi, &p_child->blockinfo_table, hd.seqno,
			                      &p_child->dl_fileinfo.header) == FALSE) {

				MBi_CommCallChildError(MB_ERRCODE_INVALID_BLOCK_NO);
				OS_TWarning("The illegal block No.[%d] has been received! (maxnum %d)\n",
				            hd.seqno, block_num);
				goto CheckRemainBlock;
			}

			if (hd.fid != p_child->fileid) {
				MBi_CommCallChildError(MB_ERRCODE_INVALID_FILE);
				OS_TWarning("Received File ID [%d] differs from what was requested!\n", hd.fid);
				goto CheckRemainBlock;
			}

			if (!MBi_IsAbleToRecv(bi.segment_no, bi.child_address, bi.size)) {
				MBi_CommCallChildError(MB_ERRCODE_INVALID_RECV_ADDR);
				OS_TWarning("The receive address of Block No.%d is illegal. [%08x - %08x]\n",
				            hd.seqno, bi.child_address, bi.child_address + bi.size);
				goto CheckRemainBlock;
			}

			if (checkRecvFlag(hd.seqno) == FALSE) {
				MB_DEBUG_OUTPUT("DATA : BLOCK(%d)/REMAIN(%d), Recv address[%x] size[%x]\n",
				                hd.seqno, countRemainingblocks(), bi.child_address, bi.size);
				MI_CpuCopy8(databuf, (void *)bi.child_address, bi.size);
				p_child->got_block++;
				setRecvFlag(hd.seqno);
			}

CheckRemainBlock:
			if (0 == countRemainingblocks()) {
				MBi_CommChangeChildState(MB_COMM_CSTATE_RECV_COMPLETE, NULL);
			}
		}
		break;
	case MB_COMM_TYPE_PARENT_BOOTREQ:
		if (p_child->c_comm_state == MB_COMM_CSTATE_RECV_COMPLETE) {
			MBi_CommChangeChildState(MB_COMM_CSTATE_BOOTREQ_ACCEPTED, NULL);
		} else if (p_child->c_comm_state == MB_COMM_CSTATE_BOOTREQ_ACCEPTED)   {
			p_child->boot_end_flag = 1;
			(void)MBi_CommEnd();
		}
		break;
	default:
		break;
	}
	return;
}

static int MBi_CommChildSendData (void)
{
	u16 block_num = pCwork->total_block;
	MBCommChildBlockHeader hd;
	int errcode = 0;
	u16 pollbmp = 0xffff;

	switch (pCwork->c_comm_state) {
	default:
		hd.type = MB_COMM_TYPE_DUMMY;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)pCwork->common.sendbuf);
		errcode = MBi_BlockHeaderEnd(MB_COMM_CHILD_HEADER_SIZE, pollbmp, pCwork->common.sendbuf);
		break;
	case MB_COMM_CSTATE_REQ_ENABLE:
	{
		const MbBeaconRecvStatus * mbrsp = MB_GetBeaconRecvStatus();
		errcode = MBi_CommRequestFile(pCwork->fileid, mbrsp->list[pCwork->connectTargetNo].gameInfo.ggid);
		MB_DEBUG_OUTPUT("Requested File (errcode:%d)\n", errcode);

	}
	break;
	case MB_COMM_CSTATE_DLINFO_ACCEPTED:
		hd.type = MB_COMM_TYPE_CHILD_ACCEPT_FILEINFO;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)pCwork->common.sendbuf);
		errcode = MBi_BlockHeaderEnd(MB_COMM_CHILD_HEADER_SIZE, pollbmp, pCwork->common.sendbuf);
		break;
	case MB_COMM_CSTATE_RECV_PROCEED:
		MI_CpuClear8(&hd, sizeof(MBCommChildBlockHeader));
		hd.type = MB_COMM_TYPE_CHILD_CONTINUE;
		hd.data.req = get_next_blockno();
		hd.data.reserved[0] = (u8)(0x00ff & pCwork->got_block);
		hd.data.reserved[1] = (u8)((0xff00 & pCwork->got_block) >> 8);
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)pCwork->common.sendbuf);
		errcode = MBi_BlockHeaderEnd(MB_COMM_CHILD_HEADER_SIZE, pollbmp, pCwork->common.sendbuf);
		break;
	case MB_COMM_CSTATE_RECV_COMPLETE:
		hd.type = MB_COMM_TYPE_CHILD_STOPREQ;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)pCwork->common.sendbuf);
		errcode = MBi_BlockHeaderEnd(MB_COMM_CHILD_HEADER_SIZE, pollbmp, pCwork->common.sendbuf);
		break;
	case MB_COMM_CSTATE_BOOTREQ_ACCEPTED:
		hd.type = MB_COMM_TYPE_CHILD_BOOTREQ_ACCEPTED;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)pCwork->common.sendbuf);
		errcode = MBi_BlockHeaderEnd(MB_COMM_CHILD_HEADER_SIZE, pollbmp, pCwork->common.sendbuf);
		break;
	}

	return errcode;
}

static void MBi_CommBeaconRecvCallback (MbBeaconMsg msg, MBGameInfoRecvList * gInfop, int index)
{
#pragma unused(index)
	void * arg = (gInfop == NULL)? NULL : (void *)&gInfop->gameInfo;

	switch (msg) {
	case MB_BC_MSG_GINFO_VALIDATED:
		MBi_CommChangeChildStateCallbackOnly(MB_COMM_CSTATE_GAMEINFO_VALIDATED, arg);
		MB_DEBUG_OUTPUT("Parent Info Enable\n");
		break;
	case MB_BC_MSG_GINFO_INVALIDATED:
		MBi_CommChangeChildStateCallbackOnly(MB_COMM_CSTATE_GAMEINFO_INVALIDATED, arg);
		break;
	case MB_BC_MSG_GINFO_LOST:
		MBi_CommChangeChildStateCallbackOnly(MB_COMM_CSTATE_GAMEINFO_LOST, arg);
		break;
	case MB_BC_MSG_GINFO_LIST_FULL:
		MBi_CommChangeChildStateCallbackOnly(MB_COMM_CSTATE_GAMEINFO_LIST_FULL, arg);
		break;
	}
}

void MB_ReadMultiBootParentBssDesc (WMBssDesc * p_desc,
                                    u16 parent_max_size, u16 child_max_size, BOOL ks_flag,
                                    BOOL cs_flag)
{
	const MBParentBssDesc * parentInfo = MB_GetMultiBootParentBssDesc();
	SDK_NULL_ASSERT(parentInfo);
	MI_CpuCopy8(parentInfo, p_desc, sizeof(MBParentBssDesc));

	p_desc->gameInfoLength = 0x10;
	p_desc->gameInfo.magicNumber = 0x0001;
	p_desc->gameInfo.ver = 0;
	p_desc->gameInfo.ggid = (u32)(*(const u16 *)(&p_desc->ssid[0]) | (*(const u16 *)(&p_desc->ssid[2]) << 16));
	p_desc->gameInfo.tgid = *(const u16 *)(&p_desc->ssid[4]);
	p_desc->gameInfo.userGameInfoLength = 0;
	p_desc->gameInfo.parentMaxSize = parent_max_size;
	p_desc->gameInfo.childMaxSize = child_max_size;

	MI_WriteByte(&p_desc->gameInfo.attribute,
	             (u8)((ks_flag ? WM_ATTR_FLAG_KS : 0) |
	                  (cs_flag ? WM_ATTR_FLAG_CS : 0) | WM_ATTR_FLAG_ENTRY));
}

static BOOL checkRecvFlag (int x)
{
	int pos = (x >> 3);
	int bit = (x & 0x7);

	SDK_ASSERT(x < MB_MAX_BLOCK);

	if (pCwork->recvflag[pos] & (1 << bit)) {
		return TRUE;
	}

	return FALSE;
}

static void setRecvFlag (int x)
{
	int pos = (x >> 3);
	int bit = (x & 0x7);

	SDK_ASSERT(x < MB_MAX_BLOCK);

	pCwork->recvflag[pos] |= (1 << bit);
	pCwork->last_recv_seq_no = x;
}

static u16 countRemainingblocks (void)
{
	return (u16)(pCwork->total_block - pCwork->got_block);
}

static u16 get_next_blockno (void)
{
	int req;
	int search_count = 0;

	req = pCwork->last_recv_seq_no;
	req++;

	while (1) {
		if (req < 0 || req >= pCwork->total_block) {
			req = 0;
		} else if (checkRecvFlag(req))   {
			req++;
		} else {
			return (u16)req;
		}

		if (pCwork->last_recv_seq_no == req) {
			return (u16)(pCwork->total_block);
		}

		search_count++;

		if (search_count > 1000) {
			pCwork->last_recv_seq_no = req;
			return (u16)req;
		}
	}
}

static BOOL checkPollbmp (u16 pollbmp, u16 child_aid)
{
	if (pollbmp & (u16)(1 << child_aid)) {
		return TRUE;
	}

	return FALSE;
}

static void clearRecvRegion (void)
{
#ifdef SDK_FINALROM
	MI_CpuClearFast((void *)MB_LOAD_AREA_LO, MB_LOAD_MAX_SIZE);
	MI_CpuClearFast((void *)MB_ARM7_STATIC_RECV_BUFFER, MB_ARM7_STATIC_RECV_BUFFER_SIZE);
	MI_CpuClear16((void *)MB_BSSDESC_ADDRESS, MB_DOWNLOAD_FILEINFO_ADDRESS + MB_DOWNLOAD_FILEINFO_SIZE - MB_BSSDESC_ADDRESS);
	MI_CpuClear16((void *)MB_ROM_HEADER_ADDRESS, ROM_HEADER_SIZE_FULL);
#endif

}

static void MBi_CommCallChildError (u16 errcode)
{
	MBErrorStatus e_stat;
	e_stat.errcode = errcode;

	MBi_CommChangeChildStateCallbackOnly(MB_COMM_CSTATE_ERROR, &e_stat);
}