#include "wmsp_private.h"

#ifdef WM_VALARM_VARIABLE_ON_WRAM
#include <nitro/wram_begin.h>
static OSVAlarm wmspVAlarm;
#include <nitro/wram_end.h>
#endif

static void WmspChildAdjustVSync1(void *arg);
static void WmspChildAdjustVSync2(void *arg);
static void WmspChildVAlarmMP(void *arg);

static void WmspParentAdjustVSync(void *arg);
static void WmspParentVAlarmMP(void *arg);

static void WmspSetVAlarm(s16 count, OSVAlarmHandler handler, void *arg);

static void WmspCalcVRemain(void);
static void WmspExpendVRemain(void);
static void WmspSetVTSF(void);

static void WmspFromVAlarmToWmspThread(void);

#define VALARM_COUNT_CHILD_VSYNC1 200
#define VALARM_COUNT_CHILD_VSYNC2 208

#define VALARM_COUNT_PARENT_VSYNC 203

#define VALARM_COUNT_LIMIT 210

#define VALARM_TAG_CHILD_VSYNC1 1
#define VALARM_TAG_CHILD_VSYNC2 2
#define VALARM_TAG_PARENT_VSYNC 3
#define VALARM_TAG_CHILD_MP     4
#define VALARM_TAG_PARENT_MP    5

void WMSP_InitVAlarm(void)
{
    WMSPWork *p = WMSP_GetSystemWork();

#ifdef WM_VALARM_VARIABLE_ON_WRAM
    OS_CreateVAlarm(&wmspVAlarm);
#else
    OS_CreateVAlarm(&p->VAlarm);
#endif
}

void WMSP_CancelVAlarm(void)
{
    WMSPWork *p = WMSP_GetSystemWork();

#ifdef WM_VALARM_VARIABLE_ON_WRAM
    OS_CancelVAlarm(&wmspVAlarm);
#else
    OS_CancelVAlarm(&p->VAlarm);
#endif
}

void WMSP_SetVAlarm(void)
{
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    OSVAlarm *pVAlarm;
#ifdef WM_VALARM_VARIABLE_ON_WRAM
    pVAlarm = &wmspVAlarm;
#else
    pVAlarm = &p->VAlarm;
#endif

    if (status->mode == WL_CMDLABEL_MODE_PARENT) {
        if (pVAlarm->handler != NULL) {
            OS_CancelVAlarm(pVAlarm);
        }
        WmspSetVAlarm(VALARM_COUNT_PARENT_VSYNC,
            WmspParentAdjustVSync,
            (void *)VALARM_TAG_PARENT_VSYNC);
    } else if (status->mode == WL_CMDLABEL_MODE_CHILD) {
        status->VSyncFlag = FALSE;

        if (pVAlarm->handler != NULL) {
            OS_CancelVAlarm(pVAlarm);
        }
        WmspSetVAlarm(VALARM_COUNT_CHILD_VSYNC1,
            WmspChildAdjustVSync1,
            (void *)VALARM_TAG_CHILD_VSYNC1);
        status->v_remain = 0;
    }
}

static void WmspSetVAlarm(s16 count, OSVAlarmHandler handler, void *arg)
{
#ifdef WM_VALARM_VARIABLE_ON_WRAM
    OS_SetVAlarm(&wmspVAlarm, count, OS_VALARM_DELAY_MAX, handler, arg);
#else
    WMSPWork *p = WMSP_GetSystemWork();
    OS_SetVAlarm(&p->VAlarm, count, OS_VALARM_DELAY_MAX, handler, arg);
#endif
}

static void WmspSetVTSF(void)
{
    u16 vcount;
    u32 cur_tsf;
    u16 low0, low1, high;
    u32 v_tsf;

    vcount = (u16)GX_GetVCount();

    low0 = *(vu16 *)MREG_TSF0;
    high = *(vu16 *)MREG_TSF1;
    low1 = *(vu16 *)MREG_TSF0;
    if (low0 > low1) {
        high = *(vu16 *)MREG_TSF1;
    }
    cur_tsf = high;
    cur_tsf <<= 16;
    cur_tsf |= low1;

    v_tsf = cur_tsf;
    v_tsf <<= 1;
    v_tsf -= vcount * 127;
    v_tsf >>= 7;
    v_tsf &= 0x0000ffff;

    *(vu16 *)V_TSF = (u16)v_tsf;

    return;
}

