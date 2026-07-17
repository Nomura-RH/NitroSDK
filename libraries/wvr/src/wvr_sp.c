#include "nitro/os/common/printf.h"
#include "nitro/wm/ARM7/wm_sp.h"
#include <nitro/spi/ARM7/pm.h>
#include <nitro/spi/common/pm_common.h>
#include <nitro/mi/memory.h>
#include <nitro/pxi/common/fifo.h>
#include <nitro/os/common/alloc.h>
#include <nitro/ctrdg/ARM7/ctrdg_sp.h>
#include <nitro/os.h>

#include "wmsp_private.h"

#define     THREAD_PRIO_WM_IND_HIGH         3
#define     THREAD_PRIO_WL_HIGH             4
#define     THREAD_PRIO_WM_REQ_HIGH         5
#define     THREAD_PRIO_WM_IND_LOW          7
#define     THREAD_PRIO_WL_LOW              8
#define     THREAD_PRIO_WM_REQ_LOW          9

#define     WVR_STATUS_BEFORE_INIT          0
#define     WVR_STATUS_READY                1
#define     WVR_STATUS_LOADING              2
#define     WVR_STATUS_DRIVING              3
#define     WVR_STATUS_UNLOADING            4

#define     WVR_THREAD_STACK_SIZE           1024
#define     WVR_THREAD_PRIO_LOW             10
#define     WVR_THREAD_PRIO_HIGH            2

static void *wvrVramImageBuf;
static u32 wvrVramImageBufSize;
static OSHeapHandle wvrHeapHandle;
static u8 wvrStatus;
static u32 wvrThreadStack[WVR_THREAD_STACK_SIZE / 4];
static OSThread wvrThread;

static u32 wvrWlWork[WM_WL_WORK_SIZE / 4];
static u32 wvrWlStack[WM_WL_STACK_SIZE / 4];
static WlStaElement wvrWlStaElement[WM_WL_CAMTABLE_NUM];

static void WvrBegin(OSHeapHandle handle);
static void WvrLoad(void);
static void WvrUnload(void);
static void WvrPxiWmSubstituteCallback(PXIFifoTag tag, u32 data, BOOL err);
static void WvrPxiReceiveCallback(PXIFifoTag tag, u32 data, BOOL err);
static BOOL WvrCheckThreadRunning(OSThread *pThread);
static BOOL WvrStartUp(void);
static void WvrStartUpThread(void *arg);
static BOOL WvrTerminate(void);
static void WvrTerminateThread(void *arg);

void WVR_Begin(OSHeapHandle handle)
{
    wvrVramImageBuf = NULL;
    wvrHeapHandle = handle;
    MI_CpuClear8(&wvrThread, sizeof(OSThread));

    WvrBegin(handle);

    wvrStatus = WVR_STATUS_DRIVING;
}

void WVR_Init(OSHeapHandle handle)
{
    wvrHeapHandle = handle;
    wvrStatus = WVR_STATUS_READY;
    MI_CpuClear8(&wvrThread, sizeof(OSThread));

    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_WVR, WvrPxiReceiveCallback);
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_WM, WvrPxiWmSubstituteCallback);
}

void WVR_Shutdown(void)
{
    reg_SND_POWCNT &= ~(REG_SND_POWCNT_EWL_MASK);

    PM_SetLEDPattern(WMSP_LED_BLINK_DISABLE);
    PMi_SetLED((PMLEDStatus)WMSP_LED_BLINK_DISABLE);
}

