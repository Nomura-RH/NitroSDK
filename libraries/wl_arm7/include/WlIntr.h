#ifndef __WLINTR_H_
#define __WLINTR_H_

void InitializeIntr(void);
void ReleaseIntr(void);

void WlIntr(void);

void *AdjustRingPointer(void *p);

#ifdef __WLINTR_C_

static void WlIntrPreTbtt(void) __attribute__((never_inline));
static void WlIntrTbtt(void) __attribute__((never_inline));
static void WlIntrActEnd(void) __attribute__((never_inline));
static void WlIntrRfWakeup(void) __attribute__((never_inline));
static void WlIntrCntOvf(void) __attribute__((never_inline));
static void WlIntrTxErr(void) __attribute__((never_inline));
static void WlIntrRxCntup(void) __attribute__((never_inline));
static void WlIntrTxEnd(void) __attribute__((never_inline));
static void WlIntrRxEnd(void) __attribute__((never_inline));
static void WlIntrMpEnd(u32 bMacBugPatch);
static void WlIntrStartTx(void) __attribute__((never_inline));
static void WlIntrStartRx(void) __attribute__((never_inline));

void UpdateRxBufBnry(u32 bnry, u32 length);
static void SetParentTbttTxq(void);

void MacBugTxMp(void *);

#define WAIT_RX_ADRS2  16 * 4
#define WAIT_RX_BITMAP 28 * 4

static void MultiPollRevicedClearSeq(void);
static u32 CheckKeyTxEnd(void);
static u32 CheckKeyTxEndMain(LPTXQ pTxq);

#endif // __WLINTR_C_
#endif // __WLINTR_H_
