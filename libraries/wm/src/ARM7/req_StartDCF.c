#include "wmsp_private.h"

void WMSP_StartDCF(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();

    WMDcfRecvBuf *recvBuf;
    u16 recvSize;
    OSIntrMode e;

    recvBuf = (WMDcfRecvBuf *)buf[1];
    recvSize = (u16)buf[2];

    e = OS_DisableInterrupts();
    status->dcf_recvBuf[0] = recvBuf;
    status->dcf_recvBufSize = recvSize;
    status->dcf_recvBuf[1] = (WMDcfRecvBuf *)((u32)recvBuf + (u32)recvSize);
    status->dcf_recvBufSel = 0;

    status->dcf_sendData = NULL;
    status->dcf_sendSize = 0;
    status->dcf_sendFlag = FALSE;

    status->state = WM_STATE_DCF_CHILD;

    {
        WMStartDCFCallback *cb = (WMStartDCFCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb->apiid = WM_APIID_START_DCF;
        cb->errcode = WM_ERRCODE_SUCCESS;
        cb->state = WM_STATECODE_DCF_START;

        WMSP_ReturnResult2Wm9((void *)cb);
    }
    status->dcf_flag = TRUE;
    (void)OS_RestoreInterrupts(e);
}
