#include <nitro.h>

#include "mb_common.h"
#include "mb_block.h"
#include "mb_child.h"
#include "mb_private.h"
#include "mb_wm_base.h"
#include "mb_fake_child.h"
#include "mb_wm.h"

#define MB_FAKE_PRINT (0)

#if (MB_FAKE_PRINT == 1)
    #define MB_FAKE_OUTPUT          OS_TPrintf
#else
    #define MB_FAKE_OUTPUT(...)    ((void)0)
#endif

#define MY_ROUND(n, a)    (((u32) (n) + (a) - 1) & ~((a) - 1))

typedef struct {
	u32 sendBuf[WM_SIZE_CHILD_SEND_BUFFER(MB_COMM_PARENT_RECV_MAX, FALSE) / sizeof(u32)] ATTRIBUTE_ALIGN(32);
	u32 recvBuf[WM_SIZE_CHILD_RECEIVE_BUFFER(MB_COMM_PARENT_SEND_MAX, FALSE) / sizeof(u32)] ATTRIBUTE_ALIGN(32);
	u32 sendDataBuf[WM_SIZE_CHILD_SEND_BUFFER(MB_COMM_PARENT_RECV_MAX, FALSE) / sizeof(u32)] ATTRIBUTE_ALIGN(32);
	WMBssDesc bssDescBuf ATTRIBUTE_ALIGN(32);
	WMScanParam scanParam ATTRIBUTE_ALIGN(32);
	MBWMWork wmWork ATTRIBUTE_ALIGN(32);
	MBUserInfo userInfo;
	u8 _padding1[2];
	MBFakeScanCallbackFunc scanCallback;
	MBCommCStateCallbackFunc stateCallback;
	u32 ggid;
	MbBeaconRecvStatus beaconRecvStatus;
	BOOL scanning;
	BOOL endScanBusy;
	BOOL locking;
	BOOL endFlag;
	u32 targetGgid;
	u16 targetFileNo;
	u16 c_comm_state;
	WMCallbackFunc verboseScanCallback;
	MBFakeCompareGGIDCallbackFunc compareGGIDCallback;
	u8 _padding2[8];
} MBFakeWork;

SDK_COMPILER_ASSERT(sizeof(MBFakeWork) <= MB_FAKE_WORK_SIZE);

static void MBFi_EndComplete(void);

static void MBFi_CommChangeChildState(u16 state, void * arg);
static void MBFi_SendCallback(u16 state, void * arg);
static void MBFi_ErrorCallback(u16 apiid, u16 errcode, BOOL isApiError);

static void MBFi_StateInStartScan(void);
static void MBFi_StateOutStartScanParent(void * arg);
static void MBFi_CommBeaconRecvCallback(MbBeaconMsg msg, MBGameInfoRecvList * gInfop, int index);

static void MBFi_ScanCallback(u16 state, MBGameInfoRecvList * gInfop, int index);
static void MBFi_ScanErrorCallback(u16 apiid, u16 errcode);
static void MBFi_ScanLock(u8 * macAddr);
static void MBFi_ScanUnlock(void);
static BOOL RotateChannel(void);

static void MBFi_StateInEndScan(void);
static void MBFi_StateOutEndScan(void * arg);

static void MBFi_WMCallback(u16 type, void * arg);
static void MBFi_CommChildRecvData(WMPortRecvCallback * cb);
static void MBFi_CommChildSendData(void);

#if (MB_FAKE_PRINT == 1)
    static void MBFi_PrintMBCallbackType(u16 type);
    static void MBFi_PrintMBCommCallbacyType(u16 type);
#else
    #define MBFi_PrintMBCallbackType(a)         ((void)0)
    #define MBFi_PrintMBCommCallbackType(a)     ((void)0)
#endif

static vu16 mbf_initialize;
static MBFakeWork * mbf_work;