static void WvrBegin(OSHeapHandle handle)
{
    WlInit  wlInit;
    WmInit  wmInit;

    wlInit.workingMemAdrs = (u32)wvrWlWork;
    wlInit.stack = &wvrWlStack[WM_WL_STACK_SIZE / 4];
    wlInit.stacksize = WM_WL_STACK_SIZE;
    wlInit.priority = THREAD_PRIO_WL_HIGH;
    wlInit.heapType = WL_HEAP_OS;
    wlInit.heapFunc.os.id = OS_ARENA_WRAM_SUBPRIV;
    wlInit.heapFunc.os.heapHandle = handle;
    wlInit.camAdrs = wvrWlStaElement;
    wlInit.camSize = sizeof(wvrWlStaElement);

    wmInit.dmaNo = WM_SP_WM_DMA_NO;
    wlInit.dmaMaxSize = WM_DMA_MAX_SIZE;

    wmInit.indPrio_high = THREAD_PRIO_WM_IND_HIGH;
    wmInit.wlPrio_high = THREAD_PRIO_WL_HIGH;
    wmInit.reqPrio_high = THREAD_PRIO_WM_REQ_HIGH;
    wmInit.indPrio_low = THREAD_PRIO_WM_IND_LOW;
    wmInit.wlPrio_low = THREAD_PRIO_WL_LOW;
    wmInit.reqPrio_low = THREAD_PRIO_WM_REQ_LOW;

    WM_sp_init(&wlInit, &wmInit);
}

static void WvrLoad(void)
{
    if (wvrVramImageBuf == NULL) {
        return;
    }

    MI_CpuCopyFast(wvrVramImageBuf, (void *)HW_EXT_WRAM, wvrVramImageBufSize);
}

static void WvrUnload(void)
{
    if (wvrVramImageBuf == NULL) {
        return;
    }

    MI_CpuClearFast((void *)HW_EXT_WRAM, wvrVramImageBufSize);
}

static void WvrPxiWmSubstituteCallback(PXIFifoTag tag, u32 data, BOOL err)
{
    WMCallback *cb = (WMCallback *)data;
    u16     apiid = *((u16 *)data);

    if (cb == NULL) {
        return;
    }
    MI_CpuClear32((void *)cb, WM_FIFO_BUF_SIZE);
    cb->apiid = apiid;
    cb->errcode = WM_ERRCODE_WM_DISABLE;
    while (0 > PXI_SendWordByFifo(PXI_FIFO_TAG_WM, (u32)cb, FALSE));
}

static BOOL WvrCheckThreadRunning(OSThread *pThread)
{
    OSThread *list;

    if (pThread == NULL) {
        return FALSE;
    }

    list = OSi_ThreadInfo.list;
    while (list) {
        if (list == pThread) {
            switch (pThread->state) {
            case OS_THREAD_STATE_WAITING:
            case OS_THREAD_STATE_READY:
                return TRUE;
            case OS_THREAD_STATE_TERMINATED:
            default:
                return FALSE;
            }
        }
        list = list->next;
    }
    return FALSE;
}

static void WvrPxiReceiveCallback(PXIFifoTag tag, u32 data, BOOL err)
{
    u32     resData = 0;

    switch (data) {
    case WVR_PXI_COMMAND_STARTUP:
        if (WvrStartUp()) {
            return;
        }
        resData = data | WVR_RESULT_ILLEGAL_STATUS;
        break;
    case WVR_PXI_COMMAND_TERMINATE:
        if (WvrTerminate()){
            return;
        }
        resData = data | WVR_RESULT_ILLEGAL_STATUS;
        break;
    default:
        resData = data | WVR_RESULT_FATAL_ERROR;
    }

    while (0 > PXI_SendWordByFifo(PXI_FIFO_TAG_WVR, resData, FALSE));
}

static BOOL WvrStartUp(void)
{
    OSIntrMode e = OS_DisableInterrupts();

    if (WvrCheckThreadRunning(&wvrThread)) {
        (void)OS_RestoreInterrupts(e);
        return FALSE;
    }
    if (wvrStatus != WVR_STATUS_READY) {
        (void)OS_RestoreInterrupts(e);
        return FALSE;
    }
    wvrStatus = WVR_STATUS_LOADING;
    (void)OS_RestoreInterrupts(e);

    OS_CreateThread(&wvrThread,
                    WvrStartUpThread,
                    NULL,
                    (void *)(wvrThreadStack + (WVR_THREAD_STACK_SIZE / sizeof(u32))),
                    WVR_THREAD_STACK_SIZE, WVR_THREAD_PRIO_LOW);
    OS_WakeupThreadDirect(&wvrThread);

    return TRUE;
}

