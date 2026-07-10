#ifndef NITRO_WVR_ARM7_WVR_SP_H_
#define NITRO_WVR_ARM7_WVR_SP_H_

#include <nitro/os/common/alloc.h>

#ifdef  __cplusplus
extern "C" {
#endif

void WVR_Init(OSHeapHandle heap);
void WVR_Shutdown(void);

#ifdef  __cplusplus
}
#endif

#endif