void MB_FakeInit (void * buf, const MBUserInfo * user)
{
	SDK_NULL_ASSERT(buf);
	SDK_ASSERT(((u32)buf & 0x1F) == 0);

	if (mbf_initialize) {
		OS_TWarning("MB_FakeInit multiply called\n");
		return;
	}

	mbf_initialize = 1;

	MB_FAKE_OUTPUT("MB_Fake Initialized\n");

	MI_CpuClear8(buf, sizeof(MBFakeWork));

	mbf_work = (MBFakeWork *) buf;
	mbf_work->c_comm_state = MB_COMM_CSTATE_NONE;
	mbf_work->verboseScanCallback = NULL;
	mbf_work->compareGGIDCallback = NULL;

	MI_CpuCopy8(user, &mbf_work->userInfo, sizeof(MBUserInfo));

	MBi_SetBeaconRecvStatusBuffer(&mbf_work->beaconRecvStatus);
	MB_InitRecvGameInfoStatus();
	MBi_SetScanLockFunc(MBFi_ScanLock, MBFi_ScanUnlock);
}

void MB_FakeEnd (void)
{
	if (mbf_work->endFlag) {
		return;
	}

	mbf_work->endFlag = TRUE;

	switch (mbf_work->c_comm_state) {
	case MB_COMM_CSTATE_NONE:
		if (mbf_work->scanning) {
			mbf_work->scanning = FALSE;
		} else if (!mbf_work->endScanBusy)   {
			MBFi_EndComplete();
		} else {

		}
		break;
	case MB_COMM_CSTATE_INIT_COMPLETE:
	case MB_COMM_CSTATE_CONNECT:
	case MB_COMM_CSTATE_CONNECT_FAILED:
	case MB_COMM_CSTATE_DISCONNECTED_BY_PARENT:
	case MB_COMM_CSTATE_REQ_REFUSED:
	case MB_COMM_CSTATE_ERROR:
	case MB_COMM_CSTATE_MEMBER_FULL:
		MBi_WMReset();
		break;
	case MB_COMM_CSTATE_REQ_ENABLE:
	case MB_COMM_CSTATE_DLINFO_ACCEPTED:
	case MB_COMM_CSTATE_RECV_PROCEED:
	case MB_COMM_CSTATE_RECV_COMPLETE:
		MBi_WMDisconnect();
		break;
	case MB_COMM_CSTATE_BOOTREQ_ACCEPTED:
		break;
	case MB_COMM_CSTATE_BOOT_READY:
	case MB_COMM_CSTATE_CANCELLED:
		MBFi_EndComplete();
		break;
	default:
		OS_TPanic("MB fake child is in Illegal State\n");
	}
}

static void MBFi_EndComplete (void)
{
	mbf_initialize = 0;
	mbf_work->endFlag = 0;

	MBi_WMClearCallback();
	MBFi_CommChangeChildState(MB_COMM_CSTATE_FAKE_END, NULL);
}

u32 MB_FakeGetWorkSize (void)
{
	return sizeof(MBFakeWork);
}

void MB_FakeSetCStateCallback (MBCommCStateCallbackFunc callback)
{
	OSIntrMode enabled = OS_DisableInterrupts();

	mbf_work->stateCallback = callback;

	(void)OS_RestoreInterrupts(enabled);
}

static void MBFi_CommChangeChildState (u16 state, void * arg)
{
#pragma unused( arg )
	OSIntrMode enabled;

	static const char * const CSTATE_NAME_STRING[] = {
		"MB_COMM_CSTATE_NONE",
		"MB_COMM_CSTATE_INIT_COMPLETE",
		"MB_COMM_CSTATE_CONNECT",
		"MB_COMM_CSTATE_CONNECT_FAILED",
		"MB_COMM_CSTATE_DISCONNECTED_BY_PARENT",
		"MB_COMM_CSTATE_REQ_ENABLE",
		"MB_COMM_CSTATE_REQ_REFUSED",
		"MB_COMM_CSTATE_DLINFO_ACCEPTED",
		"MB_COMM_CSTATE_RECV_PROCEED",
		"MB_COMM_CSTATE_RECV_COMPLETE",
		"MB_COMM_CSTATE_BOOTREQ_ACCEPTED",
		"MB_COMM_CSTATE_BOOT_READY",
		"MB_COMM_CSTATE_CANCELLED",
		"MB_COMM_CSTATE_AUTHENTICATION_FAILED",
		"MB_COMM_CSTATE_MEMBER_FULL",
		"MB_COMM_CSTATE_GAMEINFO_VALIDATED",
		"MB_COMM_CSTATE_GAMEINFO_INVALIDATED",
		"MB_COMM_CSTATE_GAMEINFO_LOST",
		"MB_COMM_CSTATE_GAMEINFO_LIST_FULL",
		"MB_COMM_CSTATE_ERROR",
		"MB_COMM_CSTATE_FAKE_END",
	};

	MB_FAKE_OUTPUT("state %s -> %s\n", CSTATE_NAME_STRING[mbf_work->c_comm_state],
	               CSTATE_NAME_STRING[state]);

	enabled = OS_DisableInterrupts();
	mbf_work->c_comm_state = state;

	(void)OS_RestoreInterrupts(enabled);
	MBFi_SendCallback(state, arg);
}

