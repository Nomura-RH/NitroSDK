#ifndef __APP_TIMER_H_
#define __APP_TIMER_H_

#define APP_TM_SCMD_BTXT 0
#define APP_TM_MPTEST    1
#define APP_TM_MACTEST   2
#define APP_TM_SYS       3

#define APP_TM_TIMEOUT_NUM 4

#define APP_TM_MS(x) (x)
// #define	APP_TM_COUNT		547

void IntrAppTimer(void *arg);
void AppTMSetTimeOut(int nTimerID, u32 dwCounter, void (*pFunc)());
void AppTMSetPeriod(int nTimerID, u32 dwCounter, void (*pFunc)());
void AppTMStopTimeOut(int nTimerID);
void InitAppTimer(void);

// extern u32 AppTime;

#endif //__TCPIP_H_
