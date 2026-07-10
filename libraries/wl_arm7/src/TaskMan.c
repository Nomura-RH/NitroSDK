#define __TASKMAN_C_
#define __INSYSROM__

#include "WlSys.h"
#include "WlLib.h"

#include "TaskMan.h"
#include "Param.h"
#include "TxCtrl.h"
#include "RxCtrl.h"
#include "CAM.h"
#include "WlCmdIf.h"
#include "WlIntrTask.h"

#include "MAC.h"

static void (*const pTaskFunc[])(void) = {
    MLME_ScanTask,
    MLME_JoinTask,
    MLME_AuthTask,
    MLME_AssTask,
    MLME_ReAssTask,
    MLME_MeasChannelTask,
    RxDataFrameTask,
    RxManCtrlTask,
    WlIntrTxBeaconTask,
    DefragTask,
    CAM_TimerTask,
    RequestCmdTask,
    LowestIdleTask,
    MLME_BeaconLostTask,
    WlIntrTxEndTask,
    WlIntrRxEndTask,
    WlIntrMpEndTask,
    DefragTimerTask,
    UpdateApListTask,
    SendMessageToWmTask,
    SetParentTbttTxqTask,
    SendFatalErrMsgTask,
    TerminateWlTask,
    ReleaseWlTask,
};

void InitializeTask(void)
{
    LPTASK_MAN pTaskMan = &wlMan->TaskMan;
    u32 i;

    DbgPuts("Init Task\r");

    pTaskMan->NextPri = 0;
    pTaskMan->TaskPri = 0;

    for (i = 0; i < TASK_MAX_PRIORITY; i++) {
        pTaskMan->EnQ[i] = pTaskMan->DeQ[i] = TASK_LAST;
    }

    for (i = 0; i < sizeof(pTaskFunc) / 4; i++) {
        pTaskMan->TaskTbl[i].NextId = TASK_LAST;
        pTaskMan->TaskTbl[i].Flag = TASK_FREE;
        pTaskMan->TaskTbl[i].pTaskFunc = pTaskFunc[i];
    }

    AddTask(TASK_LOW_PRIORITY, LOWEST_IDLE_TASK_ID);
}

void MainTaskRoutine(void *arg)
{
#pragma unused(arg)
    LPTASK_MAN pTaskMan = &wlMan->TaskMan;
    u32 x;

    pTaskMan->NextPri = 0;
    pTaskMan->CurrTaskID = 0;

    DbgPrint("Startup MainTaskRoutine\r\n");

    while (1) {
        OSMessage msg;

        if (OS_ReceiveMessage(wlMan->pRecvMsgQueue, &msg, OS_MESSAGE_NOBLOCK)) {
            ExecuteMessage(&msg);
        }

        x = OS_DisableIrqMask(OS_IE_WIRELESS | OS_IE_TIMER1);

        pTaskMan->TaskPri = pTaskMan->NextPri;

        if (pTaskMan->EnQ[pTaskMan->TaskPri] == TASK_LAST) {
            ++pTaskMan->NextPri;
            OS_EnableIrqMask(x);
        } else {
            OS_EnableIrqMask(x);

            pTaskMan->CurrTaskID = (u16)DeleteTask(pTaskMan->TaskPri);

#ifdef SDK_TEG
            x = OS_DisableIrqMask(OS_IE_WIRELESS);
            wlMan->bUnLocked = FALSE;
            OS_LockCartridge(wlMan->lockID);
            OS_EnableIrqMask(x);
#endif

            DbgSetDDO(DDO_WL_TASK, (pTaskMan->CurrTaskID + 1));

            (*pTaskMan->TaskTbl[pTaskMan->CurrTaskID].pTaskFunc)();

            DbgClrDDO(DDO_WL_TASK, (pTaskMan->CurrTaskID + 1));

#ifdef SDK_TEG
            if (wlMan->bUnLocked == FALSE) {
                x = OS_DisableIrqMask(OS_IE_WIRELESS);
                OS_UnLockCartridge(wlMan->lockID);
                wlMan->bUnLocked = TRUE;
                OS_EnableIrqMask(x);
            }
#endif

            pTaskMan->CurrTaskID = TASK_LAST;
        }
    }
}

