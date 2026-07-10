#ifndef NITRO_WM_ARM7_WM_SP_H_
#define NITRO_WM_ARM7_WM_SP_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <nitro.h>
#include <nitro_wl/common/version_wl.h>
#include <nitro_wl/ARM7/WlLib.h>


#define WM_WL_WORK_SIZE         0x700
#define WM_WL_STACK_SIZE        0x600
#define WM_WL_CAMTABLE_NUM      16
#define WM_WL_HEAP_SIZE         0x2100

#define WM_SP_WM_DMA_NO         3

#define WM_SP_WL_DMA_NO         2
#define WM_DMA_MAX_SIZE         64

//#define SDK_SMALL_BUILD_WL      1
//#define   SDK_SIMPLE_DIAG_WL  1
#define SDK_CPU_COPY_WL         1


typedef struct WmInit {
    u32     dmaNo;
    u32     indPrio_low;
    u32     indPrio_high;
    u32     reqPrio_low;
    u32     reqPrio_high;
    u32     wlPrio_low;
    u32     wlPrio_high;
} WmInit;

void    WM_sp_init(WlInit *wlInit, WmInit *wmInit);


#ifdef  __cplusplus
}
#endif

#endif
