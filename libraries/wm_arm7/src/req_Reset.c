#include "wmsp_private.h"

static void WmspError(u16 wlCommand, u16 wlResult);

void WMSP_Reset(OSMessage msg)
{
    WMStatus *status = WMSP_GetStatusStructure();
    u32 wlBuf[128];
    u16 *buf = (u16 *)wlBuf;
    WMCallback *cb;
    WlCmdCfm *pConfirm;
    u16 station;
    u16 mode;
    u16 child_bitmap;
    BOOL fparent;

    {
        BOOL fCleanQueue = FALSE;
        OSIntrMode e = OS_DisableInterrupts();

        if (status->mp_flag == TRUE) {
            status->mp_flag = FALSE;
            fCleanQueue = TRUE;
            WMSP_CancelVAlarm();

            WMSP_SetThreadPriorityLow();

            if (status->state == WM_STATE_MP_CHILD) {
                status->state = WM_STATE_CHILD;
            } else if (status->state == WM_STATE_MP_PARENT) {
                status->state = WM_STATE_PARENT;
            }
        }
        if (status->state == WM_STATE_PARENT || status->state == WM_STATE_CHILD) {
            child_bitmap = status->child_bitmap;
            fparent = (status->state == WM_STATE_PARENT);
        } else {
            child_bitmap = 0;
        }
        status->child_bitmap = 0;
        status->mp_readyBitmap = 0;
        status->ks_flag = FALSE;
        status->dcf_flag = FALSE;
        status->VSyncFlag = FALSE;
        status->beaconIndicateFlag = 0;

        (void)OS_RestoreInterrupts(e);

#ifdef WM_CLEAN_SEND_QUEUE
        if (fCleanQueue) {
            WMSP_CleanSendQueue(0xffff);
        }
#endif
    }

    if (fparent) {
        status->pparam.entryFlag = FALSE;
    }

    if (child_bitmap != 0) {
        int i;
        for (i = 0; i < (WM_NUM_MAX_CHILD + 1); i++) {
            if (child_bitmap & (0x0001 << i)) {
                u8 *mac;
                if (i == 0) {
                    mac = status->parentMacAddress;
                } else {
                    mac = status->childMacAddress[i - 1];
                }
                WMSP_IndicateDisconnectionFromMyself(fparent, (u16)i, mac);
            }
        }
    }

    MI_CpuClear8(status->childMacAddress, sizeof(status->childMacAddress));


    pConfirm = (WlCmdCfm *)WMSP_WL_DevGetStationState(buf);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_DEV_GET_STATE, pConfirm->resultCode);
        return;
    }
    station = pConfirm->buf[0];

    pConfirm = (WlCmdCfm *)WMSP_WL_ParamGetMode(buf);
    if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
        WmspError(WL_CMDCODE_PARAM_GET_MODE, pConfirm->resultCode);
        return;
    }
    mode = pConfirm->buf[0];

    switch (station) {
    case WL_STA_CLASS3:
    case WL_STA_CLASS2:

        if ((mode == WL_CMDLABEL_MODE_CHILD) || (mode == WL_CMDLABEL_MODE_HOTSPOT)) {
            {
                u16 wMac[WM_SIZE_MACADDR / sizeof(u16)];

                MI_CpuCopy8(status->parentMacAddress, wMac, WM_SIZE_MACADDR);
                {
                    s32 auth_retry;

                    for (auth_retry = 0; auth_retry < WMSP_DEAUTH_RETRY_MAX;) {
                        pConfirm = (WlCmdCfm *)WMSP_WL_MlmeDeAuthenticate(buf, wMac, WL_CMDLABEL_RSN_DEAUTH_LEAVING);
                        switch (pConfirm->resultCode) {
                        case WL_CMDRES_SUCCESS:
                            status->state = WM_STATE_CLASS1;
                            break;
                        case WL_CMDRES_TIMEOUT:
                        case WL_CMDRES_FAILURE:
                            auth_retry++;
                            continue;
                        default:
                            break;
                        }
                        break;
                    }
                }
            }
        } else if (mode == WL_CMDLABEL_MODE_PARENT) {
            {
                u16 wMac[WM_SIZE_MACADDR / sizeof(u16)];

                MI_CpuFill8(wMac, 0xff, WM_SIZE_MACADDR);
                pConfirm = (WlCmdCfm *)WMSP_WL_MlmeDeAuthenticate(buf, wMac, WL_CMDLABEL_RSN_DEAUTH_LEAVING);
                if (pConfirm->resultCode == WL_CMDRES_SUCCESS) {
                    status->state = WM_STATE_CLASS1;
                }
            }
        } else {
        }

    case WL_STA_CLASS1:

        pConfirm = (WlCmdCfm *)WMSP_WL_MlmeReset(buf, WL_CMDLABEL_RST_MIB_CLR);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_MLME_RESET, pConfirm->resultCode);
            return;
        }

    case WL_STA_SHUTDOWN:

        pConfirm = (WlCmdCfm *)WMSP_WL_DevIdle(buf);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_IDLE, pConfirm->resultCode);
            return;
        }

    case WL_STA_IDLE:

        if (status->preamble == WL_CMDLABEL_PREAMBLE_LONG) {
            WlParamSetCfm *p_confirm;

            p_confirm = WMSP_WL_ParamSetPreambleType(buf, WL_CMDLABEL_PREAMBLE_SHORT);
            if (p_confirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_PARAM_SET_PREAMBLE_TYPE, p_confirm->resultCode);
                return;
            }

            status->preamble = WL_CMDLABEL_PREAMBLE_SHORT;
        }

        status->state = WM_STATE_IDLE;

        status->wep_flag = FALSE;

        WMSP_ResetSizeVars();

        break;

    case 0x11:
    case 0x12:

        if (mode == WL_CMDLABEL_MODE_TEST) {
            pConfirm = (WlCmdCfm *)WMSP_WL_DevTestSignal(buf, WL_CMDLABEL_TEST_SIGNAL_OFF,
                0,
                WL_CMDLABEL_TEST_SIGNAL_2M,
                1
            );
            if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
                WmspError(WL_CMDCODE_DEV_TEST_SIGNAL, pConfirm->resultCode);
                return;
            }
        }
        pConfirm = (WlCmdCfm *)WMSP_WL_DevIdle(buf);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            WmspError(WL_CMDCODE_DEV_IDLE, pConfirm->resultCode);
            return;
        }
        status->state = WM_STATE_IDLE;

        break;

    default:
        WmspError(WL_CMDCODE_DEV_GET_STATE, WL_CMDRES_SUCCESS);
        return;
    }

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_RESET;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);

    return;
}

static void WmspError(u16 wlCommand, u16 wlResult)
{
    WMCallback *cb;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_RESET;
    cb->errcode = WM_ERRCODE_FAILED;
    cb->wlCmdID = wlCommand;
    cb->wlResult = wlResult;
    WMSP_ReturnResult2Wm9((void *)cb);
}

