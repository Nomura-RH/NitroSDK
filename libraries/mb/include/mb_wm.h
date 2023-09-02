#ifndef _MB_WM_H_
#define _MB_WM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nitro/types.h>
#include <nitro/mb.h>
#include <nitro/wm.h>

typedef void (*MBWMCallbackFunc) (u16 type, void * arg);

typedef struct {
	MBWMCallbackFunc mpCallback;
	u32 * sendBuf;
	u32 * recvBuf;
	BOOL start_mp_busy;
	u16 sendBufSize;
	u16 recvBufSize;
	u16 pSendLen;
	u16 pRecvLen;
	u16 blockSizeMax;
	u16 mpStarted;
	u16 endReq;
	u16 child_bitmap;
	u16 mpBusy;
	u8 _padding[2];
} MBWMWork;

void MBi_WMSetBuffer(void * buf);
void MBi_WMSetCallback(MBWMCallbackFunc callback);
void MBi_WMStartConnect(WMBssDesc * bssDesc);
void MBi_ChildStartMP(u16 * sendBuf, u16 * recvBuf);
WMErrCode MBi_MPSendToParent(u32 body_len, u16 pollbmp, u32 * sendbuf);
void MBi_WMDisconnect(void);
void MBi_WMReset(void);
void MBi_WMClearCallback(void);

#ifdef __cplusplus
}
#endif

#endif
