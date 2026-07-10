#ifndef __APLIST_H_
#define __APLIST_H_

#include "Dot11Frm.h"
#include "WlStaList.h"

typedef WlApElement AP_LIST;
typedef WlApElement *LPAP_LIST;

void InitApList(void);
void UpdateApList(u16 channel, LPBEACON_FRAME pFrm, LPSSID_ELEMENT pSSID);
void UpdateApListTask(void);

#ifdef __APLIST_C_

#endif // __APLIST_C_
#endif // __APLIST_H_