static inline void MBFi_SendCallback (u16 state, void * arg)
{
	if (mbf_work->stateCallback == NULL) {
		return;
	}

	mbf_work->stateCallback(state, arg);
}

static inline void MBFi_ErrorCallback (u16 apiid, u16 errcode, BOOL isApiError)
{
	MBErrorStatus cb;
	u16 error_type;

	if (mbf_work->stateCallback == NULL) {
		return;
	}

	if (isApiError) {
		switch (errcode) {
		case WM_ERRCODE_INVALID_PARAM:
		case WM_ERRCODE_FAILED:
		case WM_ERRCODE_WM_DISABLE:
		case WM_ERRCODE_NO_DATASET:
		case WM_ERRCODE_FIFO_ERROR:
		case WM_ERRCODE_TIMEOUT:
			error_type = MB_ERRCODE_FATAL;
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
			error_type = MB_ERRCODE_WM_FAILURE;
			break;
		}
	} else {
		switch (apiid) {
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
			error_type = MB_ERRCODE_FATAL;
			break;
		case WM_APIID_RESET:
		case WM_APIID_END:
		default:
			error_type = MB_ERRCODE_WM_FAILURE;
			break;
		}
	}

	MB_FAKE_OUTPUT("MBFi_ErrorCallback apiid = 0x%x, errcode = 0x%x\n", apiid, errcode);

    cb.errcode = error_type;
	mbf_work->stateCallback(MB_COMM_CSTATE_ERROR, &cb);
}

void MB_FakeSetVerboseScanCallback (WMCallbackFunc callback)
{
	OSIntrMode enabled = OS_DisableInterrupts();

	mbf_work->verboseScanCallback = callback;
	(void)OS_RestoreInterrupts(enabled);
}

void MB_FakeSetCompareGGIDCallback (MBFakeCompareGGIDCallbackFunc callback)
{
	OSIntrMode enabled = OS_DisableInterrupts();

	mbf_work->compareGGIDCallback = callback;
	(void)OS_RestoreInterrupts(enabled);
}

void MB_FakeStartScanParent (MBFakeScanCallbackFunc callback, u32 ggid)
{
	MB_FAKE_OUTPUT("%s\n", __func__);

	mbf_work->scanCallback = callback;
	mbf_work->ggid = ggid;
	mbf_work->scanParam.channel = 0;
	mbf_work->scanParam.scanBuf = &mbf_work->bssDescBuf;
	mbf_work->scanning = TRUE;
	mbf_work->locking = FALSE;
	mbf_work->beaconRecvStatus.nowScanTargetFlag = FALSE;

	MBFi_StateInStartScan();
}

