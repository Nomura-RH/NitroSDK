#if defined(SDK_TEG)

#include <nitro.h>

#include "../include/card_spi.h"
#include "../include/card_common.h"

static void CARDi_ReadCartridge (CARDRomStat *p)
{
	CARDiCommon *const c = &cardi_common;

	const BOOL is_dma_static =
#if defined(SDK_ARM9)
		!CARDi_IsTcm(c->dst, c->len) &&
#endif
		(c->dma <= MI_DMA_MAX_NUM) && (((c->src ^ c->dst) & 3) == 0);

	const u8 *src = (const u8 *)(c->src + HW_CTRDG_ROM);
	u8 *dst = (u8 *)c->dst;
	u32 len = c->len;

	(void)p;

	while (len > 0) {
		u32 size = (u32)(CARD_ROM_PAGE_SIZE - ((u32)dst & 31));
		if (size >= len)
			size = len;
		if (is_dma_static && !(((u32)dst | size) & 31)) {
#if defined(SDK_ARM9)
			DC_InvalidateRange(dst, size);
			IC_InvalidateRange(dst, size);
#endif
			MI_DmaCopy32(c->dma, src, dst, size);
		} else {
			MI_CpuCopy8(src, dst, size);
		}
		src += size;
		dst += size;
		len -= size;
	}
}

#if defined(SDK_ARM9)
    static void CARDi_ReadPxi (CARDRomStat *p)
    {
        CARDiCommon *const c = &cardi_common;
        do {
            p->cache_page = (u8 *)CARD_ALIGN_HI_BIT(c->src);
            DC_InvalidateRange(p->cache_buf, sizeof(p->cache_buf));
            c->cmd->src = (u32)p->cache_page;
            c->cmd->dst = (u32)p->cache_buf;
            c->cmd->len = sizeof(p->cache_buf);

            if (!CARDi_Request(c, CARD_REQ_READ_ROM, 1))
                break;
            {

                const u32 mod = (u32)(c->src - (u32)p->cache_page);
                u32 len = CARD_ROM_PAGE_SIZE - mod;
                if (len > c->len)
                    len = c->len;
                MI_CpuCopy8(p->cache_buf + mod, (void *)c->dst, len);
                c->src += len;
                c->dst += len;
                c->len -= len;
            }
        } while (c->len > 0);
    }
#endif

u32 CARDi_ReadRomID (void)
{
	CARDRomStat *const p = &rom_stat;
	CARDiCommon *const c = &cardi_common;

	u32 ret = 0;

	SDK_ASSERT(CARD_IsAvailable());
	SDK_ASSERT(CARDi_GetTargetMode() == CARD_TARGET_ROM);

	if (CARDi_IsTrueRom()) {
		OS_TPanic("cannot call CARD_ReadRomID() on (TEG && cartridge)!");
	}

	CARDi_WaitTask(c, NULL, NULL);

#if defined(SDK_ARM9)
	cardi_common.cur_th = OS_GetCurrentThread();
	(void)CARDi_Request(c, CARD_REQ_READ_ID, 1);
	ret = (u32)c->cmd->id;
#endif
	CARDi_ReadEnd();
	return ret;
}

void (*CARDi_GetRomAccessor(void)) (CARDRomStat *)
{
	if (CARDi_IsTrueRom()) {
#if defined(SDK_ARM9)

		return CARDi_ReadPxi;
#else

		return CARDi_ReadCard;
#endif
	} else {
		return CARDi_ReadCartridge;
	}
}

#endif
