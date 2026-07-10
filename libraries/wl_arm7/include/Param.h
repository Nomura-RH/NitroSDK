#ifndef __PARAM_H_
#define __PARAM_H_

#include "TaskMan.h"
#include "BufMan.h"

#include "WlNic.h"
#include "MLME.h"
#include "Dot11Frm.h"
#include "WlCmdIf.h"
#include "TxCtrl.h"
#include "RxCtrl.h"
#include "ApList.h"

typedef struct {
    TASK_MAN TaskMan;
    HEAP_MAN HeapMan;
    AP_LIST ApList[MAX_APLIST_NUM];
    OSMessageQueue *pSendMsgQueue;
    OSMessageQueue *pRecvMsgQueue;
    u32 DmaChannel;
    u32 DmaMaxCount;
    s32 lockID;
    void *pFlashImg;

    CONFIG_PARAM Config;
    WORK_PARAM Work;
    MLME_MAN MLME;
    CMDIF_MAN CmdIf;
    TX_CTRL TxCtrl;
    RX_CTRL RxCtrl;
    CAM_MAN CamMan;
    WlCounter Counter;
    RAND_CTRL Rand;
    RF_CONFIG Rf;
    OSAlarm PeriodicAlarm;
    OSAlarm Alarm;
    OSAlarm KeyAlarm;

    u32 WlDbgLevel;
    u16 WlOperation;
    u16 EnableChannel;
} WL_MAN, *LPWL_MAN;

#define wlMan ((LPWL_MAN)(*(u32 *)WL_WORKADRS_BUF_ADRS))
// #define	wlMan			((LPWL_MAN)(WL_WORKING_MEM_ADRS))

#endif // __PARAM_H_
