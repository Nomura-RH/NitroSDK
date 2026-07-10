#define __DIAG_C_
#define __INSYSROM__

#include "WlSys.h"
#include "WlLib.h"

#include "MAC.h"
#include "Diag.h"
#include "CfgDevs.h"
#include "WlCmdLabel.h"
#include "Compati.h"

static const u16 test_pattern[] = { 0xFFFF, 0x5A5A, 0xA5A5 };
static const TEST_REGS test_reg[] = {
#ifndef SDK_SIMPLE_DIAG_WL
    { 0x0006, 0x003F },
#endif
    { 0x0018, 0xFFFF },
    { 0x001A, 0xFFFF },
    { 0x001C, 0xFFFF },
#ifndef SDK_SIMPLE_DIAG_WL
    { 0x0020, 0xFFFF },
    { 0x0022, 0xFFFF },
    { 0x0024, 0xFFFF },
    { 0x002A, 0x07FF },
    { 0x0050, 0xFFFF },
    { 0x0052, 0xFFFF },
    { 0x0056, 0x0FFE },
    { 0x0058, 0x1FFE },
    { 0x005A, 0x0FFE },
    { 0x005C, 0x0FFF }, //
    { 0x0062, 0x1FFE },
    { 0x0064, 0x0FFF },
    { 0x0068, 0x1FFE },
    { 0x006C, 0x0FFF }, //
    { 0x0074, 0x1FFE },
#endif
    { 0x0122, 0xFFFF },
    { 0x0124, 0xFFFF },
#ifndef SDK_SIMPLE_DIAG_WL
    { 0x0128, 0xFFFF },
    { 0x0130, 0x0FFF },
    { 0x0132, 0x8FFF },
    { 0x0134, 0xFFFF },
    { 0x0140, 0xFFFF },
    { 0x0142, 0xFFFF },
#endif
};
void DiagMacRegister(void)
{
    vu16 *pReg;
    u16 wd, rd;
    u32 j, err;
#ifndef SDK_SIMPLE_DIAG_WL
    u32 i;
#endif

#ifdef SDK_SIMPLE_DIAG_WL
    DbgPutchar('S');
#endif
    DbgPrint("Diag MAC Register...");
    err = 0;

#ifndef SDK_SIMPLE_DIAG_WL
    for (i = 0; i < sizeof(test_pattern) / 2; i++) {
        for (j = 0; j < sizeof(test_reg) / sizeof(TEST_REGS); j++) {
            pReg = (vu16 *)(MAC_REG_BASE + (u32)test_reg[j].adrs);

            wd = test_pattern[i] & test_reg[j].mask;
            *pReg = wd;

            rd = *pReg;

            if (rd != (test_pattern[i] & test_reg[j].mask)) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T1:Adrs %03x = %04x[%04x]\r\n", test_reg[j].adrs, rd, wd);
                if (err++ > MAX_DIAG_ERR) {
                    goto mac_reg_diag_end;
                }
            }
        }
    }
#endif

    for (j = 0, wd = 0x1234; j < sizeof(test_reg) / sizeof(TEST_REGS); j++) {
        pReg = (vu16 *)(MAC_REG_BASE + (u32)test_reg[j].adrs);

        *pReg = wd & test_reg[j].mask;

        wd += 0x1234;
    }
    for (j = 0, wd = 0x1234; j < sizeof(test_reg) / sizeof(TEST_REGS); j++) {
        pReg = (vu16 *)(MAC_REG_BASE + (u32)test_reg[j].adrs);

        rd = *pReg;

        if (rd != (wd & test_reg[j].mask)) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T2:Adrs %03x = %04x[%04x]\r\n", test_reg[j].adrs, rd, wd & test_reg[j].mask);
            if (err++ > MAX_DIAG_ERR) {
                goto mac_reg_diag_end;
            }
        }

        wd += 0x1234;
    }

    for (j = 0; j < sizeof(test_reg) / sizeof(TEST_REGS); j++) {
        pReg = (vu16 *)(MAC_REG_BASE + (u32)test_reg[j].adrs);

        *pReg = 0;

        rd = *pReg;

        if (rd != 0) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T3:Adrs %03x = %04x[0000]\r\n", test_reg[j].adrs, rd);
            if (err++ > MAX_DIAG_ERR) {
                goto mac_reg_diag_end;
            }
        }
    }

