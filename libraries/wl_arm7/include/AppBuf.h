#ifndef __APPBUF_H_
#define __APPBUF_H_

#define APP_HEAPBUF_BYTES_PER_PAGE 0x20
// #define	APP_HEAPBUF_PAGE_NUM			(CPU_WRAM_HEAP_SIZE /
// APP_HEAPBUF_BYTES_PER_PAGE) #define	APP_HEAPBUF_STR_PAGE
// (APP_HEAPBUF_STR_ADRS/APP_HEAPBUF_BYTES_PER_PAGE) #define
// APP_HEAPBUF_END_PAGE
// (APP_HEAPBUF_END_ADRS/APP_HEAPBUF_BYTES_PER_PAGE)
#define APP_HEAPBUF_OFFSET (sizeof(APP_HEAPBUF_HEADER))

typedef struct {
    u16 Prev;
    u16 Next;
    u16 Pages;
    u16 UpPages;
    u16 Flag;
    u16 SubFlag;
    u32 BufID;
} APP_HEAPBUF_HEADER, *LPAPP_HEAPBUF_HEADER;

typedef struct {
    LPAPP_HEAPBUF_HEADER Head;
    LPAPP_HEAPBUF_HEADER Tail;
    u16 Count;
    u16 Flag;
    //	u16					SubFlag;
    //	u16					Dummy;
} APP_HEAPBUF_MAN, *LPAPP_HEAPBUF_MAN;
#define WlHeapBufMan APP_HEAPBUF_MAN

typedef struct {
    u32 HeapBufStrAdrs;
    u32 HeapBufEndAdrs;
    u16 UserId;
    u16 CurrLimit;

} APP_HEAP_MAN, *LPAPP_HEAP_MAN;

void App_InitializeHeapBuf(void *pHeapBuf, u32 length);

void *App_AllocateHeapBuf(LPAPP_HEAPBUF_MAN pBufMan, u32 Length);
void App_ReleaseHeapBuf(LPAPP_HEAPBUF_MAN pBufMan, void *pBuf);
void App_MoveHeapBuf(LPAPP_HEAPBUF_MAN pFromMan, LPAPP_HEAPBUF_MAN pToMan, void *pBuf);
void App_CheckHeapBufFlag(LPAPP_HEAPBUF_MAN pBufMan, void *pBuf);

void *App_GetHeapBufNextAdrs(void *pBuf);
void *App_GetHeapBufPrevAdrs(void *pBuf);

void App_ReleaseAllWlHeapBuf(void);
void App_ReleaseAllHeapBuf(LPAPP_HEAPBUF_MAN pHeapBufMan);

void App_DispHeapBufInfo(void *pBuft);
void App_DispHeapBufLinkList(APP_HEAPBUF_MAN *pBufMant);
void App_DispHeapBufManOne(char *pStr, APP_HEAPBUF_MAN *pBufMan);
u32 App_CheckHeapBuf(u32 bDisp);

#define App_GetHeapBufHeadAdrs(_BufMan_) \
    ((u32)(*(LPAPP_HEAPBUF_MAN)_BufMan_).Head + APP_HEAPBUF_OFFSET)
#define App_GetHeapBufTailAdrs(_BufMan_) \
    ((u32)(*(LPAPP_HEAPBUF_MAN)_BufMan_).Tail + APP_HEAPBUF_OFFSET)
#define App_GetHeapBufCount(_BufMan_) ((*(LPAPP_HEAPBUF_MAN)_BufMan_).Count)

#define APP_APP_HEAPBUF_HEAD_NONE (u32)(-1 + APP_HEAPBUF_OFFSET)

extern APP_HEAP_MAN AppHeapMan;

extern APP_HEAPBUF_MAN HeapFreeBuf;
extern WlHeapBufMan TmpBuf;

extern WlHeapBufMan HostIfBuf;
extern WlHeapBufMan IOIfBuf;
extern WlHeapBufMan MaDataReqBuf;
extern WlHeapBufMan MaTstDataReqBuf;
extern WlHeapBufMan MaDataIndBuf;
extern WlHeapBufMan MaMpIndBuf;
extern WlHeapBufMan MaMpEndIndBuf;
extern WlHeapBufMan MaMpAckIndBuf;
extern WlHeapBufMan MaDeQueueIndBuf;

extern WlHeapBufMan HostIfMABuf;
extern WlHeapBufMan HostIfMADataBuf;
extern WlHeapBufMan IOIfMABuf;
extern WlHeapBufMan HostIfMADataPBuf;

extern WlHeapBufMan WlBuf;

#define APP_HEAPBUF_NOUSE 0x00
#define APP_HEAPBUF_FREE  0x01
#define APP_HEAPBUF_TMP   0x02

#define APP_HEAPBUF_HOST_IF         0x03
#define APP_HEAPBUF_IO_IF           0x04
#define APP_HEAPBUF_MA_DATA_REQ     0x05
#define APP_HEAPBUF_MA_TST_DATA_REQ 0x06
#define APP_HEAPBUF_MA_DATA_IND     0x07
#define APP_HEAPBUF_MA_MP_IND       0x08
#define APP_HEAPBUF_MA_MPEND_IND    0x09
#define APP_HEAPBUF_MA_MPACK_IND    0x0a
#define APP_HEAPBUF_MA_DEQ_IND      0x0b

#define APP_HEAPBUF_HOST_IF_MA        0x0c
#define APP_HEAPBUF_HOST_IF_MA_DATA   0x0d
#define APP_HEAPBUF_IO_IF_MA          0x0e
#define APP_HEAPBUF_HOST_IF_MA_DATA_P 0x0f

#define APP_HEAPBUF_WL 0x10

#define APP_HEAPBUF_NOT_ENOUGH_MEMORY (-1)
#define APP_HEAPBUF_NOT_ENOUGH_MEM    APP_HEAPBUF_NOT_ENOUGH_MEMORY
#define APP_HEAPBUF_NOT_FOUND         (-1)

#define APP_HEAPBUF_SUCCESS          0
#define APP_HEAPBUF_NOT_HEAP_POINTER 1
#define APP_HEAPBUF_MISMATCH_FLAG    2

#define APP_PREV_NONE (u16)(-1)
#define APP_NEXT_NONE (u16)(-1)
#define APP_HEAD_NONE (LPAPP_HEAPBUF_HEADER)(-1)
#define APP_TAIL_NONE (LPAPP_HEAPBUF_HEADER)(-1)

#define APP_HEAPBUF_ID 0xBF1D4649

u32 App_CheckHeapBufID(LPAPP_HEAPBUF_HEADER pBufHeader, char *pMsg, u32 line);

#ifdef __APPBUF_C_

void App_AddHeapBuf(LPAPP_HEAPBUF_MAN pTxBufMan,
    LPAPP_HEAPBUF_HEADER pBufHeader);
void App_DeleteHeapBuf(LPAPP_HEAPBUF_MAN pTxBufMan,
    LPAPP_HEAPBUF_HEADER pBufHeader);
void App_UnionHeapBuf(LPAPP_HEAPBUF_HEADER pBase, LPAPP_HEAPBUF_HEADER pNext);

void App_InitHeapBufMan(LPAPP_HEAPBUF_MAN pBufMan, u16 Flag);

#define CalcHeapBufAdrs(_page_) \
    (AppHeapMan.HeapBufStrAdrs + (u32)_page_ * (u32)APP_HEAPBUF_BYTES_PER_PAGE)

#endif // __BUFMAN_C_
#endif // __BUFMAN_H_
