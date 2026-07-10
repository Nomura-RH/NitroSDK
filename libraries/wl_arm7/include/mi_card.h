#ifndef NITRO_MI_CARD_H
#define NITRO_MI_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u16 latency1 : 13;
    u16 dataScramble_on : 1;
    u16 scrambleUnit_on : 1;
    u16 initScramblePN : 1;

    u16 latency2 : 6;
    u16 cmdScramble_on : 1;
    u16 dataReady : 1;

    u16 pages : 3;
    u16 clockType : 1;
    u16 clockInLatency : 1;
    u16 reset : 1;
    u16 accessMode : 1;
    u16 start : 1;
} CardCnt;

typedef struct {
    u32 dmaNo;
    u32 cardCnt;
    u32 op[2];
} CardCtrlParam;

typedef void (*CardIntrFuncp)(void);

typedef struct {
    const u8 *romp;
    u8 *ramp;
    s32 restSize;
    CardCtrlParam param;
} CardIntrWork;

u32 MI_ReadCardID(void);
u32 MI_LockReadCardID(u32 lockID);

void MI_ReadCard(u32 dmaNo, const void *romp, void *ramp, u32 size);
void MI_LockReadCard(u32 lockID, u32 dmaNo, const void *romp, void *ramp, u32 size);

void MI_ReadCardAsync(u32 dmaNo, const void *romp, void *ramp, u32 size);
void MI_LockReadCardAsync(u32 lockID, u32 dmaNo, const void *romp, void *ramp, u32 size);

void MI_WaitReadCardAsync(void);
void MI_WaitLockReadCardAsync(u32 lockID);

#define MI_DMA_ENABLE    (1UL << REG_MI_DMA0CNT_E_SHIFT)
#define MI_DMA_IF_ENABLE (1UL << REG_MI_DMA0CNT_I_SHIFT)

#define MI_DMA_TIMING_MASK      (REG_MI_DMA0CNT_TIMING_MASK)
#define MI_DMA_TIMING_SHIFT     (REG_MI_DMA0CNT_TIMING_SHIFT)
#define MI_DMA_TIMING_IMM       (0UL << REG_MI_DMA0CNT_TIMING_SHIFT)
#define MI_DMA_TIMING_V_BLANK   (1UL << REG_MI_DMA0CNT_TIMING_SHIFT)
#define MI_DMA_TIMING_CARD      (2UL << REG_MI_DMA0CNT_TIMING_SHIFT)
#define MI_DMA_TIMING_WIRELESS  (3UL << REG_MI_DMA0CNT_TIMING_SHIFT)
#define MI_DMA_TIMING_CARTRIDGE MI_DMA_TIMING_WIRELESS

#define MI_DMA_16BIT_BUS (0UL << REG_MI_DMA0CNT_SB_SHIFT)
#define MI_DMA_32BIT_BUS (1UL << REG_MI_DMA0CNT_SB_SHIFT)

#define MI_DMA_CONTINUOUS_ON (1UL << REG_MI_DMA0CNT_CM_SHIFT)

#define MI_DMA_SRC_INC (0UL << REG_MI_DMA0CNT_SAR_SHIFT)
#define MI_DMA_SRC_DEC (1UL << REG_MI_DMA0CNT_SAR_SHIFT)
#define MI_DMA_SRC_FIX (2UL << REG_MI_DMA0CNT_SAR_SHIFT)

#define MI_DMA_DEST_INC    (0UL << REG_MI_DMA0CNT_DAR_SHIFT)
#define MI_DMA_DEST_DEC    (1UL << REG_MI_DMA0CNT_DAR_SHIFT)
#define MI_DMA_DEST_FIX    (2UL << REG_MI_DMA0CNT_DAR_SHIFT)
#define MI_DMA_DEST_RELOAD (3UL << REG_MI_DMA0CNT_DAR_SHIFT)

#define MI_DMA_COUNT_MASK  (REG_MI_DMA0CNT_WORDCNT_MASK)
#define MI_DMA_COUNT_SHIFT (REG_MI_DMA0CNT_WORDCNT_SHIFT)

#define MI_DMA_NONE_NO ((u32) - 1)

static inline void MIi_DmaSetParams(u32 dmaNo, u32 src, u32 dest, u32 ctrl)
{
    vu32 *p = (vu32 *)((u32)REG_DMA0SAD_ADDR + dmaNo * 12);
    *p = (vu32)src;
    *(p + 1) = (vu32)dest;
    *(p + 2) = (vu32)ctrl;
}

#define DmaReadCard(dmaNo, destp) \
                                  \
    MIi_DmaSetParams(dmaNo, REG_CARD_DATA, (u32)destp, (MI_DMA_ENABLE | MI_DMA_TIMING_CARD | MI_DMA_SRC_FIX | MI_DMA_DEST_INC | MI_DMA_CONTINUOUS_ON | MI_DMA_32BIT_BUS | (1)))

#define DmaWriteCard(dmaNo, srcp) \
                                  \
    MIi_DmaSetParams(dmaNo, (u32)srcp, REG_CARD_DATA, (MI_DMA_ENABLE | MI_DMA_TIMING_CARD | MI_DMA_SRC_INC | MI_DMA_DEST_FIX | MI_DMA_CONTINUOUS_ON | MI_DMA_32BIT_BUS | (1)))

#define MIi_IsCardDataReady() (*(vu32 *)REG_CARDCNT & CARD_DATA_READY)

#define MIi_WaitCardData()             \
    {                                  \
        while (!MIi_IsCardDataReady()) \
            ;                          \
    }

#define MIi_IsCardBusy() (*(vu32 *)REG_CARDCNT & CARD_START)

#define MIi_WaitCard()           \
    {                            \
        while (MIi_IsCardBusy()) \
            ;                    \
    }

