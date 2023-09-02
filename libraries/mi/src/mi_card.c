#include <nitro/os.h>
#include <nitro/mi/card.h>

typedef struct {
    BOOL is_init;
    u16 lock_id;
    u16 padding;
    MIDmaCallback dma_callback;
} MIi_CardParam;

static MIi_CardParam mii_card_param;

static void MIi_InitCard (void)
{
    MIi_CardParam * const p = &mii_card_param;
    OSIntrMode bak_psr = OS_DisableInterrupts();

    if (!p->is_init) {
        s32 lock_id = OS_GetLockID();

#ifndef SDK_FINALROM
        if (lock_id < 0) {
            OS_Panic("Invalid lock ID.");
        }
#endif
        p->is_init = TRUE;
        p->lock_id = (u16)lock_id;
    }

    (void)OS_RestoreInterrupts(bak_psr);
}

void MIi_LockCard (void)
{
    MIi_InitCard();
    CARD_LockRom(mii_card_param.lock_id);
}

void MIi_UnlockCard (void)
{
    SDK_ASSERT(mii_card_param.is_init);
    CARD_UnlockRom(mii_card_param.lock_id);
}

static void MIi_OnAsyncEnd (void * arg)
{
    MIi_UnlockCard();
    {
        MIDmaCallback func = mii_card_param.dma_callback;
        mii_card_param.dma_callback = NULL;
        if (func)
            (*func)(arg);
    }
}

void MIi_ReadCardAsync (u32 dmaNo, const void * src, void * dst, u32 size, MIDmaCallback callback, void * arg)
{
    MIi_LockCard();
    mii_card_param.dma_callback = callback;
    (void)CARD_ReadRomAsync(dmaNo, src, dst, size, MIi_OnAsyncEnd, arg);
}