static void MBFi_StateInStartScan (void)
{
	WMErrCode result;

	if (mbf_work->locking) {
		mbf_work->scanParam.maxChannelTime = MB_SCAN_TIME_LOCKING;
	} else {
		static const u8 ANY_PARENT[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

		WM_CopyBssid(ANY_PARENT, mbf_work->scanParam.bssid);
		mbf_work->scanParam.maxChannelTime = MB_SCAN_TIME_NORMAL;

		if (!RotateChannel()) {
			MBFi_ScanErrorCallback(WM_APIID_MEASURE_CHANNEL, 0);
			return;
		}
	}

	if (mbf_work->scanParam.channel == 0) {
		mbf_work->scanParam.channel = 1;
	}

	result = WM_StartScan(MBFi_StateOutStartScanParent, &mbf_work->scanParam);

	if (result != WM_ERRCODE_OPERATING) {
		MBFi_ScanErrorCallback(WM_APIID_START_SCAN, result);
	}
}

static void MBFi_StateOutStartScanParent (void * arg)
{
	WMStartScanCallback * cb = (WMStartScanCallback *)arg;

	if (WM_ERRCODE_SUCCESS != cb->errcode) {
		MBFi_ScanErrorCallback(WM_APIID_START_SCAN, cb->errcode);
		return;
	}

	switch (cb->state) {
	case WM_STATECODE_SCAN_START:
		break;
	case WM_STATECODE_PARENT_FOUND:
	{

		BOOL matched = (mbf_work->compareGGIDCallback == NULL) ?
		               (cb->gameInfo.ggid == mbf_work->ggid) :
		               (*mbf_work->compareGGIDCallback)(cb, mbf_work->ggid);

		if (mbf_work->verboseScanCallback) {
			mbf_work->verboseScanCallback(arg);
		}

		if (matched) {
			DC_InvalidateRange(&mbf_work->bssDescBuf, WM_BSS_DESC_SIZE);
			(void)MB_RecvGameInfoBeacon(MBFi_CommBeaconRecvCallback, cb->linkLevel, &mbf_work->bssDescBuf);
			MB_CountGameInfoLifetime(MBFi_CommBeaconRecvCallback, TRUE);
		}
	}
	case WM_STATECODE_PARENT_NOT_FOUND:
		MB_CountGameInfoLifetime(MBFi_CommBeaconRecvCallback, FALSE);

		if (mbf_work->scanning) {
			MBFi_StateInStartScan();
		} else {
			MBFi_StateInEndScan();
		}
		break;
	default:
		MBFi_ScanErrorCallback(WM_APIID_START_SCAN, WM_ERRCODE_FAILED);
		break;
	}
}

static void MBFi_ScanCallback (u16 state, MBGameInfoRecvList * gInfop, int index)
{
	MBFakeScanCallback cb;

	if (mbf_work->scanCallback == NULL) {
		return;
	}

	cb.index = (u16)index;

	if (gInfop != NULL) {
		cb.gameInfo = &gInfop->gameInfo;
		cb.bssDesc = &gInfop->bssDesc;
	} else {
		cb.gameInfo = NULL;
		cb.bssDesc = NULL;
	}

	mbf_work->scanCallback(state, &cb);
}

static void MBFi_ScanErrorCallback (u16 apiid, u16 errcode)
{
	MBFakeScanErrorCallback cb;

	if (mbf_work->scanCallback == NULL) {
		return;
	}

	cb.apiid = apiid;
	cb.errcode = errcode;

	mbf_work->scanCallback(MB_FAKESCAN_API_ERROR, &cb);
}

static void MBFi_ScanLock (u8 * macAddr)
{
	MB_FAKE_OUTPUT("Scan Locked\n");

	mbf_work->locking = TRUE;
	WM_CopyBssid(macAddr, mbf_work->scanParam.bssid);
}

static void MBFi_ScanUnlock (void)
{
	MB_FAKE_OUTPUT("Scan Unlocked\n");
	mbf_work->locking = FALSE;
}

static void MBFi_CommBeaconRecvCallback (MbBeaconMsg msg, MBGameInfoRecvList * gInfop, int index)
{
	switch (msg) {
	case MB_BC_MSG_GINFO_VALIDATED:
		MBFi_ScanCallback(MB_FAKESCAN_PARENT_FOUND, (void *)gInfop, index);
		MB_FAKE_OUTPUT("Parent Info validated\n");
		break;
	case MB_BC_MSG_GINFO_INVALIDATED:
		MB_FAKE_OUTPUT("Parent Info invalidate\n");
		break;
	case MB_BC_MSG_GINFO_LOST:
		MBFi_ScanCallback(MB_FAKESCAN_PARENT_LOST, (void *)gInfop, index);
		MB_FAKE_OUTPUT("Parent Info Lost\n");
		break;
	case MB_BC_MSG_GINFO_LIST_FULL:
		MB_FAKE_OUTPUT("Parent List Full\n");
		break;
	case MB_BC_MSG_GINFO_BEACON:
		MBFi_ScanCallback(MB_FAKESCAN_PARENT_BEACON, (void *)gInfop, index);
		break;
	}
}

static BOOL RotateChannel (void)
{
	u16 allowedChannel = WM_GetAllowedChannel();

	if (allowedChannel == 0x8000 || allowedChannel == 0) {
		return FALSE;
	}

	mbf_work->scanParam.channel++;

	while (TRUE) {
		mbf_work->scanParam.channel++;
		if (mbf_work->scanParam.channel > 16) {
			mbf_work->scanParam.channel = 1;
		}
		if (allowedChannel & (0x0001 << (mbf_work->scanParam.channel - 1))) {
			break;
		}
	}

	return TRUE;
}

void MB_FakeEndScan (void)
{
	mbf_work->scanning = FALSE;
	mbf_work->endScanBusy = TRUE;
}

static void MBFi_StateInEndScan (void)
{
	WMErrCode result;

	result = WM_EndScan(MBFi_StateOutEndScan);
	if (result != WM_ERRCODE_OPERATING) {
		MBFi_ScanErrorCallback(WM_APIID_END_SCAN, result);
	}
}

static void MBFi_StateOutEndScan (void * arg)
{
	WMCallback * cb = (WMCallback *)arg;

	if (cb->errcode != WM_ERRCODE_SUCCESS) {
		MBFi_ScanErrorCallback(WM_APIID_END_SCAN, cb->errcode);
		return;
	}

	mbf_work->endScanBusy = FALSE;

	if (mbf_work->endFlag) {
		MBFi_EndComplete();
		return;
	}

	MBFi_ScanCallback(MB_FAKESCAN_END_SCAN, NULL, 0);
}

BOOL MB_FakeEntryToParent (u16 index)
{
	MBGameInfoRecvList * info;

	info = MB_GetGameInfoRecvList(index);

	if (info == NULL) {
		return FALSE;
	}

	mbf_work->targetGgid = info->gameInfo.ggid;
	mbf_work->targetFileNo = info->gameInfo.fileNo;

	MBi_WMSetBuffer(&mbf_work->wmWork);
	MBi_WMSetCallback(MBFi_WMCallback);
	MBFi_CommChangeChildState(MB_COMM_CSTATE_INIT_COMPLETE, NULL);
	MI_CpuCopy8(&info->bssDesc, &mbf_work->bssDescBuf, sizeof(WMBssDesc));
	MBi_WMStartConnect(&mbf_work->bssDescBuf);

	return TRUE;
}

static void MBFi_WMCallback (u16 type, void * arg)
{
#pragma unused(arg)

	switch (type) {
	case MB_CALLBACK_CONNECT_FAILED:
		MBFi_CommChangeChildState(MB_COMM_CSTATE_CONNECT_FAILED, NULL);
		break;
	case MB_CALLBACK_CONNECTED_TO_PARENT:
		MB_FAKE_OUTPUT("connect to parent\n");
		MBFi_CommChangeChildState(MB_COMM_CSTATE_CONNECT, arg);
		MBi_ChildStartMP((u16 *)mbf_work->sendBuf, (u16 *)mbf_work->recvBuf);
		break;
	case MB_CALLBACK_DISCONNECTED_FROM_PARENT:
		MBFi_CommChangeChildState(MB_COMM_CSTATE_DISCONNECTED_BY_PARENT, arg);
		break;
	case MB_CALLBACK_MP_STARTED:
		break;
	case MB_CALLBACK_MP_SEND_ENABLE:
	{
		MBFi_CommChildSendData();
	}
	break;
	case MB_CALLBACK_MP_CHILD_RECV:
	{
		MBFi_CommChildRecvData((WMPortRecvCallback *)arg);
	}
	break;
	case MB_CALLBACK_DISCONNECT_COMPLETE:
		if (mbf_work->c_comm_state == MB_COMM_CSTATE_BOOTREQ_ACCEPTED) {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_BOOT_READY, NULL);
		} else {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_CANCELLED, NULL);
		}

		if (mbf_work->endFlag) {
			MBFi_EndComplete();
		}
		break;
	case MB_CALLBACK_MP_CHILD_SENT:
	case MB_CALLBACK_MP_CHILD_SENT_TIMEOUT:
	case MB_CALLBACK_MP_CHILD_SENT_ERR:
		break;
	case MB_CALLBACK_API_ERROR:
	{
		MBErrorCallback * cb = (MBErrorCallback *) arg;
		MBFi_ErrorCallback(cb->apiid, cb->errcode, TRUE);
	}
	break;
	case MB_CALLBACK_ERROR:
	{
		MBErrorCallback * cb = (MBErrorCallback *) arg;
		MBFi_ErrorCallback(cb->apiid, cb->errcode, FALSE);
	}
	break;
	default:
		break;
	}
}