mac_reg_diag_end:
    if (err != 0) {
        wlMan->Config.DiagResult |= WL_DIAG_ERR_MAC_REG;
    }
}

void DiagMacMemory(void)
{
    u32 i, err;
    u16 *p16;
    u16 w16, r16;

#ifdef SDK_SIMPLE_DIAG_WL
    DbgPutchar('S');
#endif
    DbgPrint("Diag MAC Memory...");
    err = 0;

    p16 = (u16 *)MAC_MEM_BASE;
    w16 = 0xFFFF;
    for (i = 0; i < MAC_MEM_SIZE; i += 2) {
        *p16++ = w16--;
    }

    p16 = (u16 *)MAC_MEM_BASE;
    w16 = 0xFFFF;
    for (i = 0; i < MAC_MEM_SIZE; i += 2, p16++, w16--) {
        r16 = *p16;
        if (r16 != w16) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T1:Adrs %08x = %04x[%04x]\r\n", (u32)p16, r16, w16);
            if (err++ > MAX_DIAG_ERR) {
                goto mac_diag_end;
            }
        }
    }

#ifndef SDK_SIMPLE_DIAG_WL
    p16 = (u16 *)MAC_MEM_BASE;
    w16 = 0x5A5A;
    for (i = 0; i < MAC_MEM_SIZE; i += 2, w16 = ~w16) {
        *p16++ = w16;
    }

    p16 = (u16 *)MAC_MEM_BASE;
    w16 = 0x5A5A;
    for (i = 0; i < MAC_MEM_SIZE; i += 2, p16++, w16 = ~w16) {
        r16 = *p16;
        if (r16 != w16) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T2:Adrs %08x = %04x[%04x]\r\n", (u32)p16, r16, w16);
            if (err++ > MAX_DIAG_ERR) {
                goto mac_diag_end;
            }
        }
    }
#endif

#ifndef SDK_SIMPLE_DIAG_WL
    p16 = (u16 *)MAC_MEM_BASE;
    w16 = 0xA5A5;
    for (i = 0; i < MAC_MEM_SIZE; i += 2, w16 = ~w16) {
        *p16++ = w16;
    }

    p16 = (u16 *)MAC_MEM_BASE;
    w16 = 0xA5A5;
    for (i = 0; i < MAC_MEM_SIZE; i += 2, p16++, w16 = ~w16) {
        r16 = *p16;
        if (r16 != w16) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T3:Adrs %08x = %04x[%04x]\r\n", (u32)p16, r16, w16);
            if (err++ > MAX_DIAG_ERR) {
                goto mac_diag_end;
            }
        }
    }
#endif

mac_diag_end:
    if (err > MAX_DIAG_ERR) {
        DbgPuts("Too many errors\r");
    }
    if (err != 0) {
        wlMan->Config.DiagResult |= WL_DIAG_ERR_MAC_RAM;
    } else {
        DbgPuts("OK\r");
    }
}

#ifndef SDK_CPU_COPY_WL
void DiagMacDma(void)
{
#ifndef SDK_SIMPLE_DIAG_WL
    static const u16 testLen[] = { 0x106, 0x60C, 0x1000, 0x2000 };
    u32 i, j;
#endif
    u16 *pBuf;

#ifndef SDK_SIMPLE_DIAG_WL
    for (j = sizeof(testLen) / sizeof(u16); j > 0; j--) {
        pBuf = (u16 *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, testLen[j - 1]);
        if (pBuf != HEAPBUF_NOT_ENOUGH_MEMORY) {
            break;
        }
    }
    if (j == 0) {
        DbgPrint("Can't execute MAC DMA Diag\r\n");
        return;
    }
#else
    {
        pBuf = (u16 *)AllocateHeapBuf(&wlMan->HeapMan.TmpBuf, 0x106);
        if (pBuf == HEAPBUF_NOT_ENOUGH_MEMORY) {
            DbgPrint("Can't execute MAC DMA Diag\r\n");
            return;
        }
    }
#endif

#ifdef SDK_SIMPLE_DIAG_WL
    DbgPutchar('S');
#endif
    DbgPrint("Diag MAC dma...");

#ifndef SDK_SIMPLE_DIAG_WL
    for (i = 0; i < j; i++) {
        if (DiagMacDmaMain((u32)pBuf + 0x10, (u32)testLen[i] - 0x10) != 0) {
            DbgPrint("[Test-%u]-err\r\n", i + 1);
            ReleaseHeapBuf(&wlMan->HeapMan.TmpBuf, pBuf);
            return;
        }
    }
#else
    {
        if (DiagMacDmaMain((u32)pBuf + 0x10, (u32)0x106 - 0x10) != 0) {
            DbgPrint("[Test-0]-err\r\n");
            ReleaseHeapBuf(&wlMan->HeapMan.TmpBuf, pBuf);
            return;
        }
    }
#endif

    DbgPuts("OK\r");

    ReleaseHeapBuf(&wlMan->HeapMan.TmpBuf, pBuf);
}
#endif

