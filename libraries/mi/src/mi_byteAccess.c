#include <nitro/misc.h>
#include <nitro/mi/byteAccess.h>

#ifdef SDK_TEG
    u8 MI_ReadByte (const void * address)
    {
        if ((u32)address & 1) {
            return (u8)(*(u16 *)((u32)address & ~1) >> 8);
        } else {
            return (u8)(*(u16 *)address & 0xff);
        }
    }
#endif

#ifdef SDK_TEG
    void MI_WriteByte (void * address, u8 value)
    {
        u16 val = *(u16 *)((u32)address & ~1);

        if ((u32)address & 1) {
            *(u16 *)((u32)address & ~1) = (u16)(((value & 0xff) << 8) | (val & 0xff));
        } else {
            *(u16 *)((u32)address & ~1) = (u16)((val & 0xff00) | (value & 0xff));
        }
    }
#endif