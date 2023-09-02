#include <nitro/math.h>
#include <nitro/mi/memory.h>
#include <nitro/mi/cache.h>

void MI_InitCache (MICache * cache, u32 page, void * buffer, u32 length)
{
    SDK_ASSERT(page >= sizeof(u32));

    cache->pagewidth = MATH_CTZ(page);
    cache->valid_total = 0;
    cache->invalid_total = 0;
    cache->loading_total = 0;
    cache->valid = NULL;
    cache->invalid = NULL;
    cache->loading = NULL;

    {
        u32 total = length / (sizeof(MICachePage) + page);
        u8 * buf = (u8 *)buffer;
        MICachePage * inf = (MICachePage *)&buf[total * page];

        cache->invalid_total += total;

        for (; total > 0; --total) {
            inf->offset = 0;
            inf->buffer = buf;
            inf->next = cache->invalid;
            cache->invalid = inf;
            inf += 1;
            buf += page;
        }
    }
}

static void WFSi_TouchCachePages (MICache * cache, u32 head, u32 bitset)
{
    {
        PLATFORM_ENTER_CRITICALSECTION();
        MICachePage ** load;

        for (load = &cache->loading; *load; load = &(*load)->next) {
            MICachePage * p = *load;
            u32 pos = p->offset - head;
            if ((pos < 32) && ((bitset & (1 << pos)) != 0)) {
                bitset &= ~(1UL << pos);
            }
        }

        {
            int rest = MATH_CountPopulation(bitset) - cache->invalid_total;
            if (rest > 0) {
                int del = cache->valid_total;
                MICachePage ** valid;
                for (valid = &cache->valid; *valid; valid = &(*valid)->next) {
                    if (--del < rest) {
                        MICachePage ** pp;
                        for (pp = &cache->invalid; *pp; pp = &(*pp)->next) {
                        }
                        *pp = *valid;
                        *valid = NULL;
                        cache->valid_total -= rest;
                        cache->invalid_total += rest;
                        break;
                    }
                }
            }
        }

        while (cache->invalid && bitset) {
            MICachePage * p = cache->invalid;
            u32 pos = MATH_CTZ(bitset);
            cache->invalid = p->next;
            p->offset = head + pos;
            p->next = NULL;
            *load = p;
            load = &p->next;
            --cache->invalid_total;
            --cache->loading_total;
            bitset &= ~(1UL << pos);
        }

        PLATFORM_LEAVE_CRITICALSECTION();
    }

    if (bitset) {
        static BOOL output_once = FALSE;

        if (!output_once) {
            output_once = TRUE;
            OS_TWarning("not enough cache-page! "
                        "MI_TouchCache() failed. "
                        "(total pages = %d, but need more %d)",
                        cache->invalid_total + cache->valid_total,
                        MATH_CountPopulation(bitset));
        }
    }
}

BOOL MI_ReadCache (MICache * cache, void * buffer, u32 offset, u32 length)
{
    BOOL retval = TRUE;

    const u32 unit = (1UL << cache->pagewidth);
    u32 head = (offset >> cache->pagewidth);
    u32 tail = ((offset + length + unit - 1UL) >> cache->pagewidth);
    u32 pages;
    for (; (pages = MATH_MIN(tail - head, 32UL)), (pages > 0); head += pages) {
        u32 bitset = (1UL << pages) - 1UL;
        {
            PLATFORM_ENTER_CRITICALSECTION();
            MICachePage ** pp;

            for (pp = &cache->valid; *pp && bitset; pp = &(*pp)->next) {
                MICachePage * p = *pp;
                u32 pos = p->offset - head;

                if ((pos < pages) && ((bitset & (1UL << pos)) != 0)) {
                    if (buffer) {
                        u32 len = unit;
                        int src = 0;
                        int dst = (int)((p->offset << cache->pagewidth) - offset);

                        if (dst < 0) {
                            len += dst;
                            src -= dst;
                            dst = 0;
                        }

                        if (dst + len > length) {
                            len = length - dst;
                        }

                        MI_CpuCopy8(&p->buffer[src], &((u8 *)buffer)[dst], len);
                    }

                    bitset &= ~(1UL << pos);
                }
            }

            PLATFORM_LEAVE_CRITICALSECTION();
        }

        if (bitset) {
            retval = FALSE;
            WFSi_TouchCachePages(cache, head, bitset);
        }
    }

    return retval;
}

void MI_LoadCache (MICache * cache, MIDevice * device)
{
    for (;;) {
        MICachePage * p = cache->loading;

        if (!p) {
            break;
        }

        (void)MI_ReadDevice(device, p->buffer,
                            (p->offset << cache->pagewidth),
                            (1UL << cache->pagewidth));

        {
            PLATFORM_ENTER_CRITICALSECTION();
            cache->loading = p->next;
            p->next = cache->valid;
            cache->valid = p;
            ++cache->valid_total;
            ++cache->loading_total;
            PLATFORM_LEAVE_CRITICALSECTION();
        }
    }
}