static void MBFi_CommChildRecvData (WMPortRecvCallback * cb)
{
	MBCommParentBlockHeader hd;

	(void)MBi_SetRecvBufferFromParent(&hd, (u8 *)cb->data);

	switch (hd.type) {
	case MB_COMM_TYPE_PARENT_SENDSTART:
		if (mbf_work->c_comm_state == MB_COMM_CSTATE_CONNECT) {
			MB_FAKE_OUTPUT("Allowd to request file from parent!\n");
			MBFi_CommChangeChildState(MB_COMM_CSTATE_REQ_ENABLE, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_KICKREQ:
		if (mbf_work->c_comm_state == MB_COMM_CSTATE_REQ_ENABLE) {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_REQ_REFUSED, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_MEMBER_FULL:
		if (mbf_work->c_comm_state == MB_COMM_CSTATE_REQ_ENABLE) {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_MEMBER_FULL, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_DL_FILEINFO:
		if (mbf_work->c_comm_state == MB_COMM_CSTATE_REQ_ENABLE) {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_DLINFO_ACCEPTED, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_DATA:
		if (mbf_work->c_comm_state == MB_COMM_CSTATE_DLINFO_ACCEPTED) {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_RECV_PROCEED, NULL);
		}

		if (mbf_work->c_comm_state == MB_COMM_CSTATE_RECV_PROCEED) {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_RECV_COMPLETE, NULL);
		}
		break;
	case MB_COMM_TYPE_PARENT_BOOTREQ:
		if (mbf_work->c_comm_state == MB_COMM_CSTATE_RECV_COMPLETE) {
			MBFi_CommChangeChildState(MB_COMM_CSTATE_BOOTREQ_ACCEPTED, NULL);
		} else if (mbf_work->c_comm_state == MB_COMM_CSTATE_BOOTREQ_ACCEPTED)   {
			MBi_WMDisconnect();
		}
		break;
	default:
		break;
	}

	return;
}

static void MBFi_CommChildSendData (void)
{
	MBCommChildBlockHeader hd;
	WMErrCode errcode = WM_ERRCODE_SUCCESS;
	u16 pollbmp = 0xffff;

	switch (mbf_work->c_comm_state) {
	case MB_COMM_CSTATE_REQ_ENABLE:
	{
		MBCommRequestData req_data;
		u8 * databuf;

		req_data.ggid = mbf_work->targetGgid;
		req_data.version = MB_IPL_VERSION;
		req_data.fileid = (u8)mbf_work->targetFileNo;

		MI_CpuCopy8(&mbf_work->userInfo, &req_data.userinfo, sizeof(MBUserInfo));

		hd.type = MB_COMM_TYPE_CHILD_FILEREQ;
		hd.req_data.piece = MBi_SendRequestDataPiece(hd.req_data.data, &req_data);
		databuf = MBi_MakeChildSendBuffer(&hd, (u8 *)mbf_work->sendDataBuf);

		if (databuf == NULL) {
			return;
		}

		errcode = MBi_MPSendToParent(MB_COMM_CHILD_HEADER_SIZE, pollbmp, mbf_work->sendDataBuf);
	}
	break;
	case MB_COMM_CSTATE_DLINFO_ACCEPTED:
		hd.type = MB_COMM_TYPE_CHILD_ACCEPT_FILEINFO;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)mbf_work->sendDataBuf);
		errcode = MBi_MPSendToParent(MB_COMM_CHILD_HEADER_SIZE, pollbmp, mbf_work->sendDataBuf);
		break;
	case MB_COMM_CSTATE_RECV_PROCEED:
		break;
	case MB_COMM_CSTATE_RECV_COMPLETE:
		hd.type = MB_COMM_TYPE_CHILD_STOPREQ;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)mbf_work->sendDataBuf);
		errcode = MBi_MPSendToParent(MB_COMM_CHILD_HEADER_SIZE, pollbmp, mbf_work->sendDataBuf);
		break;
	case MB_COMM_CSTATE_BOOTREQ_ACCEPTED:
		hd.type = MB_COMM_TYPE_CHILD_BOOTREQ_ACCEPTED;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)mbf_work->sendDataBuf);
		errcode = MBi_MPSendToParent(MB_COMM_CHILD_HEADER_SIZE, pollbmp, mbf_work->sendDataBuf);
		break;
	default:
		hd.type = MB_COMM_TYPE_DUMMY;
		(void)MBi_MakeChildSendBuffer(&hd, (u8 *)mbf_work->sendDataBuf);
		errcode = MBi_MPSendToParent(MB_COMM_CHILD_HEADER_SIZE, pollbmp, mbf_work->sendDataBuf);
		break;
	}
}

