#ifndef __BUFMAN_H_
#define __BUFMAN_H_

#define HEAPBUF_BYTES_PER_PAGE 0x10
// #define	HEAPBUF_PAGE_NUM			(CPU_WRAM_HEAP_SIZE /
// HEAPBUF_BYTES_PER_PAGE) #define	HEAPBUF_STR_PAGE
// (HEAPBUF_STR_ADRS/HEAPBUF_BYTES_PER_PAGE) #define	HEAPBUF_END_PAGE
// (HEAPBUF_END_ADRS/HEAPBUF_BYTES_PER_PAGE)
#define HEAPBUF_OFFSET (sizeof(HEAPBUF_HEADER))

typedef struct {
    u32 Prev;
    u32 Next;
    u16 Flag;
    u16 BufID;
} HEAPBUF_HEADER, *LPHEAPBUF_HEADER;

typedef struct {
    LPHEAPBUF_HEADER Head;
    LPHEAPBUF_HEADER Tail;
    u16 Count;
    u16 Flag;
} HEAPBUF_MAN, *LPHEAPBUF_MAN;

typedef struct {
    u32 heapType;

    union {
        struct {
            OSArenaId id;
            OSHeapHandle heapHandle;
        } os;
        struct {
            u32 (*alloc)(u32 size);
            u32 (*free)(void *p);
        } ext;
    } func;

} HEAP_INFO, *LPHEAP_INFO;

typedef struct {
    HEAP_INFO HeapInfo;

    HEAPBUF_MAN TmpBuf;

    HEAPBUF_MAN TxPri[3];
    HEAPBUF_MAN MaDataCfm;

    HEAPBUF_MAN RxData;
    HEAPBUF_MAN RxBeacon;
    HEAPBUF_MAN RxManCtrl;
    HEAPBUF_MAN Defrag;

    HEAPBUF_MAN ToWM;
    HEAPBUF_MAN RequestCmd;

    HEAPBUF_MAN man_rsv[4];
} HEAP_MAN, *LPHEAP_MAN;

void InitializeHeapBuf(LPHEAP_INFO pHeapInfo);
u32 InitHeapBuf(LPHEAPBUF_MAN pHeapBufMan);

void *AllocateHeapBuf(LPHEAPBUF_MAN pBufMan, u32 Length);
u32 ReleaseHeapBuf(LPHEAPBUF_MAN pBufMan, void *pBuf);
u32 MoveHeapBuf(LPHEAPBUF_MAN pFromMan, LPHEAPBUF_MAN pToMan, void *pBuf);
u32 CheckHeapBufFlag(LPHEAPBUF_MAN pBufMan, void *pBuf);
u32 DeleteHeapBuf(LPHEAPBUF_MAN pTxBufMan, void *pBuf);
u32 NewHeapBuf(LPHEAPBUF_MAN pBufMan, void *pBuf);

void *GetHeapBufNextAdrs(void *pBuf);
void *GetHeapBufPrevAdrs(void *pBuf);

void ReleaseAllWlHeapBuf(void);
void ReleaseAllHeapBuf(LPHEAPBUF_MAN pHeapBufMan);

#define GetHeapBufHeadAdrs(_BufMan_) ((u32)(*(LPHEAPBUF_MAN)_BufMan_).Head)
#define GetHeapBufTailAdrs(_BufMan_) ((u32)(*(LPHEAPBUF_MAN)_BufMan_).Tail)
#define GetHeapBufCount(_BufMan_)    ((*(LPHEAPBUF_MAN)_BufMan_).Count)

#define HEAPBUF_HEAD_NONE (u32)(-1)

#define HEAPBUF_NOUSE       0x00
#define HEAPBUF_FREE        0x01
#define HEAPBUF_TMP         0x02
#define HEAPBUF_TXPRI0      0x03
#define HEAPBUF_TXPRI1      0x04
#define HEAPBUF_TXPRI2      0x05
#define HEAPBUF_MADATACFM   0x06
#define HEAPBUF_RXDATA      0x07
#define HEAPBUF_RXBEACON    0x08
#define HEAPBUF_RXMANCTRL   0x09
#define HEAPBUF_DEFRAG      0x0A
#define HEAPBUF_TOWM        0x0B
#define HEAPBUF_REQUEST_CMD 0x0C

#define HEAPBUF_NOT_ENOUGH_MEMORY (NULL)
#define HEAPBUF_NOT_FOUND         (NULL)

#define HEAPBUF_SUCCESS          0
#define HEAPBUF_NOT_HEAP_POINTER 1
#define HEAPBUF_MISMATCH_FLAG    2

#define HEAPBUF_ID 0xBF1D

#ifdef __BUFMAN_C_

#define PREV_NONE (u32)(-1)
#define NEXT_NONE (u32)(-1)
#define HEAD_NONE (LPHEAPBUF_HEADER)(-1)
#define TAIL_NONE (LPHEAPBUF_HEADER)(-1)

void InitHeapBufMan(LPHEAPBUF_MAN pBufMan, u16 Flag);

u32 AddHeapBuf(LPHEAPBUF_MAN pTxBufMan, void *pBuf);

#define CheckHeapBufID(pBufHeader, pMsg, line) \
    {                                          \
        if (pBufHeader->BufID != HEAPBUF_ID)   \
            return HEAPBUF_NOT_HEAP_POINTER;   \
    }

#endif // __BUFMAN_C_
#endif // __BUFMAN_H_