static void WvrStartUpThread(void *arg)
{
    OSIntrMode e;

    WvrLoad();
    WvrBegin(wvrHeapHandle);
    while (0 > PXI_SendWordByFifo(PXI_FIFO_TAG_WVR, WVR_PXI_COMMAND_STARTUP | WVR_RESULT_SUCCESS, FALSE));
    e = OS_DisableInterrupts();
    wvrStatus = WVR_STATUS_DRIVING;
    OS_ExitThread();
}

static BOOL WvrTerminate(void)
{
    OSIntrMode e = OS_DisableInterrupts();

    if (WvrCheckThreadRunning(&wvrThread)) {
        (void)OS_RestoreInterrupts(e);
        return FALSE;
    }
    if (wvrStatus != WVR_STATUS_DRIVING) {
        (void)OS_RestoreInterrupts(e);
        return FALSE;
    }
    wvrStatus = WVR_STATUS_UNLOADING;
    (void)OS_RestoreInterrupts(e);

    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_WM, WvrPxiWmSubstituteCallback);

    OS_CreateThread(&wvrThread,
                    WvrTerminateThread,
                    NULL,
                    (void *)(wvrThreadStack + (WVR_THREAD_STACK_SIZE / sizeof(u32))),
                    WVR_THREAD_STACK_SIZE, WVR_THREAD_PRIO_HIGH);
    OS_WakeupThreadDirect(&wvrThread);

    return TRUE;
}

static void WvrTerminateThread(void *arg)
{
    OSIntrMode e;
    WMSPWork *p = WMSP_GetSystemWork();
    OSMessage msg;

    if (p->wm7buf != NULL) {
        if (p->wm7buf->status->state != WM_STATE_READY) {
            WMSP_CancelVAlarm();
            WMSP_CancelAllAlarms();
        }
    }

    while (TRUE) {
        if (FALSE == OS_ReceiveMessage(&(p->requestQ), &msg, OS_MESSAGE_NOBLOCK)) {
            break;
        }
        if (p->wm7buf != NULL) {
            WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
            MI_CpuClear32((void *)cb, WM_FIFO_BUF_SIZE);
            cb->apiid = *((u16 *)msg);
            cb->errcode = WM_ERRCODE_WM_DISABLE;
            WMSP_ReturnResult2Wm9((void *)cb);
            *((u16 *)msg) |= WM_API_REQUEST_ACCEPTED;
        }
    }

    if (WvrCheckThreadRunning(WMSP_GetRequestThread())) {
        (void)OS_SendMessage(&(p->requestQ), (OSMessage)0, OS_MESSAGE_BLOCK);
        OS_JoinThread(WMSP_GetRequestThread());
    }

    {
        BOOL    result[3];

        result[0] = OS_ReceiveMessage(&(p->requestQ), &msg, OS_MESSAGE_NOBLOCK);
        result[1] = OS_ReceiveMessage(&(p->confirmQ), &msg, OS_MESSAGE_NOBLOCK);
        result[2] = OS_ReceiveMessage(&(p->toWLmsgQ), &msg, OS_MESSAGE_NOBLOCK);
        if ((result[0] | result[1] | result[2]) != FALSE) {
            OS_Terminate();
        }
    }

    WL_Terminate();

    while (OS_ReceiveMessage(&(p->fromWLmsgQ), &msg, OS_MESSAGE_NOBLOCK));

    if (WvrCheckThreadRunning(WMSP_GetIndicateThread())) {
        (void)OS_SendMessage(&(p->fromWLmsgQ), (OSMessage)0, OS_MESSAGE_BLOCK);
        OS_JoinThread(WMSP_GetIndicateThread());
    }

    PM_SetLEDPattern(WMSP_LED_BLINK_DISABLE);
    PMi_SetLED((PMLEDStatus)WMSP_LED_BLINK_DISABLE);

    if (p->wm7buf != NULL) {
        if (p->wm7buf->status->state != WM_STATE_READY) {
            p->wm7buf->status->state = WM_STATE_READY;
        }
    }

    WvrUnload();

    while (0 > PXI_SendWordByFifo(PXI_FIFO_TAG_WVR, WVR_PXI_COMMAND_TERMINATE | WVR_RESULT_SUCCESS, FALSE));
    e = OS_DisableInterrupts();
    wvrStatus = WVR_STATUS_READY;
    OS_ExitThread();
}

