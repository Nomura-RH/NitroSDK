#ifndef LIBRARIES_WM_PRIVATE_H_
#define LIBRARIES_WM_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nitro.h>

#ifdef SDK_ARM7
#include <nitro_wl/ARM7/WlLib.h>
#endif

#define WM_WARNING(...) ((void)0)

#define WM_ASSERT(exp)    ((void)0)
#define WM_ASSERTMSG(...) ((void)0)
#define WM_DPRINTF(...)   ((void)0)

#define WMi_Printf(...)  ((void)0)
#define WMi_Warning(...) ((void)0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBRARIES_WM_PRIVATE_H_ */