static void WmspCalcVRemain(void)
{
    u16 low0, low1, high;
    u32 cur_tsf;
    u16 vcount;
    u32 next_0line_tsf;
    int i;

    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    status->v_tsf <<= 6;

    low0 = *(vu16 *)MREG_TSF0;
    high = *(vu16 *)MREG_TSF1;
    low1 = *(vu16 *)MREG_TSF0;
    if (low0 > low1) {
        high = *(vu16 *)MREG_TSF1;
    }
    cur_tsf = high;
    cur_tsf <<= 16;
    cur_tsf |= low1;
    cur_tsf &= 0x003fffc0;

    vcount = (u16)GX_GetVCount();

    next_0line_tsf = cur_tsf;
    next_0line_tsf <<= 1;
    next_0line_tsf += ((263 - vcount) * 127);
    next_0line_tsf >>= 1;
    next_0line_tsf &= 0x003fffc0;

    if (status->v_tsf > next_0line_tsf) {
        status->v_remain = 0;
        return;
    }

    for (i = 1; i < 30; ++i) {
        status->v_tsf += HW_LCD_V_CYCLE_US;

        if (status->v_tsf > next_0line_tsf) {
            status->v_remain = status->v_tsf - next_0line_tsf;
            if (status->v_remain > (HW_LCD_V_CYCLE_US - HW_LCD_LINES_CYCLE_US(5))) {
                status->v_remain = 0;
            }
            return;
        }
    }
    status->v_remain = 0;
}

static void WmspExpendVRemain(void)
{
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    s32 vcount = GX_GetVCount();
    s32 i;

    if (VALARM_COUNT_CHILD_VSYNC2 <= vcount && vcount < VALARM_COUNT_LIMIT) {
        if (status->v_remain >= HW_LCD_LINES_CYCLE_US(2)) {
            for (i = 1; i < 7; i++) {
                if (status->v_remain < HW_LCD_LINES_CYCLE_US(2) + HW_LCD_LINES_CYCLE_US(1) * i) {
                    break;
                }
            }
            GX_SetVCount(vcount + (1 - i));
            status->v_remain -= HW_LCD_LINES_CYCLE_US(1) * i;
            WMSP_DLOGF_VALARM("VRemain: +%d lines", i);
        }
#ifdef WM_DEBUG_VALARM
        else {
            WMSP_DLOGF_VALARM("VRemain: no changes. vremain=%d us, 2lines=%d us",
                status->v_remain,
                HW_LCD_LINES_CYCLE_US(2));
        }
#endif
    }
#ifdef WM_DEBUG_VALARM
    else {
        WMSP_DLOGF_VALARM("VRemain: too late! vcount=%d", vcount);
    }
#endif
}

static void WmspChildAdjustVSync1(void *arg)
{
#pragma unused(arg)
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    status->v_tsf = *(u16 *)V_TSF;
    if (status->v_tsf_bak != status->v_tsf) {
        status->v_tsf_bak = status->v_tsf;
        WmspCalcVRemain();
    }

    if (status->v_remain > HW_LCD_LINES_CYCLE_US(2)) {
        WmspSetVAlarm(VALARM_COUNT_CHILD_VSYNC2,
            WmspChildAdjustVSync2,
            (void *)VALARM_TAG_CHILD_VSYNC2);
        WMSP_DLOGF_VALARM("VRemain: vremain:%d us > 2lines:%d us", status->v_remain, HW_LCD_LINES_CYCLE_US(2));
    } else {
        status->VSyncFlag = TRUE;

        WmspSetVAlarm((s16)status->mp_childVCount, WmspChildVAlarmMP, (void *)VALARM_TAG_CHILD_MP);
        WMSP_DLOGF_VALARM("VRemain: vremain:%d us <= 2lines:%d us", status->v_remain, HW_LCD_LINES_CYCLE_US(2));
    }
}

static void WmspChildAdjustVSync2(void *arg)
{
#pragma unused(arg)
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    WmspExpendVRemain();

    if (status->v_remain >= HW_LCD_LINES_CYCLE_US(2)) {
        status->VSyncFlag = FALSE;
    }

    WmspSetVAlarm((s16)status->mp_childVCount, WmspChildVAlarmMP, (void *)VALARM_TAG_CHILD_MP);
}