#ifndef SDK_CPU_COPY_WL
u32 DiagMacDmaMain(u32 BufAdrs, u32 BufLength)
{
    u32 srcAdrs, destAdrs, length, dmaLength;
    u32 i, err;
    u16 *pDest;
    u16 *pSrc;
    u16 w16, r16;

    err = 0;

    if (BufLength < MAC_MEM_SIZE) {
        length = BufLength;
    } else {
        length = MAC_MEM_SIZE;
    }

    *(u16 *)MREG_RXBUF_STR = 0x4000;
    *(u16 *)MREG_RXBUF_END = 0x6000;
    *(u16 *)MREG_RDMA_JUMP = 0x5000;

#ifndef SDK_SIMPLE_DIAG_WL
    srcAdrs = MAC_MEM_BASE;
    destAdrs = BufAdrs;
    pSrc = (u16 *)srcAdrs;
    w16 = 0x55AA;
    for (i = 0; i < MAC_MEM_SIZE; i += 2, w16 = ~w16) {
        *pSrc++ = w16;
    }

    dmaLength = length;
    w16 = 0x55AA;
    for (srcAdrs = MAC_MEM_BASE; srcAdrs < MAC_MEM_BASE + MAC_MEM_SIZE;) {
        if (srcAdrs + length > MAC_MEM_BASE + MAC_MEM_SIZE) {
            dmaLength = MAC_MEM_BASE + MAC_MEM_SIZE - srcAdrs;
        }

        DMA_Read((void *)destAdrs, (void *)srcAdrs, dmaLength);

        pDest = (u16 *)destAdrs;
        for (i = 0; i < dmaLength; i += 2, pDest++, w16 = ~w16) {
            r16 = *pDest;
            if (r16 != w16) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T7:Adrs %08x:%08x [w/r]=[%04x:%04x]\r\n", (u32)srcAdrs + i, (u32)pDest, w16, r16);
                if (err++ > MAX_DIAG_ERR) {
                    goto dma_diag_end;
                }
            }
        }
        srcAdrs += dmaLength;
    }
#endif

    srcAdrs = BufAdrs;
    destAdrs = MAC_MEM_BASE;

#ifndef SDK_SIMPLE_DIAG_WL
    // Write DMA
    *(vu16 *)MREG_WDMA_STR = 0x4000;
    Diag_DmaClear(0x0000);

    // Check
    pDest = (u16 *)MAC_MEM_BASE;
    w16 = 0;
    for (i = 0; i < MAC_MEM_SIZE / 2; i++, pDest++) {
        r16 = *pDest;
        if (r16 != w16) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T1:Adrs %08x [w/r]=[%04x:%04x]\r\n", (u32)pDest, w16, r16);
            if (err++ > MAX_DIAG_ERR) {
                goto dma_diag_end;
            }
        }
    }
#endif // 	#ifndef	SDK_SIMPLE_DIAG_WL

#ifndef SDK_SIMPLE_DIAG_WL
    *(vu16 *)MREG_WDMA_STR = 0x4000;
    Diag_DmaClear(0xFFFF);

    pDest = (u16 *)MAC_MEM_BASE;
    w16 = 0xFFFF;
    for (i = 0; i < MAC_MEM_SIZE / 2; i++, pDest++) {
        r16 = *pDest;
        if (r16 != w16) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T2:Adrs %08x [w/r]=[%04x:%04x]\r\n", (u32)pDest, w16, r16);
            if (err++ > MAX_DIAG_ERR) {
                goto dma_diag_end;
            }
        }
    }
