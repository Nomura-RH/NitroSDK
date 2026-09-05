#include <nitro/ctrdg.h>
#include <nitro/os/common/system.h>

SDK_WEAK_SYMBOL void OS_Terminate(void)
{
    CTRDG_VibPulseEdgeUpdate(NULL);

    while (TRUE) {
        (void)OS_DisableInterrupts();
        OS_Halt();
    }
}

SDK_WEAK_SYMBOL void OS_Exit(int status)
{
#ifdef SDK_FINALROM
#pragma unused(status)
#endif
    (void)OS_DisableInterrupts();
    OS_Printf("\n" OS_EXIT_STRING, status);
    OS_Terminate();
}