static void WmspChildVAlarmMP(void *arg)
{
#pragma unused(arg)
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    if (status->mp_flag == TRUE) {
        WmspSetVAlarm(VALARM_COUNT_CHILD_VSYNC1,
            WmspChildAdjustVSync1,
            (void *)VALARM_TAG_CHILD_VSYNC1);
        if (status->mp_lifeTimeTick != 0ULL) {
            OSTick now;
            OSTick last;
            BOOL result;

            now = OS_GetTick() | 1;
            last = status->mp_lastRecvTick[0];
            if ((last != 0ULL) && ((now - last) > status->mp_lifeTimeTick)) {
                u32 *buf;

                WM_DPRINTF("No MP packet receive TimeOut\n");
                status->mp_lastRecvTick[0] = 0ULL;

                buf = WMSP_GetInternalRequestBuf();
                if (buf == NULL) {
                    result = FALSE;
                } else {
                    buf[0] = WM_APIID_AUTO_DISCONNECT;
                    buf[1] = 0;
                    buf[2] = WM_DISCONNECT_REASON_MP_LIFETIME;

                    WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

                    result = OS_SendMessage(&(WMSP_GetSystemWork()->requestQ),
                        (OSMessage)buf,
                        OS_MESSAGE_NOBLOCK);
                }
                if (!result) {
                    WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

                    cb->apiid = WM_APIID_INDICATION;
                    cb->errcode = WM_ERRCODE_FIFO_ERROR;
                    cb->state = WM_STATECODE_FIFO_ERROR;
                    cb->reason = WM_APIID_AUTO_DISCONNECT;
                    WMSP_ReturnResult2Wm9(cb);
                }
                return;
            }
        }
        WmspFromVAlarmToWmspThread();
    }
}

static void WmspParentAdjustVSync(void *arg)
{
#pragma unused(arg)
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    s32 vcount;

    if (status->valarm_counter >= 60) {
        vcount = GX_GetVCount();
        if (VALARM_COUNT_PARENT_VSYNC <= vcount && vcount < VALARM_COUNT_LIMIT) {
            GX_SetVCount(vcount);
            status->valarm_counter = 0;
        }
    } else {
        ++status->valarm_counter;
    }

    WmspSetVTSF();

    WmspSetVAlarm((s16)status->mp_parentVCount, WmspParentVAlarmMP, (void *)VALARM_TAG_PARENT_MP);
}

static void WmspParentVAlarmMP(void *arg)
{
#pragma unused(arg)
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    if (status->mp_flag == TRUE) {
        WmspSetVAlarm(VALARM_COUNT_PARENT_VSYNC,
            WmspParentAdjustVSync,
            (void *)VALARM_TAG_PARENT_VSYNC);

        WmspFromVAlarmToWmspThread();
    }
}

static void WmspFromVAlarmToWmspThread(void)
{
    BOOL result;
    u32 *buf;
    WMSPWork *wmspW = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    OSIntrMode enabled;

    enabled = OS_DisableInterrupts();
    if (status->valarm_queuedFlag == TRUE) {
        (void)OS_RestoreInterrupts(enabled);
        return;
    }
    status->valarm_queuedFlag = TRUE;
    (void)OS_RestoreInterrupts(enabled);

    buf = WMSP_GetInternalRequestBuf();
    if (buf == NULL) {
        result = FALSE;
    } else {
        buf[0] = WM_APIID_VALARM_MP;

        WMSP_DLOGF_REQUEST_QUEUE("[R]:=%d", buf[0]);

        result = OS_SendMessage(&(wmspW->requestQ), (OSMessage)buf, OS_MESSAGE_NOBLOCK);
    }

    if (!result) {
        status->valarm_queuedFlag = FALSE;
        if (wmspW->wm7buf != NULL) {
            WMIndCallback *cb = (WMIndCallback *)WMSP_GetBuffer4Callback2Wm9();

            cb->apiid = WM_APIID_INDICATION;
            cb->errcode = WM_ERRCODE_FIFO_ERROR;
            cb->state = WM_STATECODE_FIFO_ERROR;
            cb->reason = WM_APIID_VALARM_MP;
            WMSP_ReturnResult2Wm9((void *)cb);
        }
    }
}
