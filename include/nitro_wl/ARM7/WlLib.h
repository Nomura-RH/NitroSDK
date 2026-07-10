#ifndef	__WLLIB_H_
#define	__WLLIB_H_

#include "WlBuf.h"
#include "WlFrame.h"
#include "WlCmd.h"
#include "WlCmdLabel.h"
#include "WlStaList.h"
#include "WlParam.h"

typedef	struct {
	u32					workingMemAdrs;
	void*				stack;
	u32					stacksize;
	u32					priority;
	OSMessageQueue*		sendMsgQueuep;
	OSMessageQueue*		recvMsgQueuep;
	u32					dmaChannel;
	u32					dmaMaxSize;
	u32					heapType;
	union
	{
		struct
		{
			OSArenaId		id;
			OSHeapHandle	heapHandle;
		} os;
		struct
		{
			u32	(*alloc)(u32 size);
			u32	(*free)(void* p);
		} ext;
	} heapFunc;

	void*				camAdrs;
	u32					camSize;
} WlInit;

u32       WL_InitDriver(WlInit* init);
OSThread* WL_GetThreadStruct(void);
void	  WL_Terminate(void);

#define	HW_WIRELESS_INTF0		0x04800000
#define	HW_WIRELESS_INTF1		0x04808000
#ifndef	MAC_REG_BASE
#define	MAC_REG_BASE			(HW_WIRELESS_INTF1)
#endif

#define	WL_WORKADRS_BUF_ADRS	(HW_PRV_WRAM_SYSRV + 0x34)
#define	WL_VTSF_ADRS			(HW_PRV_WRAM_SYSRV + 0x30)

#define	V_TSF					(WL_VTSF_ADRS)
#define	V_TSF_L					(WL_VTSF_ADRS)
#define	V_TSF_H					(WL_VTSF_ADRS+1)


#endif	// __WLLIB_H_

