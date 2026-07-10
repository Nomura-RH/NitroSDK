#ifndef __GLOBAL_H_
#define __GLOBAL_H_

#ifdef __INSYSROM__
#pragma warn_padding reset
#else
#pragma warn_padding off
#endif

#define BIT_0  0x01
#define BIT_1  0x02
#define BIT_2  0x04
#define BIT_3  0x08
#define BIT_4  0x10
#define BIT_5  0x20
#define BIT_6  0x40
#define BIT_7  0x80
#define BIT_8  0x0100
#define BIT_9  0x0200
#define BIT_10 0x0400
#define BIT_11 0x0800
#define BIT_12 0x1000
#define BIT_13 0x2000
#define BIT_14 0x4000
#define BIT_15 0x8000
#define BIT_16 0x00010000
#define BIT_17 0x00020000
#define BIT_18 0x00040000
#define BIT_19 0x00080000
#define BIT_20 0x00100000
#define BIT_21 0x00200000
#define BIT_22 0x00400000
#define BIT_23 0x00800000
#define BIT_24 0x01000000
#define BIT_25 0x02000000
#define BIT_26 0x04000000
#define BIT_27 0x08000000
#define BIT_28 0x10000000
#define BIT_29 0x20000000
#define BIT_30 0x40000000
#define BIT_31 0x80000000

#define SwapEndianWord(_x_) (u16)((_x_ >> 8) | ((_x_ & 0xFF) << 8))
#define IP(_x0_, _x1_, _x2_, _x3_) \
    (u32)((u32)_x0_ | ((u32)_x1_) << 8 | ((u32)_x2_) << 16 | ((u32)_x3_) << 24)
#define IP_A(_x0_) (u32)(((u32)_x0_ >> 0) & 0x0FF)
#define IP_B(_x0_) (u32)(((u32)_x0_ >> 8) & 0x0FF)
#define IP_C(_x0_) (u32)(((u32)_x0_ >> 16) & 0x0FF)
#define IP_D(_x0_) (u32)(((u32)_x0_ >> 24) & 0x0FF)
#define WriteByte(_p_, _x_)                                                          \
    {                                                                                \
        if ((u32)_p_ & 1)                                                            \
            *(u16 *)((u32)_p_ - 1) = (*(u16 *)((u32)_p_ - 1) & 0x00FF) | (_x_ << 8); \
        else                                                                         \
            *(u16 *)(_p_) = (*(u16 *)(_p_) & 0xFF00) | (_x_ & 0x00FF);               \
    }
#define ReadByte(_p_, _x_)                           \
    {                                                \
        if ((u32)_p_ & 1)                            \
            _x_ = (u8)(*(u16 *)((u32)_p_ - 1) >> 8); \
        else                                         \
            _x_ = (u8)(*(u16 *)(_p_));               \
    }

#define WriteByteData(_p_, _x_) *(u8 *)_p_ = _x_
#define ReadByteData(_p_, _x_)  _x_ = *(u8 *)_p_

#define M_strcpy(_dest_, _src_, _len_)          \
    {                                           \
        u8 *___pdest = (u8 *)_dest_;            \
        u8 *___psrc = (u8 *)_src_;              \
        for (_len_ = 0; *___psrc != 0; _len_++) \
            *___pdest++ = *___psrc++;           \
        *___pdest = 0;                          \
    }

#define WLLIB_DMA_CHANNEL wlMan->DmaChannel
#define APP_DMA_CHANNEL   3

#define WLLIB_DMA_MASK (OS_IE_WIRELESS)
#define APP_DMA_MASK \
    (OS_IE_V_BLANK | OS_IE_SPFIFO_SEND | OS_IE_SPFIFO_RECV | OS_IE_SUBP | OS_IE_WIRELESS | OS_IE_TIMER1 | 0)

#define WLLIB_M_DmaCopy(Srcp, Destp, Size, Bit)                  \
    {                                                            \
        OSIrqMask BkIe = OS_DisableIrqMask(WLLIB_DMA_MASK);      \
        MI_DmaCopy##Bit##(WLLIB_DMA_CHANNEL, Srcp, Destp, Size); \
        OS_EnableIrqMask(BkIe);                                  \
    }

inline void APP_DmaCopy32(void *Srcp, void *Destp, u32 Size)
{
    OSIrqMask BkIe = OS_DisableIrqMask(APP_DMA_MASK);
    MI_DmaCopy32(APP_DMA_CHANNEL, (void *)(Srcp), (void *)(Destp), Size);
    OS_EnableIrqMask(BkIe);
}

inline void APP_DmaCopy16(void *Srcp, void *Destp, u32 Size)
{
    OSIrqMask BkIe = OS_DisableIrqMask(APP_DMA_MASK);
    MI_DmaCopy16(APP_DMA_CHANNEL, (void *)(Srcp), (void *)(Destp), Size);
    OS_EnableIrqMask(BkIe);
}

inline void APP_DmaFill32(u32 Data, void *Destp, u32 Size)
{
    OSIrqMask BkIe = OS_DisableIrqMask(APP_DMA_MASK);
    MI_DmaFill32(APP_DMA_CHANNEL, (void *)Destp, (u32)Data, (u32)(Size));
    OS_EnableIrqMask(BkIe);
}

inline void APP_DmaFill16(u32 Data, void *Destp, u32 Size)
{
    OSIrqMask BkIe = OS_DisableIrqMask(APP_DMA_MASK);
    MI_DmaFill16(APP_DMA_CHANNEL, (void *)Destp, (u32)Data, (u32)(Size));
    OS_EnableIrqMask(BkIe);
}