BOOL MB_FakeGetParentGameInfo (u16 index, MBGameInfo * pGameInfo)
{
	MBGameInfoRecvList * parentInfo;
	OSIntrMode enabled;

	if (index >= MB_GAME_INFO_RECV_LIST_NUM) {
		return FALSE;
	}

	enabled = OS_DisableInterrupts();
	parentInfo = MB_GetGameInfoRecvList(index);

	if (parentInfo == NULL) {
		(void)OS_RestoreInterrupts(enabled);
		return FALSE;
	}

	MI_CpuCopy8(&parentInfo->gameInfo, pGameInfo, sizeof(MBGameInfo));
	(void)OS_RestoreInterrupts(enabled);

	return TRUE;
}

BOOL MB_FakeGetParentBssDesc (u16 index, WMBssDesc * pBssDesc)
{
	MBGameInfoRecvList * parentInfo;
	OSIntrMode enabled;

	if (index >= MB_GAME_INFO_RECV_LIST_NUM) {
		return FALSE;
	}

	enabled = OS_DisableInterrupts();
	parentInfo = MB_GetGameInfoRecvList(index);

	if (parentInfo == NULL) {
		(void)OS_RestoreInterrupts(enabled);
		return FALSE;
	}

	MI_CpuCopy8(&parentInfo->bssDesc, pBssDesc, sizeof(WMBssDesc));
	(void)OS_RestoreInterrupts(enabled);

	return TRUE;
}

