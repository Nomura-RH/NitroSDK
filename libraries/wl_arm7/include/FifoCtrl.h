#ifndef __FIFO_CTRL_H_
#define __FIFO_CTRL_H_

void InitializeFifo(void);
u32 FifoWrite8(u32 data);
u32 FifoWrite8Wait(u32 data);
u32 FifoWriteBuf(u8 *datap, u32 count);
u32 FifoWriteBufWait(u8 *datap, u32 count);
u32 FifoFlashBufWait(void);

void FifoRecvHandler(void);
void FifoSendHandler(void);

typedef struct {
    u16 RxEnable;
    u16 dm[3];
} FIFO_CTRL;

extern FIFO_CTRL FifoCtrl;

#define REG_MAINP_FIFO_CNT (HW_REG_BASE + 0x184)
#define REG_SUBP_FIFO_CNT  (HW_REG_BASE + 0x184)

#define MPFIFO_SEND_EMPTY     0x0001
#define MPFIFO_SEND_FULL      0x0002
#define MPFIFO_SEND_IF_ENABLE 0x0004
#define MPFIFO_SEND_CLEAR     0x0008
#define MPFIFO_RECV_EMPTY     0x0100
#define MPFIFO_RECV_FULL      0x0200
#define MPFIFO_RECV_IF_ENABLE 0x0400
#define MPFIFO_ENABLE         0x8000

#define SPFIFO_SEND_EMPTY     0x0001
#define SPFIFO_SEND_FULL      0x0002
#define SPFIFO_SEND_IF_ENABLE 0x0004
#define SPFIFO_SEND_CLEAR     0x0008
#define SPFIFO_RECV_EMPTY     0x0100
#define SPFIFO_RECV_FULL      0x0200
#define SPFIFO_RECV_IF_ENABLE 0x0400
#define SPFIFO_ENABLE         0x8000

#define REG_FIFO_CNT REG_MAINP_FIFO_CNT

#define FIFO_ENABLE MPFIFO_ENABLE

#define FIFO_SEND_EMPTY     MPFIFO_SEND_EMPTY
#define FIFO_SEND_FULL      MPFIFO_SEND_FULL
#define FIFO_SEND_IF_ENABLE MPFIFO_SEND_IF_ENABLE
#define FIFO_SEND_CLEAR     MPFIFO_SEND_CLEAR
#define FIFO_RECV_EMPTY     MPFIFO_RECV_EMPTY
#define FIFO_RECV_FULL      MPFIFO_RECV_FULL
#define FIFO_RECV_IF_ENABLE MPFIFO_RECV_IF_ENABLE

#define FIFO_SEND_INTR_FLAG MPFIFO_SEND_INTR_FLAG
#define FIFO_RECV_INTR_FLAG MPFIFO_RECV_INTR_FLAG

#ifdef __FIFO_CTRL_C_

#include "Compati.h"

#define MAX_FIFO_BUF (16 * 4 - 1)
// #define	MAX_FIFO_BUF	(64/sizeof(u16) - 1)

typedef struct {
    u8 count;
    u8 buf[MAX_FIFO_BUF];
    //	u16	count;
    //	u16	buf[MAX_FIFO_BUF];
} FIFO_BUF;

#define ClearTxFifoBuf APP_DmaFill32(0, &TxFifoBuf, sizeof(TxFifoBuf))
#define ClearRxFifoBuf APP_DmaFill32(0, &RxFifoBuf, sizeof(RxFifoBuf))

#define APP_DmaSet(dmaNo, Srcp, Destp, DmaCntData)                    \
    {                                                                 \
        vu32 *dmaCntp = (vu32 *)((u32)REG_DMA0SAD_ADDR + dmaNo * 12); \
        OSIrqMask BkIe = OS_DisableIrqMask(APP_DMA_MASK);             \
        dmaCntp[0] = (vu32)(Srcp);                                    \
        dmaCntp[1] = (vu32)(Destp);                                   \
        dmaCntp[2] = (vu32)(DmaCntData);                              \
        OS_EnableIrqMask(BkIe);                                       \
    }

inline void FifoSend(u32 *srcp, u32 size)
{
    APP_DmaSet(APP_DMA_CHANNEL, srcp, REG_SEND_FIFO, (DMA_ENABLE | DMA_TIMMING_IMM | DMA_SRC_INC | DMA_DEST_FIX | DMA_32BIT_BUS | (size / 4)));
    APP_DmaWait();
    MI_CpuFillFast(srcp, 0, size);
}

inline void FifoRecv(u32 *dest, u32 size)
{
    APP_DmaSet(APP_DMA_CHANNEL, REG_RECV_FIFO, dest, (DMA_ENABLE | DMA_TIMMING_IMM | DMA_SRC_FIX | DMA_DEST_INC | DMA_32BIT_BUS | (size / 4)));
}

#endif // __FIFO_CTRL_C_
#endif // __FIFO_CTRL_C_
