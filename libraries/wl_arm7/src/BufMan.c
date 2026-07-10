#define __BUFMAN_C_
#define __INSYSROM__

#include "WlSys.h"

#include "WlLib.h"
#include "BufMan.h"
#include "TaskMan.h"
#include "Param.h"

void *AllocateHeapBuf(LPHEAPBUF_MAN pBufMan, u32 Length)
{
    u32 AllocPages;
    LPHEAPBUF_HEADER pAllocBuf;
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;

    if (Length == 0) {
        return (void *)HEAPBUF_NOT_ENOUGH_MEMORY;
    }

    AllocPages = Length + sizeof(HEAPBUF_HEADER);

    switch (pHeapMan->HeapInfo.heapType) {
    case WL_HEAP_OS:
        pAllocBuf = (LPHEAPBUF_HEADER)OS_AllocFromHeap(pHeapMan->HeapInfo.func.os.id, pHeapMan->HeapInfo.func.os.heapHandle, AllocPages);
        break;

    case WL_HEAP_EXT:
        pAllocBuf = (LPHEAPBUF_HEADER)(*pHeapMan->HeapInfo.func.ext.alloc)(AllocPages);
        break;
    }

    if (pAllocBuf != NULL) {
        pAllocBuf->BufID = HEAPBUF_ID;
        pAllocBuf->Flag = HEAPBUF_NOUSE;

        AddHeapBuf(pBufMan, pAllocBuf);

        return (void *)pAllocBuf;
    }

    return NULL;
}

u32 ReleaseHeapBuf(LPHEAPBUF_MAN pBufMan, void *pBuf)
{
    LPHEAPBUF_HEADER pBufHeader = (LPHEAPBUF_HEADER)pBuf;
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    u32 sts;

    CheckHeapBufID(pBufHeader, pMsg, line);

    sts = DeleteHeapBuf(pBufMan, pBufHeader);
    if (sts) {
        goto release_err;
    }

    switch (pHeapMan->HeapInfo.heapType) {
    case WL_HEAP_OS:
        OS_FreeToHeap(pHeapMan->HeapInfo.func.os.id, pHeapMan->HeapInfo.func.os.heapHandle, pBufHeader);
        break;

    case WL_HEAP_EXT:
        (*pHeapMan->HeapInfo.func.ext.free)(pBufHeader);
        break;
    }

release_err:
    return sts;
}

u32 MoveHeapBuf(LPHEAPBUF_MAN pFromMan, LPHEAPBUF_MAN pToMan, void *pBuf)
{
    LPHEAPBUF_HEADER pBufHeader = (LPHEAPBUF_HEADER)pBuf;
    u32 sts;
    u32 x;

    CheckHeapBufID(pBufHeader, pMsg, line);

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    sts = DeleteHeapBuf(pFromMan, pBufHeader);
    if (sts) {
        goto move_err;
    }
    sts = AddHeapBuf(pToMan, pBufHeader);

move_err:
    OS_EnableIrqMask(x);

    return sts;
}

u32 NewHeapBuf(LPHEAPBUF_MAN pBufMan, void *pBuf)
{
    LPHEAPBUF_HEADER pBufHeader = (LPHEAPBUF_HEADER)pBuf;
    LPHEAPBUF_HEADER pTailBuf;
    u32 x;

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (pBufMan->Count == 0) {
        pBufHeader->Prev = PREV_NONE;
        pBufMan->Head = pBufHeader;
    } else {
        pTailBuf = pBufMan->Tail;
        pBufHeader->Prev = (u32)pTailBuf;
        pTailBuf->Next = (u32)pBufHeader;
    }

    pBufHeader->Next = NEXT_NONE;
    pBufHeader->Flag = pBufMan->Flag;
    pBufHeader->BufID = HEAPBUF_ID;

    pBufMan->Tail = pBufHeader;
    pBufMan->Count++;

    OS_EnableIrqMask(x);

    return HEAPBUF_SUCCESS;
}

