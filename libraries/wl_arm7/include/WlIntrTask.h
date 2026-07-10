#ifndef __WLINTRTASK_H_
#define __WLINTRTASK_H_

void WlIntrTxBeaconTask(void);
void WlIntrTxEndTask(void);
void WlIntrRxEndTask(void);
void WlIntrMpEndTask(void);
void SetParentTbttTxqTask(void);

#ifdef __WLINTRTASK_C_

static LPRXFRM TakeoutRxFrame(LPRXFRM_MAC pMFrm, u32 length)
    __attribute__((never_inline));

#endif // __WLINTRTASK_C_
#endif // __WLINTRTASK_H_