void AddTask(s32 nPriority, u32 nTaskID)
{
    LPTASK_MAN pTaskMan = &wlMan->TaskMan;
    LPTASK_TBL pTaskTbl;
    u32 x;

    ASSERT(nPriority >= TASK_MAX_PRIORITY);

    pTaskTbl = pTaskMan->TaskTbl;

    x = OS_DisableIrqMask(OS_IE_WIRELESS | OS_IE_TIMER1);

    if (pTaskTbl[nTaskID].Flag == TASK_FREE) {
        pTaskTbl[nTaskID].Flag = TASK_RESERVED;

        pTaskTbl[nTaskID].NextId = TASK_LAST;

        if (pTaskMan->DeQ[nPriority] == TASK_LAST) {
            pTaskMan->EnQ[nPriority] = (u16)nTaskID;
        } else {
            pTaskMan->TaskTbl[pTaskMan->DeQ[nPriority]].NextId = (u16)nTaskID;
        }

        pTaskMan->DeQ[nPriority] = (u16)nTaskID;

        if (nPriority < pTaskMan->NextPri) {
            pTaskMan->NextPri = (u16)nPriority;
        }
    }

    OS_EnableIrqMask(x);

    if ((nPriority != TASK_LOW_PRIORITY) && (pTaskMan->TaskPri == TASK_LOW_PRIORITY)) {
        OS_SendMessage(wlMan->pRecvMsgQueue, NULL, OS_MESSAGE_NOBLOCK);
    }
}

u32 DeleteTask(u32 nPriority)
{
    LPTASK_MAN pTaskMan = &wlMan->TaskMan;
    LPTASK_TBL pTaskTbl;
    u32 x, nTaskID, nOfstTaskID;

    x = OS_DisableIrqMask(OS_IE_WIRELESS | OS_IE_TIMER1);

    if ((nTaskID = pTaskMan->EnQ[nPriority]) != TASK_LAST) {
        pTaskTbl = pTaskMan->TaskTbl;
        nOfstTaskID = nTaskID;

        pTaskTbl[nOfstTaskID].Flag = TASK_FREE;

        if (pTaskTbl[nOfstTaskID].NextId == TASK_LAST) {
            pTaskMan->EnQ[nPriority] = TASK_LAST;
            pTaskMan->DeQ[nPriority] = TASK_LAST;
        } else {
            pTaskMan->EnQ[nPriority] = pTaskTbl[nOfstTaskID].NextId;
            pTaskTbl[nOfstTaskID].NextId = TASK_LAST;
        }
    }

    OS_EnableIrqMask(x);

    return nTaskID;
}

void LowestIdleTask(void)
{
    OSMessage msg;

#ifdef SDK_TEG
    {
        BOOL y = OS_DisableIrqMask(OS_IE_WIRELESS);
        OS_UnLockCartridge(wlMan->lockID);
        wlMan->bUnLocked = TRUE;
        OS_EnableIrqMask(y);
    }
#endif
    OS_ReceiveMessage(wlMan->pRecvMsgQueue, &msg, OS_MESSAGE_BLOCK);

    ExecuteMessage(&msg);

    AddTask(TASK_LOW_PRIORITY, LOWEST_IDLE_TASK_ID);
}

void ExecuteMessage(OSMessage *pMsg)
{
    if (*pMsg != NULL) {
        WlCmdReq *pReq = (WlCmdReq *)*pMsg;

        NewHeapBuf(&wlMan->HeapMan.RequestCmd, *pMsg);
        AddTask(TASK_NORMAL_PRIORITY, REQUEST_CMD_TASK_ID);
    }
}