u32 AddHeapBuf(LPHEAPBUF_MAN pBufMan, void *pBuf)
{
    LPHEAPBUF_HEADER pBufHeader = (LPHEAPBUF_HEADER)pBuf;
    LPHEAPBUF_HEADER pTailBuf;
    u32 x;

    CheckHeapBufID(pBufHeader, pMsg, line);

    if (pBufHeader->Flag != HEAPBUF_NOUSE) {
        DbgWlPrint(B_WL_DBG_MSG, "Can't add buffers.(%x)\r\n", wlMan->TaskMan.CurrTaskID);
        DbgPrint("[%08x:%x:%x]", (u32)pBufHeader, pBufMan->Flag, pBufHeader->Flag);
        return HEAPBUF_MISMATCH_FLAG;
    }

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    if (pBufMan->Count == 0) {
        pBufHeader->Prev = PREV_NONE;
        pBufMan->Head = pBufHeader;
    } else {
        pTailBuf = pBufMan->Tail;
        pBufHeader->Prev = (u32)pTailBuf;
        pTailBuf->Next = (u32)pBufHeader;
    }

    pBufHeader->Next = NEXT_NONE;
    pBufHeader->Flag = pBufMan->Flag;

    pBufMan->Tail = pBufHeader;
    pBufMan->Count++;

    OS_EnableIrqMask(x);

    return HEAPBUF_SUCCESS;
}

u32 DeleteHeapBuf(LPHEAPBUF_MAN pBufMan, void *pBuf)
{
    LPHEAPBUF_HEADER pBufHeader = (LPHEAPBUF_HEADER)pBuf;
    u32 x;

    CheckHeapBufID(pBufHeader, pMsg, line);

    if (pBufHeader->Flag != pBufMan->Flag) {
        return HEAPBUF_MISMATCH_FLAG;
    }

    x = OS_DisableIrqMask(OS_IE_WIRELESS);

    pBufMan->Count--;

    if (pBufMan->Count == 0) {
        pBufMan->Head = HEAD_NONE;
        pBufMan->Tail = TAIL_NONE;
    } else {
        if (pBufHeader == pBufMan->Head) {
            pBufMan->Head = (LPHEAPBUF_HEADER)pBufHeader->Next;
            pBufMan->Head->Prev = PREV_NONE;
        } else if (pBufHeader == pBufMan->Tail) {
            pBufMan->Tail = (LPHEAPBUF_HEADER)pBufHeader->Prev;
            pBufMan->Tail->Next = NEXT_NONE;
        } else {
            ((LPHEAPBUF_HEADER)pBufHeader->Next)->Prev = pBufHeader->Prev;
            ((LPHEAPBUF_HEADER)pBufHeader->Prev)->Next = pBufHeader->Next;
        }
    }

    pBufHeader->Flag = HEAPBUF_NOUSE;

    OS_EnableIrqMask(x);

    return HEAPBUF_SUCCESS;
}

void *GetHeapBufNextAdrs(void *pBuf)
{
    LPHEAPBUF_HEADER pHeader = (LPHEAPBUF_HEADER)pBuf;

    return (void *)(pHeader->Next);
}

void *GetHeapBufPrevAdrs(void *pBuf)
{
    LPHEAPBUF_HEADER pHeader = (LPHEAPBUF_HEADER)pBuf;

    return (void *)(pHeader->Prev);
}

u32 CheckHeapBufFlag(LPHEAPBUF_MAN pBufMan, void *pBuf)
{
    LPHEAPBUF_HEADER pBufHeader = (LPHEAPBUF_HEADER)pBuf;

    CheckHeapBufID(pBufHeader, pMsg, line);

    if (pBufHeader->Flag != pBufMan->Flag) {
        DbgPrint("Chk Buffer flag err[%08x][%04x!=%04x](line %u in %s)\r\n", (u32)pBufMan, pBufHeader->Flag, pBufMan->Flag, line, pMsg);
        return HEAPBUF_MISMATCH_FLAG;
    }

    return HEAPBUF_SUCCESS;
}

void InitHeapBufMan(LPHEAPBUF_MAN pBufMan, u16 Flag)
{
    pBufMan->Head = HEAD_NONE;
    pBufMan->Tail = TAIL_NONE;
    pBufMan->Count = 0;
    pBufMan->Flag = Flag;
}

