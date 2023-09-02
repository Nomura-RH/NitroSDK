#include <nitro.h>

#include "mb_common.h"
#include "mb_wm.h"
#include "mb_child.h"
#include "mb_wm_base.h"
#include "mb_block.h"

static BOOL IsSendEnabled(void);
static void MBi_WMStateOutStartConnect(void * arg);
static void ChildStateOutStartMP(void * arg);
static void ChildPortCallback(void * arg);
static void StateOutMPSendToParent(void * arg);
static void MBi_WMStateOutStartConnect(void * arg);
static void MBi_WMStateOutEndMP(void * arg);
static void MBi_WMStateOutDisconnect(void * arg);
static void MBi_WMStateInDisconnect(void);
static void MBi_WMStateOutReset(void * arg);
static void MBi_WMSendCallback(u16 type, void * arg);
static void MBi_WMErrorCallback(u16 apiid, u16 error_code);
static void MBi_WMApiErrorCallback(u16 apiid, u16 error_code);

static MBWMWork * wmWork = NULL;

void MBi_WMSetBuffer (void * buf)
{
	SDK_NULL_ASSERT(buf);
	SDK_ASSERT(((u32)buf & 0x1f) == 0);

	wmWork = (MBWMWork *) buf;
	wmWork->start_mp_busy = 0;
	wmWork->mpStarted = 0;
	wmWork->child_bitmap = 0;
	wmWork->mpBusy = 0;
	wmWork->endReq = 0;
	wmWork->sendBuf = NULL;
	wmWork->recvBuf = NULL;
	wmWork->mpCallback = NULL;
}

void MBi_WMSetCallback (MBWMCallbackFunc callback)
{
	OSIntrMode enabled = OS_DisableInterrupts();

	wmWork->mpCallback = callback;
	(void)OS_RestoreInterrupts(enabled);
}

void MBi_WMStartConnect (WMBssDesc * bssDesc)
{
	WMErrCode result;

	wmWork->mpStarted = 0;
	wmWork->endReq = 0;

	wmWork->sendBufSize = (u16)WM_SIZE_MP_CHILD_SEND_BUFFER(bssDesc->gameInfo.childMaxSize, FALSE);
	wmWork->recvBufSize = (u16)WM_SIZE_MP_CHILD_RECEIVE_BUFFER(bssDesc->gameInfo.parentMaxSize, FALSE);
	wmWork->pSendLen = bssDesc->gameInfo.parentMaxSize;
	wmWork->pRecvLen = bssDesc->gameInfo.childMaxSize;
	wmWork->blockSizeMax = (u16)MB_COMM_CALC_BLOCK_SIZE(wmWork->pSendLen);

	MBi_SetChildMPMaxSize(wmWork->pRecvLen);

	result = WM_StartConnect(MBi_WMStateOutStartConnect, bssDesc, NULL);

	if (result != WM_ERRCODE_OPERATING) {
		MBi_WMSendCallback(MB_CALLBACK_CONNECT_FAILED, NULL);
	}
}

static void MBi_WMStateOutStartConnect (void * arg)
{
	WMStartConnectCallback * cb = (WMStartConnectCallback *)arg;

	if (cb->errcode != WM_ERRCODE_SUCCESS) {
		MBi_WMSendCallback(MB_CALLBACK_CONNECT_FAILED, arg);
		return;
	}

	switch (cb->state) {
	case WM_STATECODE_BEACON_LOST:
		break;
	case WM_STATECODE_CONNECT_START:
		break;
	case WM_STATECODE_DISCONNECTED:
		MBi_WMSendCallback(MB_CALLBACK_DISCONNECTED_FROM_PARENT, NULL);
		break;
	case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
		break;
	case WM_STATECODE_CONNECTED:
		MBi_WMSendCallback(MB_CALLBACK_CONNECTED_TO_PARENT, arg);
		break;
	}
}

void MBi_ChildStartMP (u16 * sendBuf, u16 * recvBuf)
{
	WMErrCode result;

	wmWork->sendBuf = (u32 *)sendBuf;
	wmWork->recvBuf = (u32 *)recvBuf;

	result = WM_SetPortCallback(WM_PORT_BT, ChildPortCallback, NULL);

	if (result != WM_ERRCODE_SUCCESS) {
		MBi_WMApiErrorCallback(WM_APIID_START_MP, result);
		return;
	}

	result = WM_StartMPEx(ChildStateOutStartMP, recvBuf, wmWork->recvBufSize, sendBuf, wmWork->sendBufSize, 1,
	                      0,
	                      FALSE,
	                      FALSE,
	                      TRUE,
	                      TRUE);

	if (result != WM_ERRCODE_OPERATING) {
		MBi_WMApiErrorCallback(WM_APIID_START_MP, result);
	}
}

static void ChildStateOutStartMP (void * arg)
{
	WMStartMPCallback * cb = (WMStartMPCallback *)arg;

	if (cb->errcode != WM_ERRCODE_SUCCESS) {
		if (cb->errcode == WM_ERRCODE_SEND_FAILED) {
			return;
		} else if (cb->errcode == WM_ERRCODE_TIMEOUT)   {
			return;
		} else if (cb->errcode == WM_ERRCODE_INVALID_POLLBITMAP)   {
			return;
		}

		MBi_WMErrorCallback(cb->apiid, cb->errcode);
		return;
	}

	switch (cb->state) {
	case WM_STATECODE_MP_START:
		wmWork->mpStarted = 1;
		wmWork->mpBusy = 0;
		wmWork->child_bitmap = 0;
		MBi_WMSendCallback(MB_CALLBACK_MP_STARTED, NULL);
		{
			MBi_WMSendCallback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
		}
		break;
	case WM_STATECODE_MP_IND:
		break;
	case WM_STATECODE_MPACK_IND:
		break;
	case WM_STATECODE_MPEND_IND:
	default:
		MBi_WMErrorCallback(cb->apiid, WM_ERRCODE_FAILED);
		break;
	}
}

