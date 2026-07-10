#include "wmsp_private.h"

static void WmspStabilizeBeacon(void);
static u32 BBP_Read(u32 adrs);

BOOL WMSPi_CommonWlIdle(u16 *pWlCommand, u16 *pWlResult);

void WMSP_PowerOn(OSMessage msg)
{
    WMCallback *cb;
    WMStatus *status = WMSP_GetStatusStructure();

    if (status->state != WM_STATE_STOP) {
        cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
        cb->apiid = WM_APIID_POWER_ON;
        cb->errcode = WM_ERRCODE_ILLEGAL_STATE;
        WMSP_ReturnResult2Wm9((void *)cb);
        return;
    }

    {
        u16 wlcom;
        u16 wlres;

        if (!WMSPi_CommonWlIdle(&wlcom, &wlres)) {
            cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
            cb->apiid = WM_APIID_POWER_ON;
            cb->errcode = WM_ERRCODE_FAILED;
            cb->wlCmdID = wlcom;
            cb->wlResult = wlres;
            WMSP_ReturnResult2Wm9((void *)cb);
            return;
        }
    }

    status->state = WM_STATE_IDLE;

    cb = (WMCallback *)WMSP_GetBuffer4Callback2Wm9();
    cb->apiid = WM_APIID_POWER_ON;
    cb->errcode = WM_ERRCODE_SUCCESS;
    WMSP_ReturnResult2Wm9((void *)cb);
}

BOOL WMSPi_CommonWlIdle(u16 *pWlCommand, u16 *pWlResult)
{
    WMStatus *status = WMSP_GetStatusStructure();
    WlCmdCfm *pConfirm;
    u32 wlBuf[128];
    u16 enableChannel = 0x0000;

    {
        pConfirm = (WlCmdCfm *)WMSP_WL_DevRestart((u16 *)wlBuf);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            *pWlCommand = WL_CMDCODE_DEV_REBOOT;
            *pWlResult = pConfirm->resultCode;
            return FALSE;
        }
    }

    {
        pConfirm = (WlCmdCfm *)WMSP_WL_DevIdle((u16 *)wlBuf);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            *pWlCommand = WL_CMDCODE_DEV_IDLE;
            *pWlResult = pConfirm->resultCode;
            return FALSE;
        }
    }

    WmspStabilizeBeacon();

#ifdef SDK_TEG
    enableChannel = 0x7ffe;

#else

    {
        WlParamGetEnableChannelCfm *pECConfirm;

        pECConfirm = (WlParamGetEnableChannelCfm *)WMSP_WL_ParamGetEnableChannel((u16 *)wlBuf);
        if (pECConfirm->resultCode != WL_CMDRES_SUCCESS) {
            *pWlCommand = WL_CMDCODE_PARAM_GET_ENABLECHANNEL;
            *pWlResult = pECConfirm->resultCode;
            return FALSE;
        }
        enableChannel = pECConfirm->enableChannel;
    }

#endif

    status->enableChannel = enableChannel;
    status->allowedChannel = WMSP_GetAllowedChannel((u16)(enableChannel >> 1));

    {
        pConfirm = (WlCmdCfm *)WMSP_WL_ParamSetLifeTime((u16 *)wlBuf, 0xffff,
            WMSP_DEFAULT_CAM_LIFE_TIME,
            WMSP_DEFAULT_FRAME_LIFE_TIME
        );

        status->mp_lifeTimeTick = OS_MilliSecondsToTicks(WMSP_DEFAULT_MP_LIFE_TIME * 100);
    }

    status->rate = WL_CMDLABEL_RATE_2M;

    status->preamble = WL_CMDLABEL_PREAMBLE_SHORT;

    {
        WlDevGetVerInfoCfm *pVConfirm;

        pVConfirm = (WlDevGetVerInfoCfm *)WMSP_WL_DevGetVersion((u16 *)wlBuf);
        if (pVConfirm->resultCode != WL_CMDRES_SUCCESS) {
            *pWlCommand = WL_CMDCODE_DEV_GET_VERINFO;
            *pWlResult = pVConfirm->resultCode;
            return FALSE;
        }
        MI_CpuCopy16(pVConfirm->wlVersion, status->wlVersion, WM_SIZE_WL_VERSION);
        status->macVersion = pVConfirm->macVersion;
        status->bbpVersion[0] = pVConfirm->bbpVersion[0];
        status->bbpVersion[1] = pVConfirm->bbpVersion[1];
        status->rfVersion = pVConfirm->rfVersion;
    }

    {
        WlParamGetMacAdrsCfm *pMConfirm;

        pMConfirm = (WlParamGetMacAdrsCfm *)WMSP_WL_ParamGetMacAddress((u16 *)wlBuf);
        if (pMConfirm->resultCode != WL_CMDRES_SUCCESS) {
            *pWlCommand = WL_CMDCODE_PARAM_GET_MAC_ADRS;
            *pWlResult = pMConfirm->resultCode;
            return FALSE;
        }
        MI_CpuCopy8(pMConfirm->staMacAdrs, status->MacAddress, WM_SIZE_MACADDR);
    }

    {
        pConfirm = (WlCmdCfm *)WMSP_WL_ParamSetBeaconSendRecvInd((u16 *)wlBuf, 1);
        if (pConfirm->resultCode != WL_CMDRES_SUCCESS) {
            *pWlCommand = WL_CMDCODE_PARAM_SET_BCNTXRX_IND;
            *pWlResult = pConfirm->resultCode;
            return FALSE;
        }
    }

    return TRUE;
}

static void WmspStabilizeBeacon(void)
{
    *(vu16 *)MREG_TMPTT_ACT_TIME = 200;
    *(vu16 *)MREG_TBTT_ACT_TIME = 2000;
    *(vu16 *)MREG_RF_WAKEUP_TIME = 0x0202;
}

#define MAC_REG_RBASE   HW_WIRELESS_INTF1
#define SRLDEV_STS_BUSY (0x0001 << 0)
#define SRLBBP_CMD_READ (0x0006 * SRLDEV_CMD)
#define SRLDEV_CMD_ADRS (0x0001 << 0)

#define SRLDEV_CMD         (0x0001 << 12)
#define OFST_MREG_BBP_STS  0x15E
#define MREG_BBP_STS       (MAC_REG_RBASE + OFST_MREG_BBP_STS)
#define OFST_MREG_BBP_CMD  0x158
#define MREG_BBP_CMD       (MAC_REG_RBASE + OFST_MREG_BBP_CMD)
#define OFST_MREG_BBP_RDAT 0x15C
#define MREG_BBP_RDAT      (MAC_REG_RBASE + OFST_MREG_BBP_RDAT)

u32 BBP_Read(u32 adrs)
{
    while (*(vu16 *)MREG_BBP_STS & SRLDEV_STS_BUSY) {
    }

    *(vu16 *)MREG_BBP_CMD = (u16)(SRLBBP_CMD_READ | (adrs * SRLDEV_CMD_ADRS));

    while (*(vu16 *)MREG_BBP_STS & SRLDEV_STS_BUSY) {
    }

    return (u32)(*(vu16 *)MREG_BBP_RDAT);
}

