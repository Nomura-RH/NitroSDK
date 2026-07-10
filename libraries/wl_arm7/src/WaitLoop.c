#define __WAITLOOP_C_
#define __INSYSROM__

#include "WlSys.h"
#include "WlLib.h"

#include "MAC.h"
#include "DbgChar.h"
#include "WaitLoop.h"

void WaitLoop_Rxpe(void)
{
    u32 i;

    for (i = 4000; i > 0; i--) {
        if (*(vu16 *)MREG_SIGNAL_STATE & SIGNAL_STATE_RXPE) {
            return;
        }
    }

    DbgWlPrint(B_WL_DBG_ERRMSG, "RX_PE timeout\r\n");
}

void WaitLoop_Waitus(u32 us, void (*TimeoutFunc)(void *))
{
    volatile u32 flag = TRUE;

    OS_SetAlarm(&wlMan->Alarm, OS_MicroSecondsToTicks(us), TimeoutFunc, (void *)&flag);

    while (flag) {}
}

void WaitLoop_ClrAid(void)
{
    u32 x, state, b = TRUE;

    while (b) {
        x = OS_DisableIrq();

        if ((*(vu16 *)MREG_SIGNAL_STATE & (SIGNAL_STATE_CCA | SIGNAL_STATE_TRRDY)) != (SIGNAL_STATE_CCA | SIGNAL_STATE_TRRDY)) {
            state = *(vu16 *)MREG_MAC_STATE;

            if ((state != 5) && (state != 7) && (state != 8)) {
                *(vu16 *)MREG_KSID = 0;

                b = FALSE;
            }
        }

        OS_RestoreIrq(x);
    }
}

u32 WaitLoop_BbpAccess(void)
{
    u32 i;

    for (i = 0; i < 10240; i++) {
        if ((*(vu16 *)MREG_BBP_STS & SRLDEV_STS_BUSY) == 0) {
            return 0;
        }
    }

    DbgPuts("Check BBP Timeout\r");

    return 1;
}

u32 WaitLoop_RfAccess(void)
{
    u32 i;

    for (i = 0; i < 10240; i++) {
        if ((*(vu16 *)MREG_RF_STS & SRLDEV_STS_BUSY) == 0) {
            return 0;
        }
    }

    DbgPuts("Check RF Timeout\r");

    return 1;
}
