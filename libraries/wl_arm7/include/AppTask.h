#ifndef __APPTASK_H_
#define __APPTASK_H_

#define SCMD_TASK_ID                 0
#define DummyARM9IO_TASK_ID          1
#define HOSTIF_TASK_ID               2
#define CDMPRCES_TASK_ID             3
#define CMDP_WMANAGER_TASK_ID        4
#define WM_PARENT_MAIN_TASK_ID       5
#define WM_CHILD_MAIN_TASK_ID        6
#define WM_HOTSPOT_MAIN_TASK_ID      7
#define WM_INSPECT_MAIN_TASK_ID      8
#define WM_TST_MAIN_TASK_ID          9
#define WM_TST_MLME_INDICATE_TASK_ID 10
#define WM_TST_MA_INDICATE_TASK_ID   11
#define TX_TSET_TASK_ID              12
#define TX_TSET_RXEND_TASK_ID        13
#define WM_TST_MORE_RECV_MSG_TASK_ID 14
#define SET_NOISE_TASK_ID            15

#define MAC_TEST_TASK_ID      16
#define MAC_TEST_WAIT_TASK_ID 17

#define WL_TEST_TASK_ID      16
#define WL_TEST_WAIT_TASK_ID 17

#define TP_TASK_ID 19

#define APP_TASK_NUM 20

#define APP_TASK_CRITICAL_PRIORITY 0
#define APP_TASK_HIGH_PRIORITY     1
#define APP_TASK_NORMAL_PRIORITY   2
#define APP_TASK_LOW_PRIORITY      3
#define APP_TASK_MAX_PRIORITY      4

typedef struct {
    u16 NextId;
    u16 Flag;
    void (*pTaskFunc)(void);
} APP_TASK_TBL, *LPAPP_TASK_TBL;

typedef struct {
    u16 EnQ[APP_TASK_MAX_PRIORITY];
    u16 DeQ[APP_TASK_MAX_PRIORITY];

    vu16 NextPri;
    u16 TaskPri;
    u16 CurrTaskID;
    u16 pad;

    void (*pRecvMsgFunc)(WlCmdReq *);
} APP_TASK_MAN, *LPAPP_TASK_MAN;

void App_InitializeTask(void);

void App_AddTask(s32 nPriority, u32 nTaskID);

void App_MainTaskRoutine(void);
void App_DispTaskQueueList(void);
void App_ExecOneTask(void);

void App_SetRecvMsgFunction(void (*pFunc)(WlCmdReq *));

#ifdef __APPTASK_C_

u32 App_DeleteTask(u32 nPriority);
void App_ExecuteMessage(OSMessage *pMsg);

#define TASK_FREE     0x0000
#define TASK_RESERVED 0x0001
#define TASK_LAST     0xFFFF

#endif // __TASKMAN_C_
#endif // __TASKMAN_H_
