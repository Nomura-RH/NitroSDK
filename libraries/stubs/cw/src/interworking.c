#include <nitro/types.h>

void __call_via_r0(void);
void __call_via_r1(void);
void __call_via_r2(void);
void __call_via_r3(void);
void __call_via_r4(void);
void __call_via_r5(void);
void __call_via_r6(void);
void __call_via_r7(void);

SDK_WEAK_SYMBOL asm void __call_via_r0 (void){
    bx r0
}

SDK_WEAK_SYMBOL asm void __call_via_r1 (void)
{
    bx r1
}

SDK_WEAK_SYMBOL asm void __call_via_r2 (void)
{
    bx r2
}

SDK_WEAK_SYMBOL asm void __call_via_r3 (void)
{
    bx r3
}

SDK_WEAK_SYMBOL asm void __call_via_r4 (void)
{
    bx r4
}

SDK_WEAK_SYMBOL asm void __call_via_r5 (void)
{
    bx r5
}

SDK_WEAK_SYMBOL asm void __call_via_r6 (void)
{
    bx r6
}

SDK_WEAK_SYMBOL asm void __call_via_r7 (void)
{
    bx r7
}