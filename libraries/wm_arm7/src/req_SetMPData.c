#include "wmsp_private.h"

void WMSP_SetMPData(OSMessage msg)
{
    u32 *buf = (u32 *)msg;
    WMSPWork *sys = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    u16 *sendData;
    u16 sendSize;
    u32 destBitmap;
    u16 port;
    u16 priority;
    WMCallbackFunc callback;
    void *arg;
    int result;
    u16 myAid = status->aid;
    u32 childBitmap = status->child_bitmap;

    sendData = (u16 *)buf[1];
    sendSize = (u16)buf[2];
    destBitmap = buf[3];
    port = (u16)buf[4];
    priority = (u16)buf[5];
    callback = (WMCallbackFunc)buf[6];
    arg = (void *)buf[7];

    if (myAid != 0) {
        destBitmap = 0x0001U;
    }

    if (status->mp_flag == 0) {
        result = WM_ERRCODE_ILLEGAL_STATE;
    } else {
        if ((destBitmap & childBitmap) == 0) {
            result = WM_ERRCODE_SUCCESS;
        } else {
            result = WMSP_PutSendQueue(childBitmap, priority, port, destBitmap, sendData, sendSize, callback, arg);
        }
    }

    if (result != WM_ERRCODE_OPERATING) {
        WMPortSendCallback *cb_PortSend = (WMPortSendCallback *)WMSP_GetBuffer4Callback2Wm9();

        cb_PortSend->apiid = WM_APIID_PORT_SEND;
        cb_PortSend->errcode = (u16)result;
        cb_PortSend->state = WM_STATECODE_PORT_SEND;
        cb_PortSend->port = (u16)port;
        cb_PortSend->destBitmap = (u16)destBitmap;
        if (result == WM_ERRCODE_SEND_QUEUE_FULL) {
            cb_PortSend->restBitmap = (u16)(destBitmap & childBitmap);
        } else {
            cb_PortSend->restBitmap = 0;
        }
        cb_PortSend->sentBitmap = 0;
        cb_PortSend->size = sendSize;
        cb_PortSend->data = sendData;
        cb_PortSend->callback = callback;
        cb_PortSend->arg = arg;
        cb_PortSend->seqNo = 0xffff;

        {
            u16 parentSize = status->mp_parentSize;
            u16 childSize = status->mp_childSize;
            cb_PortSend->maxSendDataSize = (status->aid == 0) ? parentSize : childSize;
            cb_PortSend->maxRecvDataSize = (status->aid == 0) ? childSize : parentSize;
        }

        WMSP_ReturnResult2Wm9((void *)cb_PortSend);
    }
}
