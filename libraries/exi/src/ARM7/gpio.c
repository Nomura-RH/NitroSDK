#include "nitro/exi.h"
#include "nitro/exi/ARM7/genPort.h"

void EXIi_SetBitRcnt0L(u16 mask, u16 data)
{
    EXIi_SetRcnt0L(EXIi_GetRcnt0L() & ~mask | data);
}

void EXIi_SetBitRcnt0H(u16 mask, u16 data)
{
    EXIi_SetRcnt0H(EXIi_GetRcnt0H() & ~mask | data);
}

void EXIi_SetBitRcnt1(u16 mask, u16 data)
{
    EXIi_SetRcnt1(EXIi_GetRcnt1() & ~mask | data);
}

u16 EXIi_GetBitRcnt0L(u16 mask)
{
    return EXIi_GetRcnt0L() & mask;
}

u16 EXIi_GetBitRcnt0H(u16 mask)
{
    return EXIi_GetRcnt0H() & mask;
}

u16 EXIi_GetBitRcnt1(u16 mask)
{
    return EXIi_GetRcnt1() & mask;
}

void EXIi_SelectRcnt(EXIGpioIF type)
{
    EXIi_SetBitRcnt0L(EXI_GPIOIF_MASK, type);
}

void EXIi_SendBitRcnt0L(u16 mask, u16 data)
{
    data |= (mask << 4);
    mask |= (mask << 4);
    EXIi_SetBitRcnt0L(mask, data);
}

void EXIi_SendBitRcnt0H(u16 mask, u16 data)
{
    data |= (mask << 8);
    mask |= (mask << 8);
    EXIi_SetBitRcnt0H(mask, data);
}

void EXIi_SendBitRcnt1(u16 mask, u16 data)
{
    data |= (mask << 4);
    mask |= (mask << 4);
    EXIi_SetBitRcnt1(mask, data);
}

u16 EXIi_RecvBitRcnt0L(u16 mask)
{
    EXIi_SetBitRcnt0L(mask << 4, 0);
    return EXIi_GetBitRcnt0L(mask);
}

u16 EXIi_RecvBitRcnt0H(u16 mask)
{
    EXIi_SetBitRcnt0H(mask << 8, 0);
    return EXIi_GetBitRcnt0H(mask);
}

u16 EXIi_RecvBitRcnt1(u16 mask)
{
    EXIi_SetBitRcnt1(mask << 4, 0);
    return EXIi_GetBitRcnt1(mask);
}