#endif //	#ifndef	SDK_SIMPLE_DIAG_WL

#ifndef SDK_SIMPLE_DIAG_WL
    pSrc = (u16 *)srcAdrs;
    w16 = 0x55AA;
    for (i = 0; i < length; i += 2, w16 = ~w16) {
        *pSrc++ = w16;
    }

    dmaLength = length;
    for (destAdrs = MAC_MEM_BASE; destAdrs < MAC_MEM_BASE + MAC_MEM_SIZE;) {
        if (destAdrs + length > MAC_MEM_BASE + MAC_MEM_SIZE) {
            dmaLength = MAC_MEM_BASE + MAC_MEM_SIZE - destAdrs;
        }

        DMA_Write((void *)destAdrs, (void *)srcAdrs, dmaLength);

        pDest = (u16 *)destAdrs;
        w16 = 0x55AA;
        for (i = 0; i < dmaLength; i += 2, pDest++, w16 = ~w16) {
            r16 = *pDest;

            if (w16 != r16) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T3:Adrs %08x [w/r]=[%04x:%04x]\r\n", (u32)pDest, w16, r16);
                if (err++ > MAX_DIAG_ERR) {
                    goto dma_diag_end;
                }
            }
        }

        destAdrs += dmaLength;
    }
#endif // 	#ifndef	SDK_SIMPLE_DIAG_WL

    pSrc = (u16 *)srcAdrs;
    for (i = w16 = 0; i < length; i += 2) {
        *pSrc++ = w16--;
    }

    dmaLength = length;
    for (destAdrs = MAC_MEM_BASE; destAdrs < MAC_MEM_BASE + MAC_MEM_SIZE;) {
        if (destAdrs + length > MAC_MEM_BASE + MAC_MEM_SIZE) {
            dmaLength = MAC_MEM_BASE + MAC_MEM_SIZE - destAdrs;
        }

        DMA_Write((void *)destAdrs, (void *)srcAdrs, dmaLength);

        pDest = (u16 *)destAdrs;
        w16 = 0;
        for (i = 0; i < dmaLength; i += 2, pDest++, w16--) {
            r16 = *pDest;

            if (w16 != r16) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T4:Adrs %08x [w:r]=[%04x:%04x]\r\n", (u32)pDest, w16, r16);
                if (err++ > MAX_DIAG_ERR) {
                    goto dma_diag_end;
                }
            }
        }

        destAdrs += dmaLength;
    }

#ifndef SDK_SIMPLE_DIAG_WL
    srcAdrs = MAC_MEM_BASE;
    destAdrs = BufAdrs;
    pSrc = (u16 *)srcAdrs;
    for (i = 0; i < MAC_MEM_SIZE; i += 2) {
        *pSrc++ = 0;
    }

    dmaLength = length;
    w16 = 0;
    for (srcAdrs = MAC_MEM_BASE; srcAdrs < MAC_MEM_BASE + MAC_MEM_SIZE;) {
        if (srcAdrs + length > MAC_MEM_BASE + MAC_MEM_SIZE) {
            dmaLength = MAC_MEM_BASE + MAC_MEM_SIZE - srcAdrs;
        }

        DMA_Read((void *)destAdrs, (void *)srcAdrs, dmaLength);

        pDest = (u16 *)destAdrs;
        for (i = 0; i < dmaLength; i += 2, pDest++) {
            r16 = *pDest;
            if (r16 != w16) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T5:Adrs %08x:%08x [w/r]=[%04x:%04x]\r\n", (u32)srcAdrs + i, (u32)pDest, w16, r16);
                if (err++ > MAX_DIAG_ERR) {
                    goto dma_diag_end;
                }
            }
        }
        srcAdrs += dmaLength;
    }
#endif

