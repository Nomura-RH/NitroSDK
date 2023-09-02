#include <nitro.h>

void MI_SetAgbCartridgeFastestRomCycle (MICartridgeRomCycle1st * prev1st, MICartridgeRomCycle2nd * prev2nd)
{
    if (prev1st) {
        *prev1st = MI_GetCartridgeRomCycle1st();
    }

    if (prev2nd) {
        *prev2nd = MI_GetCartridgeRomCycle2nd();
    }

    MI_SetCartridgeRomCycle1st(MI_CTRDG_ROMCYCLE1_8);
    MI_SetCartridgeRomCycle2nd(MI_CTRDG_ROMCYCLE2_4);
}
