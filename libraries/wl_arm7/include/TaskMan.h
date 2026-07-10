#ifndef __TASKMAN_H_
#define __TASKMAN_H_

#define MLME_SCAN_TASK_ID         0
#define MLME_JOIN_TASK_ID         1
#define MLME_AUTH_TASK_ID         2
#define MLME_ASS_TASK_ID          3
#define MLME_REASS_TASK_ID        4
#define MLME_MEASCHAN_TASK_ID     5
#define RXDATA_TASK_ID            6
#define RXMANCTRL_TASK_ID         7
#define INTR_TXBEACON_TASK_ID     8
#define DEFRAG_TASK_ID            9
#define CAM_TIMER_TASK_ID         10
#define REQUEST_CMD_TASK_ID       11
#define LOWEST_IDLE_TASK_ID       12
#define MLME_BEACON_LOST_TASK_ID  13
#define INTR_TXEND_TASK_ID        14
#define INTR_RXEND_TASK_ID        15
#define INTR_MPEND_TASK_ID        16
#define DEFRAG_TIMER_TASK_ID      17
#define UPDATE_APLIST_TASK_ID     18
#define SEND_MSG_TASK_ID          19
#define PARENT_TBTT_TXQ_TASK_ID   20
#define SEND_FATALERR_MSG_TASK_ID 21
#define TERMINATE_WL_TASK_ID      22
#define RELEASE_WL_TASK_ID        23

// #define	TASK_NUM					8
#define TASK_NUM 24

#define TASK_CRITICAL_PRIORITY 0
#define TASK_HIGH_PRIORITY     1
#define TASK_NORMAL_PRIORITY   2
#define TASK_LOW_PRIORITY      3
#define TASK_MAX_PRIORITY      4

typedef struct {
    u16 NextId;
    u16 Flag;
    void (*pTaskFunc)(void);
} TASK_TBL, *LPTASK_TBL;

typedef struct {
    u16 EnQ[TASK_MAX_PRIORITY];
    u16 DeQ[TASK_MAX_PRIORITY];

    vu16 NextPri;
    u16 TaskPri;
    u16 CurrTaskID;
    u16 pad;

    OSThread Thread;

    TASK_TBL TaskTbl[TASK_NUM];
} TASK_MAN, *LPTASK_MAN;

void InitializeTask(void);
void AddTask(s32 nPriority, u32 nTaskID);
u32 DeleteTask(u32 nPriority);

void MainTaskRoutine(void *arg);
void DispTaskQueueList(void);

#define TASK_FREE     0x0000
#define TASK_RESERVED 0x0001
#define TASK_LAST     0xFFFF

#ifdef __TASKMAN_C_

void LowestIdleTask(void);
void ExecuteMessage(OSMessage *pMsg);

#endif // __TASKMAN_C_
#endif // __TASKMAN_H_