#ifndef SDK_SIMPLE_DIAG_WL
    srcAdrs = MAC_MEM_BASE;
    destAdrs = BufAdrs;
    pSrc = (u16 *)srcAdrs;
    for (i = 0; i < MAC_MEM_SIZE; i += 2) {
        *pSrc++ = 0xFFFF;
    }

    dmaLength = length;
    w16 = 0xFFFF;
    for (srcAdrs = MAC_MEM_BASE; srcAdrs < MAC_MEM_BASE + MAC_MEM_SIZE;) {
        if (srcAdrs + length > MAC_MEM_BASE + MAC_MEM_SIZE) {
            dmaLength = MAC_MEM_BASE + MAC_MEM_SIZE - srcAdrs;
        }

        DMA_Read((void *)destAdrs, (void *)srcAdrs, dmaLength);

        pDest = (u16 *)destAdrs;
        for (i = 0; i < dmaLength; i += 2, pDest++) {
            r16 = *pDest;
            if (r16 != w16) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T6:Adrs %08x:%08x [w/r]=[%04x:%04x]\r\n", (u32)srcAdrs + i, (u32)pDest, w16, r16);
                if (err++ > MAX_DIAG_ERR) {
                    goto dma_diag_end;
                }
            }
        }
        srcAdrs += dmaLength;
    }
#endif // 	#ifndef	SDK_SIMPLE_DIAG_WL

    srcAdrs = MAC_MEM_BASE;
    destAdrs = BufAdrs;

    pSrc = (u16 *)srcAdrs;
    for (i = w16 = 0; i < MAC_MEM_SIZE; i += 2) {
        *pSrc++ = w16--;
    }

    dmaLength = length;
    w16 = 0;
    for (srcAdrs = MAC_MEM_BASE; srcAdrs < MAC_MEM_BASE + MAC_MEM_SIZE;) {
        if (srcAdrs + length > MAC_MEM_BASE + MAC_MEM_SIZE) {
            dmaLength = MAC_MEM_BASE + MAC_MEM_SIZE - srcAdrs;
        }

        DMA_Read((void *)destAdrs, (void *)srcAdrs, dmaLength);

        pDest = (u16 *)destAdrs;
        for (i = 0; i < dmaLength; i += 2, w16--, pDest++) {
            r16 = *pDest;
            if (r16 != w16) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T8:Adrs %08x:%08x [w:r]=[%04x:%04x]\r\n", (u32)srcAdrs + i, (u32)pDest, w16, r16);
                if (err++ > MAX_DIAG_ERR) {
                    goto dma_diag_end;
                }
            }
        }
        srcAdrs += dmaLength;
    }

dma_diag_end:
    if (err > MAX_DIAG_ERR) {
        DbgPuts("Too many errors\r");
    }
    if (err != 0) {
        wlMan->Config.DiagResult |= WL_DIAG_ERR_MAC_DMA;
    }

    return err;
}
#endif

#if (INCLUDE_BBP_ES1)
static const u16 BBPDiagSkipAdrsES1[] = {
    0x02, 0x04, 0x05, 0x06, 0x07, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x26, 0x29, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x33, 0x34, 0x35, 0x36, 0x37, 0x65, 0x00
};
#endif
static const u16 BBPDiagSkipAdrsRelease[] = {
    0,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    39,
    77,
    93,
    94,
    95,
    96,
    97,
    100,
    102,
    0
};

void DiagBaseBand(void)
{
    u16 *BBPDiagSkipAdrs;
    u32 adrs, i, err;
    u16 w8, r8;
#ifndef SDK_SIMPLE_DIAG_WL
    u32 j;
    u16 bit;
#endif

#ifdef SDK_SIMPLE_DIAG_WL
    DbgPutchar('S');
#endif
    DbgPrint("Diag BBP Regs...");
    err = 0;

#if (INCLUDE_BBP_ES1)
    if (wlMan->Rf.Id == MTMBBP_ES1) {
        BBPDiagSkipAdrs = (u16 *)BBPDiagSkipAdrsES1;
    } else
#endif
        BBPDiagSkipAdrs = (u16 *)BBPDiagSkipAdrsRelease;

#ifndef SDK_SIMPLE_DIAG_WL
    for (adrs = 0; adrs < BBP_REG_SIZE; adrs++) {
        if (BBP_Write(adrs, 0x0FF) == -1) {
            DbgPuts("Access Error\r");
            err = 1;
            goto bbp_diag_end;
        }
    }

    w8 = 0x00FF;
    for (i = adrs = 0; adrs < BBP_REG_SIZE; adrs++) {
        if (adrs == BBPDiagSkipAdrs[i]) {
            i++;
            continue;
        }

        r8 = BBP_Read(adrs);
        if (r8 != w8) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T1:Adrs %02x [w:r] = [%02x:%02x]\r\n", adrs, w8, r8);
            if (err++ > MAX_DIAG_ERR) {
                goto bbp_diag_end;
            }
        }
    }
