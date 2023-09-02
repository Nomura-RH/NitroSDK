#include <nitro/os.h>

#ifndef OS_PROFILE_AVAILABLE
    SDK_WEAK_SYMBOL asm void __PROFILE_ENTRY (void)
    {
        bx lr
    }

    SDK_WEAK_SYMBOL asm void __PROFILE_EXIT (void)
    {
        bx lr
    }
#endif
