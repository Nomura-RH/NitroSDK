#include "wmsp_private.h"

void WMSP_SetMPParameter(OSMessage msg)
{
    u32 *buf = (u32 *)msg;

    WMSetMPParameterReq *req;
    WMMPParam *param;
    WMMPParam old_param;

    WMErrCode result;

    req = (WMSetMPParameterReq *)buf;
    param = &req->param;

    result = WMSP_SetMPParameterCore(param, &old_param);

    {
        WMSetMPParameterCallback *cb = (WMSetMPParameterCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_SET_MP_PARAMETER;
        cb->errcode = result;
        cb->mask = param->mask;
        MI_CpuCopy8(&old_param, &cb->oldParam, sizeof(cb->oldParam));
        WMSP_ReturnResult2Wm9(cb);
    }
}

WMErrCode WMSP_SetMPParameterCore(const WMMPParam *param, WMMPParam *old_param)
{
    WMStatus *status = WMSP_GetStatusStructure();
    OSIntrMode enabled;

    WMErrCode result = WM_ERRCODE_SUCCESS;
    u32 mask;

    mask = param->mask;

    if (status->state == WM_STATE_MP_PARENT || status->state == WM_STATE_MP_CHILD) {
        if (mask & WM_MP_PARAM_MUST_SET_BEFORE_MP) {
            result = WM_ERRCODE_ILLEGAL_STATE;
            mask &= ~WM_MP_PARAM_MUST_SET_BEFORE_MP;
        }
    }

    enabled = OS_DisableInterrupts();

    if (old_param != NULL) {
        old_param->mask = WM_MP_PARAM_MIN_FREQUENCY | WM_MP_PARAM_FREQUENCY | WM_MP_PARAM_MAX_FREQUENCY | WM_MP_PARAM_PARENT_SIZE | WM_MP_PARAM_CHILD_SIZE | WM_MP_PARAM_PARENT_VCOUNT | WM_MP_PARAM_CHILD_VCOUNT | WM_MP_PARAM_PARENT_INTERVAL | WM_MP_PARAM_CHILD_INTERVAL | WM_MP_PARAM_DEFAULT_RETRY_COUNT | WM_MP_PARAM_MIN_POLL_BMP_MODE | WM_MP_PARAM_SINGLE_PACKET_MODE | WM_MP_PARAM_IGNORE_FATAL_ERROR_MODE | WM_MP_PARAM_IGNORE_SIZE_PRECHECK_MODE;
        old_param->minFrequency = status->mp_minFreq;
        old_param->frequency = status->mp_freq;
        old_param->maxFrequency = status->mp_maxFreq;
        old_param->parentSize = status->mp_parentSize;
        old_param->childSize = status->mp_childSize;
        old_param->parentInterval = status->mp_parentInterval;
        old_param->childInterval = status->mp_childInterval;
        old_param->parentVCount = status->mp_parentVCount;
        old_param->childVCount = status->mp_childVCount;
        old_param->defaultRetryCount = status->mp_defaultRetryCount;
        old_param->minPollBmpMode = (u8)status->mp_minPollBmpMode;
        old_param->singlePacketMode = (u8)status->mp_singlePacketMode;
        old_param->ignoreFatalErrorMode = (u8)status->mp_ignoreFatalErrorMode;
        old_param->ignoreSizePrecheckMode = (u8)status->mp_ignoreSizePrecheckMode;
    }

    if (mask & WM_MP_PARAM_MIN_FREQUENCY) {
        u16 freq = param->minFrequency;

        if (freq == 0) {
            freq = WM_MP_FREQ_CONT;
        }

        status->mp_minFreq = freq;
        status->mp_current_minFreq = freq;
    }

    if (mask & WM_MP_PARAM_FREQUENCY) {
        u16 freq = param->frequency;

        if (freq == 0) {
            freq = WM_MP_FREQ_CONT;
        }

        status->mp_freq = freq;
        status->mp_current_freq = freq;
        if (status->mp_count > freq) {
            status->mp_count = (s16)freq;
        }
    }

    if (mask & WM_MP_PARAM_MAX_FREQUENCY) {
        u16 freq = param->maxFrequency;

        if (freq == 0) {
            freq = WM_MP_FREQ_CONT;
        }

        status->mp_maxFreq = freq;
        status->mp_current_maxFreq = freq;
        if (status->mp_count > freq) {
            status->mp_count = (s16)freq;
        }
    }

    if (mask & WM_MP_PARAM_PARENT_SIZE) {
        u16 parentSize = (u16)MATH_ROUNDUP(param->parentSize, 2);
        if (parentSize <= status->mp_parentMaxSize) {
            WMSP_SetParentSize(param->parentSize);
        } else {
            result = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (mask & WM_MP_PARAM_CHILD_SIZE) {
        u16 childSize = (u16)MATH_ROUNDUP(param->childSize, 2);
        if (childSize <= 0x200) {
            WMSP_SetChildSize(param->childSize);
        } else {
            result = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (mask & WM_MP_PARAM_PARENT_INTERVAL) {
        u16 parentInterval = param->parentInterval;
        if (parentInterval <= WM_MP_PARAM_INTERVAL_MAX) {
            status->mp_parentInterval = parentInterval;
            status->mp_parentIntervalTick = OS_MilliSecondsToTicks(parentInterval) / 1024;
        } else {
            result = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (mask & WM_MP_PARAM_CHILD_INTERVAL) {
        u16 childInterval = param->childInterval;
        if (childInterval <= WM_MP_PARAM_INTERVAL_MAX) {
            status->mp_childInterval = childInterval;
            status->mp_childIntervalTick = OS_MilliSecondsToTicks(childInterval) / 1024;
        } else {
            result = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (mask & WM_MP_PARAM_PARENT_VCOUNT) {
        u16 parentVCount = param->parentVCount;
        if (parentVCount <= WM_VALARM_COUNT_RANGE_BOTTOM
            || (WM_VALARM_COUNT_RANGE_TOP <= parentVCount && parentVCount <= 262)) {
            status->mp_parentVCount = parentVCount;
        } else {
            result = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (mask & WM_MP_PARAM_CHILD_VCOUNT) {
        u16 childVCount = param->childVCount;
        if (childVCount <= WM_VALARM_COUNT_RANGE_BOTTOM
            || (WM_VALARM_COUNT_RANGE_TOP <= childVCount && childVCount <= 262)) {
            status->mp_childVCount = childVCount;
        } else {
            result = WM_ERRCODE_INVALID_PARAM;
        }
    }

    if (mask & WM_MP_PARAM_DEFAULT_RETRY_COUNT) {
        u16 defaultRetryCount = param->defaultRetryCount;
        status->mp_defaultRetryCount = defaultRetryCount;
        status->mp_current_defaultRetryCount = defaultRetryCount;
    }

    if (mask & WM_MP_PARAM_MIN_POLL_BMP_MODE) {
        status->mp_minPollBmpMode = param->minPollBmpMode;
    }

    if (mask & WM_MP_PARAM_SINGLE_PACKET_MODE) {
        status->mp_singlePacketMode = param->singlePacketMode;
    }

    if (mask & WM_MP_PARAM_IGNORE_FATAL_ERROR_MODE) {
        status->mp_ignoreFatalErrorMode = param->ignoreFatalErrorMode;
        status->mp_current_ignoreFatalErrorMode = status->mp_ignoreFatalErrorMode;
    }

    if (mask & WM_MP_PARAM_IGNORE_SIZE_PRECHECK_MODE) {
        status->mp_ignoreSizePrecheckMode = param->ignoreSizePrecheckMode;
    }

    (void)OS_RestoreInterrupts(enabled);

    return result;
}