#endif // 	#ifndef	SDK_SIMPLE_DIAG_WL

#ifndef SDK_SIMPLE_DIAG_WL
    for (adrs = 0; adrs < BBP_REG_SIZE; adrs++) {
        BBP_Write(adrs, 0x00);
    }

    w8 = 0x00;
    for (i = adrs = 0; adrs < BBP_REG_SIZE; adrs++) {
        if (adrs == BBPDiagSkipAdrs[i]) {
            i++;
            continue;
        }

        r8 = BBP_Read(adrs);
        if (r8 != w8) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T2:Adrs %02x [w:r] = [%02x:%02x]\r\n", adrs, w8, r8);
            if (err++ > MAX_DIAG_ERR) {
                goto bbp_diag_end;
            }
        }
    }
#endif // 	#ifndef	SDK_SIMPLE_DIAG_WL

#ifndef SDK_SIMPLE_DIAG_WL
    w8 = 0x55;
    for (adrs = 0; adrs < BBP_REG_SIZE; adrs++, w8 = ~w8) {
        BBP_Write(adrs, w8);
    }

    w8 = 0x55;
    for (i = adrs = 0; adrs < BBP_REG_SIZE; adrs++, w8 = (~w8 & 0x00FF)) {
        if (adrs == BBPDiagSkipAdrs[i]) {
            i++;
            continue;
        }

        r8 = BBP_Read(adrs);
        if (r8 != w8) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T3:Adrs %02x [w:r] = [%02x:%02x]\r\n", adrs, w8, r8);
            if (err++ > MAX_DIAG_ERR) {
                goto bbp_diag_end;
            }
        }
    }
#endif // 	#ifndef	SDK_SIMPLE_DIAG_WL

    w8 = 0x0FF;
    for (adrs = 0; adrs < BBP_REG_SIZE; adrs++, w8--) {
        BBP_Write(adrs, w8);
    }

    w8 = 0x0FF;
    for (i = adrs = 0; adrs < BBP_REG_SIZE; adrs++, w8--) {
        if (adrs == BBPDiagSkipAdrs[i]) {
            i++;
            continue;
        }

        r8 = BBP_Read(adrs);
        if (r8 != w8) {
            DbgWlPrint(B_WL_DBG_DIAGMSG, "T4:Adrs %02x [w:r] = [%02x:%02x]\r\n", adrs, w8, r8);
            if (err++ > MAX_DIAG_ERR) {
                goto bbp_diag_end;
            }
        }
    }

#ifndef SDK_SIMPLE_DIAG_WL
    for (i = adrs = 0; adrs < BBP_REG_SIZE; adrs++) {
        if (adrs == BBPDiagSkipAdrs[i]) {
            i++;
            continue;
        }

        bit = 1;
        for (j = 0; j < 9; bit = (bit << 1) & 0x00FF, j++) {
            BBP_Write(adrs, (u32)bit);
            r8 = BBP_Read(adrs);

            if (r8 != bit) {
                DbgWlPrint(B_WL_DBG_DIAGMSG, "T5:Adrs %02x [w:r] = [%02x:%02x]\r\n", adrs, bit, r8);
                if (err++ > MAX_DIAG_ERR) {
                    goto bbp_diag_end;
                }
            }
        }
    }
#endif // 	#ifndef	SDK_SIMPLE_DIAG_WL

bbp_diag_end:
    if (err > MAX_DIAG_ERR) {
        DbgPuts("Too many errors\r");
    }
    if (err != 0) {
        wlMan->Config.DiagResult |= WL_DIAG_ERR_BBP_REG;
    } else {
        DbgPuts("OK\r");
    }
}

#ifndef SDK_SIMPLE_DIAG_WL
void Diag_DmaClear(u16 data)
{
    vu32 *p = (vu32 *)((u32)REG_DMA0SAD_ADDR + WLLIB_DMA_CHANNEL * 12);

    *p = (vu32)&data;
    *(p + 1) = (vu32)(MREG_WDMA_PORT);
    *(p + 2) = (vu32)(DMA_ENABLE | DMA_TIMMING_IMM | DMA_SRC_FIX | DMA_DEST_FIX | DMA_16BIT_BUS | (MAC_MEM_SIZE / 2));
}
#endif