void MBi_WMDisconnect (void)
{
	WMErrCode result;

	wmWork->endReq = 1;
	result = WM_EndMP(MBi_WMStateOutEndMP);

	if (result != WM_ERRCODE_OPERATING) {
		wmWork->endReq = 0;
		MBi_WMApiErrorCallback(WM_APIID_END_MP, result);
	}
}

static void MBi_WMStateOutEndMP (void * arg)
{
	WMCallback * cb = (WMCallback *)arg;

	if (cb->errcode != WM_ERRCODE_SUCCESS) {
		wmWork->endReq = 0;
		MBi_WMErrorCallback(cb->apiid, cb->errcode);
		return;
	}

	wmWork->mpStarted = 0;
	MBi_WMStateInDisconnect();
}

static void MBi_WMStateInDisconnect (void)
{
	WMErrCode result;

	result = WM_Disconnect(MBi_WMStateOutDisconnect, 0);

	if (result != WM_ERRCODE_OPERATING) {
		wmWork->endReq = 0;
		MBi_WMApiErrorCallback(WM_APIID_DISCONNECT, result);
	}
}

static void MBi_WMStateOutDisconnect (void * arg)
{
	WMCallback * cb = (WMCallback *)arg;

	wmWork->endReq = 0;
	if (cb->errcode != WM_ERRCODE_SUCCESS) {
		MBi_WMErrorCallback(cb->apiid, cb->errcode);
		return;
	}

	MBi_WMSendCallback(MB_CALLBACK_DISCONNECT_COMPLETE, NULL);
}

void MBi_WMReset (void)
{
	WMErrCode result;

	result = WM_Reset(MBi_WMStateOutReset);
	if (result != WM_ERRCODE_OPERATING) {
		MBi_WMApiErrorCallback(WM_APIID_RESET, result);
	}
}

static void MBi_WMStateOutReset (void * arg)
{
	WMCallback * cb = (WMCallback *)arg;

	if (cb->errcode != WM_ERRCODE_SUCCESS) {
		MBi_WMErrorCallback(cb->apiid, cb->errcode);
		return;
	}

	MBi_WMSendCallback(MB_CALLBACK_DISCONNECT_COMPLETE, NULL);
}

static BOOL IsSendEnabled (void)
{
	return (wmWork->mpStarted == 1) && (wmWork->mpBusy == 0) && (wmWork->endReq == 0);
}

static void ChildPortCallback (void * arg)
{
	WMPortRecvCallback * cb = (WMPortRecvCallback *)arg;

	if (cb->errcode != WM_ERRCODE_SUCCESS) {
		return;
	}

	switch (cb->state) {
	case WM_STATECODE_PORT_RECV:
		MBi_WMSendCallback(MB_CALLBACK_MP_CHILD_RECV, cb);
		break;
	case WM_STATECODE_CONNECTED:
		break;
	case WM_STATECODE_PORT_INIT:
	case WM_STATECODE_DISCONNECTED:
	case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
		break;
	}
}

WMErrCode MBi_MPSendToParent (u32 body_len, u16 pollbmp, u32 * sendbuf)
{
	WMErrCode result;

	SDK_ASSERT(((u32)sendbuf & 0x1F) == 0);

	DC_FlushRange(sendbuf, sizeof(body_len));
	DC_WaitWriteBufferEmpty();

	if (!IsSendEnabled()) {
		return WM_ERRCODE_FAILED;
	}

	result = WM_SetMPDataToPort(StateOutMPSendToParent,
	                            (u16 *)sendbuf,
	                            (u16)body_len, pollbmp, WM_PORT_BT, WM_PRIORITY_LOW);
	if (result != WM_ERRCODE_OPERATING) {
		return result;
	}

	wmWork->mpBusy = 1;
	return WM_ERRCODE_OPERATING;
}

static void StateOutMPSendToParent (void * arg)
{
	WMCallback * cb = (WMCallback *)arg;

	wmWork->mpBusy = 0;

	if (cb->errcode == WM_ERRCODE_SUCCESS) {
		MBi_WMSendCallback(MB_CALLBACK_MP_CHILD_SENT, arg);
	} else if (cb->errcode == WM_ERRCODE_TIMEOUT)   {
		MBi_WMSendCallback(MB_CALLBACK_MP_CHILD_SENT_TIMEOUT, arg);
	} else {
		MBi_WMSendCallback(MB_CALLBACK_MP_CHILD_SENT_ERR, arg);
	}

	MBi_WMSendCallback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
}

static inline void MBi_WMSendCallback (u16 type, void * arg)
{
	if (wmWork->mpCallback == NULL) {
		return;
	}

	wmWork->mpCallback(type, arg);
}

static inline void MBi_WMErrorCallback (u16 apiid, u16 error_code)
{
	MBErrorCallback arg;

	if (wmWork->mpCallback == NULL) {
		return;
	}

	arg.apiid = apiid;
	arg.errcode = error_code;

	wmWork->mpCallback(MB_CALLBACK_ERROR, &arg);
}

static inline void MBi_WMApiErrorCallback (u16 apiid, u16 error_code)
{
	MBErrorCallback arg;

	if (wmWork->mpCallback == NULL) {
		return;
	}

	arg.apiid = apiid;
	arg.errcode = error_code;

	wmWork->mpCallback(MB_CALLBACK_API_ERROR, &arg);
}

void MBi_WMClearCallback (void)
{
	(void)WM_SetPortCallback(WM_PORT_BT, NULL, NULL);
}
