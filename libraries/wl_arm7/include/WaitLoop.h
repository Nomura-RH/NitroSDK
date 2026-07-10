#ifndef __WAITLOOP_H_
#define __WAITLOOP_H_

void WaitLoop_Rxpe(void);
void WaitLoop_Waitus(u32 us, void (*TimeoutFunc)(void *));
void WaitLoop_ClrAid(void);
u32 WaitLoop_BbpAccess(void);
u32 WaitLoop_RfAccess(void);

#ifdef __WAITLOOP_C_

#endif // __WAITLOOP_C_
#endif // __WAITLOOP_H_