BOOL MB_FakeReadParentBssDesc (u16 index, WMBssDesc * pBssDesc, u16 parent_max_size,
                               u16 child_max_size, BOOL ks_flag, BOOL cs_flag)
{
	BOOL result;

	SDK_NULL_ASSERT(pBssDesc);

	result = MB_FakeGetParentBssDesc(index, pBssDesc);

	if (!result) {
		return FALSE;
	}

	pBssDesc->gameInfoLength = 0x10;
	pBssDesc->gameInfo.userGameInfoLength = 0;
	pBssDesc->gameInfo.parentMaxSize = parent_max_size;
	pBssDesc->gameInfo.childMaxSize = child_max_size;
	pBssDesc->gameInfo.attribute = (u8)((ks_flag ? WM_ATTR_FLAG_KS : 0) |
	                                    (cs_flag ? WM_ATTR_FLAG_CS : 0) | WM_ATTR_FLAG_ENTRY);
	return TRUE;
}

#if (MB_FAKE_PRINT == 1)
    static void MBFi_PrintMBCallbackType (u16 type)
    {
    #pragma unused( type )
        static const char * const CALLBACK_NAME[] = {
            "MB_CALLBACK_CHILD_CONNECTED",
            "MB_CALLBACK_CHILD_DISCONNECTED",
            "MB_CALLBACK_MP_PARENT_SENT",
            "MB_CALLBACK_MP_PARENT_RECV",
            "MB_CALLBACK_PARENT_FOUND",
            "MB_CALLBACK_PARENT_NOT_FOUND",
            "MB_CALLBACK_CONNECTED_TO_PARENT",
            "MB_CALLBACK_DISCONNECTED",
            "MB_CALLBACK_MP_CHILD_SENT",
            "MB_CALLBACK_MP_CHILD_RECV",
            "MB_CALLBACK_DISCONNECTED_FROM_PARENT",
            "MB_CALLBACK_CONNECT_FAILED",
            "MB_CALLBACK_DCF_CHILD_SENT",
            "MB_CALLBACK_DCF_CHILD_SENT_ERR",
            "MB_CALLBACK_DCF_CHILD_RECV",
            "MB_CALLBACK_DISCONNECT_COMPLETE",
            "MB_CALLBACK_DISCONNECT_FAILED",
            "MB_CALLBACK_END_COMPLETE",
            "MB_CALLBACK_MP_CHILD_SENT_ERR",
            "MB_CALLBACK_MP_PARENT_SENT_ERR",
            "MB_CALLBACK_MP_STARTED",
            "MB_CALLBACK_INIT_COMPLETE",
            "MB_CALLBACK_END_MP_COMPLETE",
            "MB_CALLBACK_SET_GAMEINFO_COMPLETE",
            "MB_CALLBACK_SET_GAMEINFO_FAILED",
            "MB_CALLBACK_MP_SEND_ENABLE",
            "MB_CALLBACK_PARENT_STARTED",
            "MB_CALLBACK_BEACON_LOST",
            "MB_CALLBACK_BEACON_SENT",
            "MB_CALLBACK_BEACON_RECV",
            "MB_CALLBACK_MP_SEND_DISABLE",
            "MB_CALLBACK_DISASSOCIATE",
            "MB_CALLBACK_REASSOCIATE",
            "MB_CALLBACK_AUTHENTICATE",
            "MB_CALLBACK_SET_LIFETIME",
            "MB_CALLBACK_DCF_STARTED",
            "MB_CALLBACK_DCF_SENT",
            "MB_CALLBACK_DCF_SENT_ERR",
            "MB_CALLBACK_DCF_RECV",
            "MB_CALLBACK_DCF_END",
            "MB_CALLBACK_MPACK_IND",
            "MB_CALLBACK_MP_CHILD_SENT_TIMEOUT",
            "MB_CALLBACK_SEND_QUEUE_FULL_ERR",
            "MB_CALLBACK_API_ERROR",
            "MB_CALLBACK_ERROR",
        };

        MB_FAKE_OUTPUT("RecvCallback %s\n", CALLBACK_NAME[type]);
    }

    static void MBFi_PrintMBCommCallbacyType (u16 type)
    {
    #pragma unused(type)
        static const char * const MB_TYPE_STRING[] = {
            "MB_COMM_TYPE_DUMMY",
            "MB_COMM_TYPE_PARENT_SENDSTART",
            "MB_COMM_TYPE_PARENT_KICKREQ",
            "MB_COMM_TYPE_PARENT_DL_FILEINFO",
            "MB_COMM_TYPE_PARENT_DATA",
            "MB_COMM_TYPE_PARENT_BOOTREQ",
            "MB_COMM_TYPE_PARENT_MEMBER_FULL",
            "MB_COMM_TYPE_CHILD_FILEREQ",
            "MB_COMM_TYPE_CHILD_ACCEPT_FILEINFO",
            "MB_COMM_TYPE_CHILD_CONTINUE",
            "MB_COMM_TYPE_CHILD_STOPREQ",
            "MB_COMM_TYPE_CHILD_BOOTREQ_ACCEPTED",
        };

        MB_FAKE_OUTPUT("RECV %s\n", MB_TYPE_STRING[type]);
    }
#endif
