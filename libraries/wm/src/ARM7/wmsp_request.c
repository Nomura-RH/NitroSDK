#include "wmsp_private.h"

static void WmspRequestFuncDummy(OSMessage msg);

static const WMSPRequestFunc WmspRequestFuncTable[WM_APIID_ASYNC_KIND_MAX] = {
    WMSP_Initialize,
    WMSP_Reset,
    WMSP_End,
    WMSP_Enable,
    WMSP_Disable,
    WMSP_PowerOn,
    WMSP_PowerOff,
    WMSP_SetParentParam,
    WMSP_StartParent,
    WMSP_EndParent,
    WMSP_StartScan,
    WMSP_EndScan,
    WMSP_StartConnectEx,
    WMSP_Disconnect,
    WMSP_StartMP,
    WMSP_SetMPData,
    WMSP_EndMP,
    WMSP_StartDCF,
    WMSP_SetDCFData,
    WMSP_EndDCF,
    WMSP_SetWEPKey,
    WmspRequestFuncDummy,
    WmspRequestFuncDummy,
    WmspRequestFuncDummy,
    WMSP_SetGameInfo,
    WMSP_SetBeaconTxRxInd,
    WMSP_StartTestMode,
    WMSP_StopTestMode,
    WMSP_VAlarmSetMPData,
    WMSP_SetLifeTime,
    WMSP_MeasureChannel,
    WMSP_InitWirelessCounter,
    WMSP_GetWirelessCounter,
    WMSP_SetEntry,
    WMSP_AutoDeAuth,
    WMSP_SetMPParameter,
    WMSP_SetBeaconPeriod,
    WMSP_AutoDisconnect,
    WMSP_StartScanEx,
    WMSP_SetWEPKeyEx,
    WMSP_SetPowerSaveMode,
    WMSP_StartTestRxMode,
    WMSP_StopTestRxMode,
    WMSP_KickNextMP_Parent,
    WMSP_KickNextMP_Child,
    WMSP_KickNextMP_Resume,
};

void WMSP_RequestThread(void *arg)
{
    WMSPWork *p = WMSP_GetSystemWork();
    WMStatus *status = WMSP_GetStatusStructure();
    OSMessage msg;
    u16 apiid;

    while (TRUE) {
        (void)OS_ReceiveMessage(&(p->requestQ), &msg, OS_MESSAGE_BLOCK);

        if (msg == 0) {
            OS_ExitThread();
            return;
        }

        apiid = *((u16 *)msg);
        if (apiid & WM_API_REQUEST_ACCEPTED) {
            apiid &= ~(WM_API_REQUEST_ACCEPTED);
        }
        if (apiid < WM_APIID_ASYNC_KIND_MAX) {
            WMSP_DLOGF_REQUEST_QUEUE("[R]>>%d", apiid);

            status->apiBusy = TRUE;
            status->BusyApiid = apiid;
            (WmspRequestFuncTable[apiid])(msg);
            status->apiBusy = FALSE;
        } else {
        }
        *((u16 *)msg) = (u16)(apiid | WM_API_REQUEST_ACCEPTED);
    }
}

static void WmspRequestFuncDummy(OSMessage msg)
{
    OS_Printf("ARM7: Dummy request function is called.\n");
    return;
}
