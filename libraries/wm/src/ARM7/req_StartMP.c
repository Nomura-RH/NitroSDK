#include <nitro.h>

#include "wmsp_private.h"

static void WMSP_SetMPTmpParameter(WMMPTmpParam *tmpParam)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u32 mask = tmpParam->mask;
    if (status->state != WM_STATE_MP_PARENT && status->state != WM_STATE_MP_CHILD) {
        OSIntrMode e = OS_DisableInterrupts();

        u16 maxFreq = (mask & WM_MP_TMP_PARAM_MAX_FREQUENCY) ? tmpParam->maxFrequency : status->mp_maxFreq;
        if (!maxFreq) {
            maxFreq = WM_MP_FREQ_CONT;
        }

        u16 minFreq = (mask & WM_MP_TMP_PARAM_MIN_FREQUENCY) ? tmpParam->minFrequency : status->mp_minFreq;
        if (!minFreq) {
            minFreq = WM_MP_FREQ_CONT;
        }
        if (minFreq > maxFreq) {
            minFreq = maxFreq;
        }

        u16 freq = (mask & WM_MP_TMP_PARAM_FREQUENCY) ? tmpParam->frequency : status->mp_freq;
        if (!freq) {
            freq = WM_MP_FREQ_CONT;
        }
        if (freq > maxFreq) {
            freq = maxFreq;
        }

        status->mp_current_maxFreq = maxFreq;
        status->mp_current_minFreq = minFreq;
        status->mp_current_freq = freq;

        if (status->mp_count > maxFreq) {
            status->mp_count = maxFreq;
        }

        status->mp_current_defaultRetryCount = (mask & WM_MP_TMP_PARAM_DEFAULT_RETRY_COUNT) ? tmpParam->defaultRetryCount : status->mp_defaultRetryCount;
        status->mp_current_minPollBmpMode = (mask & WM_MP_TMP_PARAM_MIN_POLL_BMP_MODE) ? tmpParam->minPollBmpMode : status->mp_minPollBmpMode;
        status->mp_current_singlePacketMode = (mask & WM_MP_TMP_PARAM_SINGLE_PACKET_MODE) ? tmpParam->singlePacketMode : status->mp_singlePacketMode;
        status->mp_current_ignoreFatalErrorMode = (mask & WM_MP_TMP_PARAM_IGNORE_FATAL_ERROR_MODE) ? tmpParam->ignoreFatalErrorMode : status->mp_ignoreFatalErrorMode;

        OS_RestoreInterrupts(e);
    }
}

