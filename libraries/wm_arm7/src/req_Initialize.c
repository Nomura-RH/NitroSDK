#include "wmsp_private.h"

extern void WMSPi_CommonInit(u32 param1);
extern BOOL WMSPi_CommonWlIdle(u16 *pWlCommand, u16 *pWlResult);

void WMSP_Initialize(OSMessage msg)
{
    u32 *reqBuf = (u32 *)msg;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMArm7Buf *p;
    WMCallback *cb;

    sys->wm7buf = (WMArm7Buf *)(reqBuf[1]);
    p = sys->wm7buf;
    p->status = sys->status = (WMStatus *)(reqBuf[2]);
    p->fifo7to9 = (u32 *)(reqBuf[3]);

    WMSPi_CommonInit(reqBuf[4]);

    {
        u16 wlcom;
        u16 wlres;

        if (!WMSPi_CommonWlIdle(&wlcom, &wlres)) {
            cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
            cb->apiid = WM_APIID_INITIALIZE;
            cb->errcode = WM_ERRCODE_FAILED;
            cb->wlCmdID = wlcom;
            cb->wlResult = wlres;
            WMSP_ReturnResult2Wm9((void *)cb);
            return;
        }
    }

    p->status->state = WM_STATE_IDLE;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_INITIALIZE;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);
}