#define WLLIB_DmaWait() MI_WaitDma(WLLIB_DMA_CHANNEL)

#define APP_DmaWait() MI_WaitDma(APP_DMA_CHANNEL)

extern u32 SysDbgLevel;
extern u32 WmDbgLevel;
extern u32 HostDbgLevel;
extern u32 NetDbgLevel;
extern u16 ddoDbgLevel;
extern u16 testDbgLevel;

void MyPrintf(const char *format, ...);
void MyPuts(const char *pStr);
void MyLcdBg0Printf(const char *pStr, ...);
void MyLcdBg2Printf(const char *pStr, ...);
void MySubLcdBg0Printf(const char *format, ...);
void MyLcdBg1Printf(const char *format, ...);
void MyUartPrintf(const char *pStr, ...);
int myfputc(int ch, u32 f);

#define printf MyPrintf
#define puts   MyPuts
#undef putchar
#define putchar(c) myfputc(c, 0)

#define LevelPrint(dbg, l, ...) \
    if (dbg & l)                \
    printf(__VA_ARGS__)
#define LevelPuts(dbg, l, _x_) \
    if (dbg & l)               \
    puts(_x_)
#define LevelPutchar(dbg, l, _x_) \
    if (dbg & l)                  \
    putchar(_x_)

// #define	HostPrint(l,_x_)			if( HostDbgLevel & l )
// printf(__VA_ARGS__) #define	HostPrint(l,_x_)

#define HostPrint(l, ...) \
    if (HostDbgLevel & l) \
    printf(__VA_ARGS__)
#define HostPuts(l, _x_)  \
    if (HostDbgLevel & l) \
    puts(_x_)
#define HostPutchar(l, _x_) \
    if (HostDbgLevel & l)   \
    putchar(_x_)

// #define	HostPrint(l,_x_)			if( HostDbgLevel & l )
// fprintf(__VA_ARGS__) #define	HostPuts(l,_x_)				if(
// HostDbgLevel & l ) fputs(_x_) #define	HostPutchar(l,_x_)
// if( HostDbgLevel & l ) fputchar(_x_)

#define WlPrint(l, ...)        \
    if (wlMan->WlDbgLevel & l) \
    printf(__VA_ARGS__)
#define WlPuts(l, _x_)         \
    if (wlMan->WlDbgLevel & l) \
    puts(_x_)
#define WlPutchar(l, _x_)      \
    if (wlMan->WlDbgLevel & l) \
    putchar(_x_)

#define WmPrint(l, ...) \
    if (WmDbgLevel & l) \
    printf(__VA_ARGS__)
#define WmPuts(l, _x_)  \
    if (WmDbgLevel & l) \
    puts(_x_)
#define WmPutchar(l, _x_) \
    if (WmDbgLevel & l)   \
    putchar(_x_)

#define NetPrint(l, ...) \
    if (NetDbgLevel & l) \
    printf(__VA_ARGS__)
#define NetPuts(l, _x_)  \
    if (NetDbgLevel & l) \
    puts(_x_)
#define NetPutchar(l, _x_) \
    if (NetDbgLevel & l)   \
    putchar(_x_)

#define Print(...)   printf(__VA_ARGS__)
#define Puts(_x_)    puts(_x_)
#define Putchar(_x_) putchar(_x_)

#define fPrint(...)   fprintf(__VA_ARGS__)
#define fPuts(_x_)    fputs(_x_)
#define fPutchar(_x_) fputchar(_x_)

#define DbgLevelPrint(l, ...)
#define DbgLevelPuts(l, _x_)
#define DbgLevelPutchar(l, _x_)

#define DbgSysPrint(l, ...)
#define DbgSysPuts(l, _x_)
#define DbgSysPutchar(l, _x_)

#define DbgHostPrint(l, ...)
#define DbgHostPuts(l, _x_)
#define DbgHostPutchar(l, _x_)

#define DbgWlPrint(l, ...)
#define DbgWlPuts(l, _x_)
#define DbgWlPutchar(l, _x_)

#define DbgWmPrint(l, ...)
#define DbgWmPuts(l, _x_)
#define DbgWmPutchar(l, _x_)

#define DbgNetPrint(l, ...)
#define DbgNetPuts(l, _x_)
#define DbgNetPutchar(l, _x_)

#define DbgPrint(...)
#define DbgPuts(_x_)
#define DbgPutchar(_x_)

#define DbgPutTestPinLevel(l, _x_)
#define DbgSetTestPinLevel(l, _x_)
#define DbgClrTestPinLevel(l, _x_)
#define DbgInvTestPinLevel(l, _x_)
/*
#define	DbgPutTestPinLevel(l,_x_)	if( TestPinLevel & (l) )
DDO_SetData(_x_); #define	DbgSetTestPinLevel(l,_x_)	if( TestPinLevel
& (l) ) DDO_OrData(_x_); #define	DbgClrTestPinLevel(l,_x_)	if(
TestPinLevel & (l) ) DDO_AndData(~_x_); #define	DbgInvTestPinLevel(l,_x_)
if( TestPinLevel & (l) ) DDO_XorData(_x_);
*/
#define DbgPutTestPin(_x_)
#define DbgSetTestPin(_x_)
#define DbgClrTestPin(_x_)
#define DbgInvTestPin(_x_)

#define DbgPutDDO(l, _x_)
#define DbgSetDDO(l, _x_)
#define DbgClrDDO(l, _x_)
#define DbgInvDDO(l, _x_)

#define ASSERT(_x_)

#endif // __GLOBAL_H_