void WMSP_StartMP(OSMessage msg)
{
    u32 wlBuf[128];
    u32 *buf = (u32 *)msg;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    WMErrCode err = WM_ERRCODE_SUCCESS;

    WMStartMPReq *req = (WMStartMPReq *)buf;
    WMMpRecvBuf *recvBuf = (WMMpRecvBuf *)req->recvBuf;
    u32 recvBufSize = req->recvBufSize;
    u32 *sendBuf = req->sendBuf;
    u32 sendBufSize = req->sendBufSize;
    WMMPParam *mpParam = &req->param;
    WMMPTmpParam *mpTmpParam = &req->tmpParam;

    if (status->mp_ignoreSizePrecheckMode == FALSE) {
        if (sendBufSize < WMSP_GetMPSendBufferSize(status->mp_maxSendSize)) {
            err = WM_ERRCODE_INVALID_PARAM;
        }
        if (recvBufSize < ((status->aid == 0) ? WMSP_GetMPOneReceiveBufferSizeParent(status->mp_maxRecvSize, status->pparam.maxEntry) : WMSP_GetMPOneReceiveBufferSizeChild(status->mp_maxRecvSize))) {
            err = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (status->mode == WL_CMDLABEL_MODE_CHILD) {
        if (!(status->allowedChannel & (0x0001 << sys->wm7buf->connectPInfo.channel) >> 1)) {
            err = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (err != WM_ERRCODE_SUCCESS) {
        WMStartMPCallback *cb = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();
        cb->apiid = WM_APIID_START_MP;
        cb->errcode = err;
        cb->state = WM_STATECODE_MP_START;
        WMSP_ReturnResult2Wm9((void *)cb);
        return;
    }

    {
        BOOL fCleanQueue = FALSE;
        OSIntrMode e;

        if (status->mp_flag) {
            status->mp_flag = FALSE;
            fCleanQueue = TRUE;
            OS_Warning("[ARM7] WMSP_StartMP: mp_flag == TRUE");
        }

#ifdef WM_CLEAN_SEND_QUEUE
        if (fCleanQueue) {
            WMSP_CleanSendQueue(0xffff);
        }
#endif
        WMSP_InitSendQueue();

        e = OS_DisableInterrupts();

        (void)WMSP_SetMPParameterCore(mpParam, NULL);

        WMSP_SetMPTmpParameter(mpTmpParam);

        if ((status->state == WM_STATE_CHILD) || (status->state == WM_STATE_PARENT)) {
            status->mp_waitAckFlag = FALSE;
            status->mp_vsyncOrderedFlag = FALSE;
            status->mp_vsyncFlag = TRUE;
            status->mp_newFrameFlag = FALSE;
            status->mp_pingFlag = FALSE;
            status->mp_pingCounter = WMSP_MP_PING_COUNT;
            status->sendQueueInUse = FALSE;
            status->mp_setDataFlag = FALSE;
            status->mp_sentDataFlag = FALSE;
            status->mp_bufferEmptyFlag = FALSE;
            status->mp_isPolledFlag = FALSE;
            status->mp_resumeFlag = FALSE;
            status->mp_recvBuf[0] = recvBuf;
            status->mp_recvBufSize = (u16)recvBufSize;
            status->mp_recvBuf[1] = (WMMpRecvBuf *)((u32)recvBuf + recvBufSize);
            status->mp_recvBufSel = 0;
            status->mp_sendBuf = sendBuf;
            status->mp_sendBufSize = (u16)sendBufSize;
            status->mp_count = 0;
            status->mp_limitCount = 0;
            status->mp_prevPollBitmap = 0;
            status->mp_prevWmHeader = 0;

#ifdef WMSP_UPDATE_LINK_LEVEL_PER_FRAME
            status->minRssi = 0xffff;
            status->rssiCounter = WMSP_RSSI_COUNT;
#endif

            {
                OSTick now = OS_GetTick() | 1;
                int i;

                for (i = 0; i < (1 + WM_NUM_MAX_CHILD); i++) {
                    status->mp_lastRecvTick[i] = now;
                }
            }

            WMSP_SetThreadPriorityHigh();

            status->valarm_queuedFlag = FALSE;
            WMSP_SetVAlarm();

            if (status->state == WM_STATE_CHILD) {
                status->state = WM_STATE_MP_CHILD;
            } else if (status->state == WM_STATE_PARENT) {
                status->state = WM_STATE_MP_PARENT;
            }
            {
                WMStartMPCallback *cb = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();

                cb->apiid = WM_APIID_START_MP;
                cb->errcode = WM_ERRCODE_SUCCESS;
                cb->state = WM_STATECODE_MP_START;

                WMSP_ReturnResult2Wm9((void *)cb);
            }
            status->mp_flag = TRUE;
            (void)OS_RestoreInterrupts(e);

            {
                WlCmdCfm *p_confirm;
#ifdef WMSP_MP_FORCE_DISABLE_NULL_ACK
                p_confirm = (WlCmdCfm *)WMSP_WL_ParamSetNullKeyResponseMode((u16 *)wlBuf, 0);
#else
                p_confirm = (WlCmdCfm *)WMSP_WL_ParamSetNullKeyResponseMode((u16 *)wlBuf, 1);
#endif
                if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                    WMCallback *cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
                    cb->apiid = WM_APIID_START_MP;
                    cb->errcode = WM_ERRCODE_FAILED;
                    cb->wlCmdID = WL_CMDCODE_PARAM_SET_NULLKEYMODE;
                    cb->wlResult = p_confirm->resultCode;
                    WMSP_ReturnResult2Wm9((void *)cb);
                    return;
                }
            }
            return;
        } else {
            (void)OS_RestoreInterrupts(e);
            {
                WMStartMPCallback *cb = (WMStartMPCallback *)WMSP_GetBuffer4Callback2Wm9();
                cb->apiid = WM_APIID_START_MP;
                cb->errcode = WM_ERRCODE_ILLEGAL_STATE;
                cb->state = WM_STATECODE_MP_START;
                WMSP_ReturnResult2Wm9((void *)cb);
            }
            return;
        }
    }
}