void InitializeHeapBuf(LPHEAP_INFO pHeapInfo)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;
    LPWORK_PARAM pWork = &wlMan->Work;

    pHeapMan->HeapInfo.heapType = pHeapInfo->heapType;
    pHeapMan->HeapInfo.func.os.id = pHeapInfo->func.os.id;
    pHeapMan->HeapInfo.func.os.heapHandle = pHeapInfo->func.os.heapHandle;

    InitHeapBufMan(&pHeapMan->TmpBuf, HEAPBUF_TMP);
    InitHeapBufMan(&pHeapMan->TxPri[0], HEAPBUF_TXPRI0);
    InitHeapBufMan(&pHeapMan->TxPri[1], HEAPBUF_TXPRI1);
    InitHeapBufMan(&pHeapMan->TxPri[2], HEAPBUF_TXPRI2);
    InitHeapBufMan(&pHeapMan->MaDataCfm, HEAPBUF_MADATACFM);
    InitHeapBufMan(&pHeapMan->RxData, HEAPBUF_RXDATA);
    InitHeapBufMan(&pHeapMan->RxBeacon, HEAPBUF_RXBEACON);
    InitHeapBufMan(&pHeapMan->RxManCtrl, HEAPBUF_RXMANCTRL);
    InitHeapBufMan(&pHeapMan->Defrag, HEAPBUF_DEFRAG);
    InitHeapBufMan(&pHeapMan->ToWM, HEAPBUF_TOWM);
    InitHeapBufMan(&pHeapMan->RequestCmd, HEAPBUF_REQUEST_CMD);

    pWork->GameInfoAdrs = (LPHEAPBUF_HEADER)((u32)AllocateHeapBuf(&pHeapMan->TmpBuf, MAX_GAMEINFO_LENGTH + 1) + WL_RSV * 2);
    pWork->GameInfoLength = 0;
    pWork->bUpdateGameInfo = FALSE;
}

void ReleaseAllWlHeapBuf(void)
{
    LPHEAP_MAN pHeapMan = &wlMan->HeapMan;

    ReleaseAllHeapBuf(&pHeapMan->TxPri[0]);
    ReleaseAllHeapBuf(&pHeapMan->TxPri[1]);
    ReleaseAllHeapBuf(&pHeapMan->TxPri[2]);
    ReleaseAllHeapBuf(&pHeapMan->MaDataCfm);
    ReleaseAllHeapBuf(&pHeapMan->RxData);
    ReleaseAllHeapBuf(&pHeapMan->RxBeacon);
    ReleaseAllHeapBuf(&pHeapMan->RxManCtrl);
    ReleaseAllHeapBuf(&pHeapMan->Defrag);
}

void ReleaseAllHeapBuf(LPHEAPBUF_MAN pHeapBufMan)
{
    u32 *pNext;
    u32 *pBuf = (u32 *)GetHeapBufHeadAdrs(pHeapBufMan);

    if (pHeapBufMan->Count != 0) {
        while ((u32)pBuf != HEAPBUF_HEAD_NONE) {
            pNext = (u32 *)GetHeapBufNextAdrs(pBuf);
            ReleaseHeapBuf(pHeapBufMan, pBuf);
            pBuf = pNext;
        }
    }
}

void Wl_DispHeapBufLinkList(HEAPBUF_MAN *pBufMan)
{
    LPHEAPBUF_HEADER pBuf;

    if (pBufMan->Head == HEAD_NONE) {
        Puts("Not exist Buffer link\r");
        return;
    }

    Puts(" Address :   Prev   :   Next   : Flag : SubFlag\r");
    for (pBuf = pBufMan->Head;;) {
        DbgPrint("%08X : %08X : %08X : %4X\r\n",
            (u32)pBuf,
            pBuf->Prev,
            pBuf->Next,
            pBuf->Flag);

        if (pBuf->Next == NEXT_NONE) {
            break;
        }
    }
}

void Wl_DispHeapBufManOne(char *pStr, HEAPBUF_MAN *pBufMan)
{
}
