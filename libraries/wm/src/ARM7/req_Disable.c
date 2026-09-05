
#include "wmsp_private.h"

void WMSP_Disable(OSMessage msg)
{
#pragma unused(msg)

    WMCallback *cb;
    WMStatus *status = WMSP_GetStatusStructure();

    if (status->state != WM_STATE_STOP) {
        cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        cb->apiid = WM_APIID_DISABLE;
        cb->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9((void *)cb);
        return;
    }

    PM_SetLEDPattern(WMSP_LED_BLINK_DISABLE);

    status->state = WM_STATE_READY;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_DISABLE;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);
}

