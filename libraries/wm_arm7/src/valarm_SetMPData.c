#include "wmsp_private.h"

void WMSP_VAlarmSetMPData(OSMessage msg)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    OSIntrMode enabled;

    status->valarm_queuedFlag = FALSE;
    status->mp_newFrameFlag = TRUE;

#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
    --status->rssiCounter;
    if (status->rssiCounter == 0) {
#ifdef WMSP_LINK_LEVEL_TREAT_ABSENCE_AS_ERROR
        if (status->minRssi == 0xffff) {
            status->minRssi = WMSP_RSSI_ERROR;
        }
        WMSP_AddRssiToList((u8)status->minRssi);
        status->linkLevel = WMSP_GetAverageLinkLevel();
        status->minRssi = 0xffff;
#else
        if (status->minRssi != 0xffff) {
            WMSP_AddRssiToList((u8)status->minRssi);
            status->linkLevel = WMSP_GetAverageLinkLevel();
            status->minRssi = 0xffff;
        }
#endif
        status->rssiCounter = WMSP_RSSI_COUNT;
    }
#endif

    if (status->state == WM_STATE_MP_PARENT) {
        BOOL sleeping;
        BOOL kick;

        enabled = OS_DisableInterrupts();

        if (status->child_bitmap == 0) {
            status->mp_count = 0;
            (void)OS_RestoreInterrupts(enabled);
            return;
        }

        sleeping = (status->mp_count <= 0 || status->mp_limitCount <= 0);

        if (status->mp_count < 0) {
            status->mp_count = 0;
        }
        status->mp_count += (s16)status->mp_current_freq;
        if (status->mp_count > WM_MP_COUNT_LIMIT) {
            status->mp_count = WM_MP_COUNT_LIMIT;
        }

        status->mp_limitCount = (s16)status->mp_current_maxFreq;

        kick = (sleeping && status->mp_count > 0 && status->mp_limitCount > 0);

        (void)OS_RestoreInterrupts(enabled);

        if (kick) {
            WMSP_SendMaMP(0xffff);
        }

        if (status->mp_current_minPollBmpMode == TRUE) {
            status->mp_pingCounter--;
            if (status->mp_pingCounter == 0) {
                status->mp_pingFlag = TRUE;
                status->mp_pingCounter = WMSP_MP_PING_COUNT;
            }
        }
    }
    else if (status->state == WM_STATE_MP_CHILD) {

        BOOL kick = FALSE;

        enabled = OS_DisableInterrupts();

#if SDK_VERSION_WL < 25700
        if (status->mp_vsyncFlag == TRUE)
#endif
        {
            if (status->sendQueueInUse == TRUE) {
                WM_DPRINTF("valarm while sendQueueInUse == TRUE\n");
            } else {
                kick = TRUE;
                status->mp_vsyncFlag = FALSE;
            }
        }

        (void)OS_RestoreInterrupts(enabled);

        if (kick == TRUE) {
            WMSP_SendMaKeyData();
        }
    }
#ifdef WM_DEBUG
    else {
        OS_Warning("state %d != WM_STATE_MP_{PARENT, CHILD}", status->state);
    }
#endif
}

void WMSP_KickNextMP_Parent(OSMessage msg)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    u32 *reqBuf = (u32 *)msg;

    if (status->state == WM_STATE_MP_PARENT) {
        WMSP_SendMaMP((u16)reqBuf[1]);
    }
#ifdef WM_DEBUG
    else {
        OS_Warning("state %d != WM_STATE_MP_PARENT", status->state);
    }
#endif
}

void WMSP_KickNextMP_Child(OSMessage msg)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    if (status->state == WM_STATE_MP_CHILD) {
        WMSP_SendMaKeyData();
    }
#ifdef WM_DEBUG
    else {
        OS_Warning("state %d != WM_STATE_MP_CHILD", status->state);
    }
#endif
}

void WMSP_KickNextMP_Resume(OSMessage msg)
{
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    u32 *reqBuf = (u32 *)msg;

    if (status->state == WM_STATE_MP_PARENT) {
        WMSP_ResumeMaMP((u16)reqBuf[1]);
    }
#ifdef WM_DEBUG
    else {
        OS_Warning("state %d != WM_STATE_MP_PARENT", status->state);
    }
#endif
}