#define MIi_GetCardCnt4Game() (*(vu32 *)MROMCNT_GAME_BUF)

#define MROM_SECURE_AREA 0x4000
#define MROM_GAME_AREA   0x8000

#define MROM_SEGMENT_SIZE 0x1000
#define MROM_SECURE_SIZE  0x4000

#ifndef MROM_PAGE_SIZE
#define MROM_PAGE_SIZE 512
#endif

#define MROMCNT_GAME_BUF   (HW_ROM_HEADER_BUF + 0x60)
#define MROMCNT_SECURE_BUF (HW_ROM_HEADER_BUF + 0x64)

#ifndef REG_BASE
#define REG_BASE 0x04000000
#endif
#ifndef REG_IME
#define REG_IME (REG_BASE + 0x208)
#endif

#define REG_CARDMST_SPI_CNT (REG_BASE + 0x1a0)

#define REG_CARD_MASTER_CNT (REG_BASE + 0x1a1)

#define REG_CARD_SPI_CNT  (REG_BASE + 0x1a0)
#define REG_CARD_SPI_DATA (REG_BASE + 0x1a2)

#define REG_CARDCNT   (REG_BASE + 0x1a4)
#define REG_CARD_CMD  (REG_BASE + 0x1a8)
#define REG_CARD_DATA (REG_BASE + 0x100010)

#define REG_CARD_PN_INIT    (REG_BASE + 0x1b0)
#define REG_CARD_PNA_INIT_L (REG_BASE + 0x1b0)
#define REG_CARD_PNB_INIT_L (REG_BASE + 0x1b4)
#define REG_CARD_PNA_INIT_H (REG_BASE + 0x1b8)
#define REG_CARD_PNB_INIT_H (REG_BASE + 0x1ba)

#define CARDMST_SEL_DEVICE 0x20
#define CARDMST_SEL_ROM    0x00
#define CARDMST_SEL_SPI    0x20

#define CARDMST_IF_ENABLE 0x40
#define CARDMST_ENABLE    0x80

#define CARDSPI_SCK_MASK 0x03

#define CARDSPI_SCK_SHIFT 0

#define CARDSPI_SCK_4M   0x00
#define CARDSPI_SCK_2M   0x01
#define CARDSPI_SCK_1M   0x02
#define CARDSPI_SCK_524K 0x03

#define CARDSPI_INV_CS 0x40
#define CARDSPI_BUSY   0x80

#define CARD_LATENCY1_CYCLES_MASK 0x00001fff
#define CARD_LATENCY2_CYCLES_MASK 0x003f0000
#define CARD_LATENCY_MASK         0x003f1fff
#define CARD_PAGE_COUNT_MASK      0x07000000

#define CARD_LATENCY1_CYCLES_SHIFT 0
#define CARD_LATENCY2_CYCLES_SHIFT 16
#define CARD_PAGE_COUNT_SHIFT      24

#define CARD_SCRAMBLE_SET_MASK \
    (CARD_SCRAMBLE_UNIT_ON | CARD_DATA_SCRAMBLE_ON | CARD_CMD_SCRAMBLE_ON)
#define CARD_SCRAMBLE_CLEAR_MASK \
    (~(CARD_INIT_SCRAMBLE_PN | CARD_SCRAMBLE_SET_MASK | CARD_CLOCK_IN_LATENCY | CARD_WRITE_MODE))

#define CARD_DATA_SCRAMBLE_ON 0x00002000
#define CARD_SCRAMBLE_UNIT_ON 0x00004000
#define CARD_INIT_SCRAMBLE_PN 0x00008000
#define CARD_CMD_SCRAMBLE_ON  0x00400000

#define CARD_DATA_READY 0x00800000

#define CARD_0_PAGE   0x00000000
#define CARD_1_PAGE   0x01000000
#define CARD_2_PAGES  0x02000000
#define CARD_4_PAGES  0x03000000
#define CARD_8_PAGES  0x04000000
#define CARD_16_PAGES 0x05000000
#define CARD_32_PAGES 0x06000000
#define CARD_STATUS   0x07000000

#define CARD_CLOCK_TYPE       0x08000000
#define CARD_CLOCK_150NS      0x00000000
#define CARD_CLOCK_240NS      0x08000000
#define CARD_CLOCK_IN_LATENCY 0x10000000
#define CARD_RESET_LO         0x00000000
#define CARD_RESET_HI         0x20000000
#define CARD_ACCESS_MODE      0x40000000
#define CARD_READ_MODE        0x00000000
#define CARD_WRITE_MODE       0x40000000
#define CARD_START            0x80000000

#define ST_CARD_0_PAGE   0
#define ST_CARD_1_PAGE   1
#define ST_CARD_2_PAGES  2
#define ST_CARD_4_PAGES  3
#define ST_CARD_8_PAGES  4
#define ST_CARD_16_PAGES 5
#define ST_CARD_32_PAGES 6
#define ST_CARD_STATUS   7

#define ST_CARD_CLOCK_150NS 0
#define ST_CARD_CLOCK_240NS 1

#define ST_CARD_READ_MODE  0
#define ST_CARD_WRITE_MODE 1

#define MROMOP_G_OP_MASK 0xff000000

#define MROMOP_G_READ_ID   0xb8000000
#define MROMOP_G_READ_PAGE 0xb7000000

#define MROMOP_G_READ_PAGE_PAD_L     0x00ffffff
#define MROMOP_G_READ_PAGE_PAD_H     0x00f00000
#define MROMOP_G_READ_PAGE_MASK_L    0x00ffffff
#define MROMOP_G_READ_PAGE_MASK_H    0xfffffffe
#define MROMOP_G_READ_PAGE_ADDR_MASK 0x000ffffe

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NITRO_MI_CARD_H
