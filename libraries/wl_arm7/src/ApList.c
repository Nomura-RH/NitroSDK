#define __APLIST_C_
#define __INSYSROM__

#include "WlSys.h"
#include "TaskMan.h"

#include "WlLib.h"
#include "WlNic.h"
#include "ApList.h"

void UpdateApList(u16 channel, LPBEACON_FRAME pFrm, LPSSID_ELEMENT pSSID)
{
    LPAP_LIST pApList = wlMan->ApList;
    u32 i, j;
    u32 free0, free1, life1;

    if (WL_ReadByte(&pSSID->Length) > MAX_SSID_LENGTH) {
        return;
    }

    free0 = MAX_APLIST_NUM;
    free1 = MAX_APLIST_NUM;
    life1 = MAX_APLIST_LIFETIME;
    for (i = 0; i < MAX_APLIST_NUM; i++, pApList++) {
        if (pApList->rssi != 0) {
            if (MatchMacAdrs(pApList->bssid, pFrm->Dot11Header.BSSID)) {
                goto write_ap_list;
            } else if (pApList->lifeTime < life1) {
                life1 = pApList->lifeTime;
                free1 = i;
            }
        } else {
            free0 = i;
        }
    }

    if (free0 != MAX_APLIST_NUM) {
        i = free0;
    } else if (free1 != MAX_APLIST_NUM) {
        i = free1;
    } else {
        return;
    }

write_ap_list:
    pApList = &wlMan->ApList[i];

    WLLIB_DmaClear16((u32)pApList, sizeof(AP_LIST));

    pApList->lifeTime = MAX_APLIST_LIFETIME;
    pApList->rssi = LoadLow(&pFrm->MacHeader.Rx.rsv_RSSI);
    pApList->channel = channel;
    WSetMacAdrs1(pApList->bssid, pFrm->Dot11Header.BSSID);
    pApList->ssidLength = WL_ReadByte(&pSSID->Length);
    for (j = 0; j < pApList->ssidLength; j++) {
        WL_WriteByte(&pApList->ssid[j], WL_ReadByte(&pSSID->SSID[j]));
    }
    pApList->beaconInterval = pFrm->Body.BeaconInterval;
    pApList->capaInfo = pFrm->Body.CapaInfo.Data;
}

void InitApList(void)
{
    WLLIB_DmaClear16((u32)wlMan->ApList, sizeof(AP_LIST) * MAX_APLIST_NUM);
}

void UpdateApListTask(void)
{
    LPAP_LIST pApList = wlMan->ApList;
    u32 i;

    for (i = 0; i < MAX_APLIST_NUM; i++) {
        if (pApList->lifeTime != 0) {
            if (--pApList->lifeTime == 0) {
                pApList->rssi = 0;
            }
        }
    }
}
