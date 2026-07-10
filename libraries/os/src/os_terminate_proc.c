#include <nitro/os/common/system.h>
#ifdef SDK_ARM7
#include <nitro/ctrdg.h>
#endif

SDK_WEAK_SYMBOL void OS_Terminate (void)
{
#ifdef SDK_ARM7
    CTRDG_VibPulseEdgeUpdate(NULL);
#endif
    while (1) {
        (void)OS_DisableInterrupts();
        OS_Halt();
    }
}

#ifdef SDK_ARM9
#include <nitro/code32.h>

SDK_WEAK_SYMBOL asm void OS_Halt (void)
{
    mov r0, #0
    mcr p15, 0, r0, c7, c0, 4
    bx lr
}

#include <nitro/codereset.h>

SDK_WEAK_SYMBOL void OS_Exit (int status)
{
#ifdef SDK_FINALROM
#pragma unused(status)
#endif
    (void)OS_DisableInterrupts();
    OS_Printf("\n" OS_EXIT_STRING, status);
    OS_Terminate();
}
#endif
