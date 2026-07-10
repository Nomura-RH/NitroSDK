#include "nitro/spi/ARM7/pm.h"
#include "nitro/spi/common/pm_common.h"
#include "wmsp_private.h"

void WMSPi_CommonInit(u32 param1);

void WMSP_Enable(OSMessage msg)
{
    u32 *reqBuf = (u32 *)msg;
    WMCallback *cb;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMArm7Buf *p;

    sys->wm7buf = (WMArm7Buf *)(reqBuf[1]);
    p = sys->wm7buf;
    p->status = sys->status = (WMStatus *)(reqBuf[2]);
    p->fifo7to9 = (u32 *)(reqBuf[3]);

    WMSPi_CommonInit(reqBuf[4]);

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_ENABLE;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);
}

void WMSPi_CommonInit(u32 param1)
{
    WMArm7Buf *p = WMSP_GetSystemWork()->wm7buf;
    WMStatus *status = WMSP_GetStatusStructure();

#ifdef WM_DEBUG_HEAP
    WMSP_CheckWLHeap("WMSPi_CommonInit");
#endif

    {
        BOOL fCleanQueue = FALSE;
        OSIntrMode e = OS_DisableInterrupts();

        if (status->mp_flag == TRUE) {
            status->mp_flag = FALSE;
            fCleanQueue = TRUE;
            WMSP_CancelVAlarm();

            WMSP_SetThreadPriorityLow();
        }
        status->child_bitmap = 0;
        status->mp_readyBitmap = 0;
        status->ks_flag = FALSE;
        status->dcf_flag = FALSE;
        status->VSyncFlag = FALSE;
        status->valarm_queuedFlag = FALSE;
        status->beaconIndicateFlag = 0;

        status->mp_minFreq = 1;
        status->mp_freq = 1;
        status->mp_maxFreq = WM_DEFAULT_MP_FREQ_LIMIT;
        status->mp_defaultRetryCount = 0;
        status->mp_minPollBmpMode = FALSE;
        status->mp_singlePacketMode = FALSE;
        status->mp_ignoreFatalErrorMode = FALSE;
        status->mp_ignoreSizePrecheckMode = FALSE;

        status->mp_current_minFreq = status->mp_minFreq;
        status->mp_current_freq = status->mp_freq;
        status->mp_current_maxFreq = status->mp_maxFreq;
        status->mp_current_defaultRetryCount = status->mp_defaultRetryCount;
        status->mp_current_minPollBmpMode = status->mp_minPollBmpMode;
        status->mp_current_singlePacketMode = status->mp_singlePacketMode;
        status->mp_current_ignoreFatalErrorMode = status->mp_ignoreFatalErrorMode;

        status->wep_flag = FALSE;
        status->wepMode = WL_CMDLABEL_WEP_NO;
        MI_CpuClear8(status->wepKey, WM_SIZE_WEPKEY);

        WMSP_ResetSizeVars();

        status->mp_parentVCount = WM_VALARM_COUNT_PARENT_MP;
        status->mp_childVCount = WM_VALARM_COUNT_CHILD_MP;
        status->mp_parentInterval = WM_DEFAULT_MP_PARENT_INTERVAL;
        status->mp_childInterval = WM_DEFAULT_MP_CHILD_INTERVAL;
        status->mp_parentIntervalTick = OS_MicroSecondsToTicks(WM_DEFAULT_MP_PARENT_INTERVAL);
        status->mp_childIntervalTick = OS_MicroSecondsToTicks(WM_DEFAULT_MP_CHILD_INTERVAL);

        status->pwrMgtMode = 0;
        status->preamble = WL_CMDLABEL_PREAMBLE_SHORT;

        status->miscFlags = param1;

        (void)OS_RestoreInterrupts(e);

#ifdef WM_CLEAN_SEND_QUEUE
        if (fCleanQueue) {
            WMSP_CleanSendQueue(0xffff);
        }
#endif
    }

    {
        s32 i;

        for (i = 0; i < 32; i++) {
            p->requestBuf[i * 4] = WM_API_REQUEST_ACCEPTED;
        }
    }

    MI_CpuFill16(status->portSeqNo, 0x0001, sizeof(status->portSeqNo)
    );

    WMSP_InitAlarm();

    OS_InitMutex(&status->sendQueueMutex);

    WMSP_InitVAlarm();

    if (!(param1 & 2)) {
        PM_SetLEDPattern(PM_LED_PATTERN_WIRELESS);
    }

    status->state = WM_STATE_STOP;
}

