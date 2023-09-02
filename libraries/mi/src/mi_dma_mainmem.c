#ifdef SDK_ARM9
#include <nitro.h>

#include "../include/mi_dma.h"

void MI_DispMemDmaCopy (u32 dmaNo, const void * src)
{
	MIi_ASSERT_DMANO(dmaNo);
	MIi_ASSERT_SRC_ALIGN4(src);
	MIi_WARNING_ADDRINTCM(src, 0x18000);

	MIi_CheckAnotherAutoDMA(dmaNo, MI_DMA_TIMING_DISP_MMEM);
	MIi_CheckDma0SourceAddress(dmaNo, (u32)src, 0x18000, MI_DMA_SRC_INC);
	MIi_DmaSetParams(dmaNo, (u32)src, (u32)REG_DISP_MMEM_FIFO_ADDR, MI_CNT_MMCOPY(0));
}
#endif
