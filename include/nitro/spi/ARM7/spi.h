#ifndef NITRO_SPI_SPI_H_
#define NITRO_SPI_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nitro/hw/ARM7/ioreg_SPI.h>
#include <nitro/ioreg.h>
#include <nitro/misc.h>

typedef enum {
    SPI_BAUDRATE_4MHZ = 0,
    SPI_BAUDRATE_2MHZ = 1,
    SPI_BAUDRATE_1MHZ = 2,
    SPI_BAUDRATE_512KHZ = 3
} SPIBaudRate;

typedef enum {
    SPI_TRANSMODE_1BYTE = 0,
    SPI_TRANSMODE_CONTINUOUS = 1
} SPITransMode;

#ifdef SDK_TS
typedef enum {
    SPI_CLKMODE_8CLK = 0,
    SPI_CLKMODE_16CLK = 1
} SPIClockMode;
#endif

#ifdef SDK_TS
typedef enum {
    SPI_COMMPARTNER_PMIC = 0,
    SPI_COMMPARTNER_EEPROM = 1,
    SPI_COMMPARTNER_TP = 2
} SPICommPartner;
#else
typedef enum {
    SPI_COMMPARTNER_PMIC = 0,
    SPI_COMMPARTNER_EEPROM = 1
} SPICommPartner;

#define SPI_COMMPARTNER_TP (SPI_COMMPARTNER_PMIC)
#endif

#define SPI_COMMPARTNER_ASSERT(x) SDK_ASSERT((u32)x <= SPI_COMMPARTNER_EEPROM)
#define SPI_TRANSMODE_ASSERT(x)   SDK_ASSERT((u32)x <= SPI_TRANSMODE_CONTINUOUS)
#define SPI_BAUDRATE_ASSERT(x)    SDK_ASSERT((u32)x <= SPI_BAUDRATE_512KHZ)

#define SPI_THREAD_STACK_SIZE    512
#define SPI_DEVICE_COMMAND_DUMMY 0
#define SPI_MESSAGE_ARRAY_MAX    16
#define SPI_ENTRY_ARGS_MAX       4

typedef struct SPIEntry {
    SPIDeviceType type;
    u32 process;
    u32 arg[SPI_ENTRY_ARGS_MAX];
} SPIEntry;

typedef struct SPIWork {
    BOOL exception;
    SPIDeviceType type;
    OSThread thread;
    u64 stack[SPI_THREAD_STACK_SIZE / sizeof(u64)];
    OSMessageQueue message;
    OSMessage msg_buf[SPI_MESSAGE_ARRAY_MAX];
    SPIEntry entry[SPI_MESSAGE_ARRAY_MAX];
    u32 entryIndex;
    OSThreadQueue lock;
#ifndef SDK_THREAD_INFINITY
#if (OS_THREAD_MAX_NUM <= 16)
    u8 reserved[2];
#endif
#endif
    u32 lockId;
} SPIWork;

static inline BOOL SPI_Enable(void)
{
    BOOL pre = (reg_SPI_SPICNT & ~REG_SPI_SPICNT_E_MASK) >> REG_SPI_SPICNT_E_SHIFT;
    reg_SPI_SPICNT = (u16)((reg_SPI_SPICNT & ~REG_SPI_SPICNT_E_MASK) | (1UL << REG_SPI_SPICNT_E_SHIFT));
    return pre;
}

static inline BOOL SPI_Disable(void)
{
    BOOL pre = (reg_SPI_SPICNT & ~REG_SPI_SPICNT_E_MASK) >> REG_SPI_SPICNT_E_SHIFT;
    reg_SPI_SPICNT = (u16)(reg_SPI_SPICNT & ~REG_SPI_SPICNT_E_MASK);
    return pre;
}

static inline BOOL SPI_Restore(BOOL flag)
{
    BOOL pre = (reg_SPI_SPICNT & ~REG_SPI_SPICNT_E_MASK) >> REG_SPI_SPICNT_E_SHIFT;
    reg_SPI_SPICNT = (u16)((reg_SPI_SPICNT & ~REG_SPI_SPICNT_E_MASK) | (flag ? 1UL << REG_SPI_SPICNT_E_SHIFT : 0));
    return pre;
}

static inline void SPI_SetInterrupt(BOOL e)
{
    reg_SPI_SPICNT = (u16)((reg_SPI_SPICNT & REG_SPI_SPICNT_I_MASK) | (e ? 1UL << REG_SPI_SPICNT_I_SHIFT : 0));
}

static inline BOOL SPI_GetInterrupt(void)
{
    return (BOOL)((reg_SPI_SPICNT & REG_SPI_SPICNT_I_MASK) >> REG_SPI_SPICNT_I_SHIFT);
}

static inline void SPI_SetCommPartner(SPICommPartner p)
{
    SPI_COMMPARTNER_ASSERT(p);
    reg_SPI_SPICNT = (u16)((reg_SPI_SPICNT & REG_SPI_SPICNT_SEL_MASK) | (p << REG_SPI_SPICNT_SEL_SHIFT));
}

static inline SPICommPartner SPI_GetCommPartner(void)
{
    return (SPICommPartner)((reg_SPI_SPICNT & REG_SPI_SPICNT_SEL_MASK) >> REG_SPI_SPICNT_SEL_SHIFT);
}

static inline void SPI_SetTransMode(SPITransMode mode)
{
    SPI_TRANSMODE_ASSERT(mode);
    reg_SPI_SPICNT = (u16)((reg_SPI_SPICNT & REG_SPI_SPICNT_MODE_MASK) | (mode << REG_SPI_SPICNT_MODE_SHIFT));
}

static inline SPITransMode SPI_GetTransMode(void)
{
    return (SPITransMode)((reg_SPI_SPICNT & REG_SPI_SPICNT_MODE_MASK) >> REG_SPI_SPICNT_MODE_SHIFT);
}

static inline BOOL SPI_IsBusy(void)
{
    return (BOOL)((reg_SPI_SPICNT & REG_SPI_SPICNT_BUSY_MASK) >> REG_SPI_SPICNT_BUSY_SHIFT);
}

static inline void SPI_SetBaudRate(SPIBaudRate baud)
{
    SPI_BAUDRATE_ASSERT(baud);
    reg_SPI_SPICNT = (u16)((reg_SPI_SPICNT & REG_SPI_SPICNT_BAUDRATE_MASK) | (baud << REG_SPI_SPICNT_BAUDRATE_SHIFT));
}

static inline SPIBaudRate SPI_GetBaudRate(void)
{
    return (SPIBaudRate)((reg_SPI_SPICNT & REG_SPI_SPICNT_BAUDRATE_MASK) >> REG_SPI_SPICNT_BAUDRATE_SHIFT);
}

static inline void SPI_SendData(u8 data)
{
    reg_SPI_SPID = (u16)data;
}

static inline u8 SPI_GetData(void)
{
    return reg_SPI_SPICNT & 0xFF;
}

static inline void SPI_Wait(void)
{
    while (reg_SPI_SPICNT & REG_SPI_SPICNT_BUSY_MASK) {
    }
}

static inline void SPI_Dummy(void)
{
    reg_SPI_SPID = SPI_DEVICE_COMMAND_DUMMY;
}

static inline void SPI_Send(u16 data)
{
    reg_SPI_SPID = data & 0xFF;
}

static inline u16 SPI_Receive(void)
{
    return reg_SPI_SPID & 0xFF;
}

static inline void SPI_DummyWait(void)
{
    SPI_Dummy();
    SPI_Wait();
}

static inline void SPI_SendWait(u16 data)
{
    SPI_Send(data);
    SPI_Wait();
}

static inline u16 SPI_DummyWaitReceive(void)
{
    SPI_Dummy();
    SPI_Wait();
    return SPI_Receive();
}

static inline u16 SPI_SendWaitReceive(u16 data)
{
    SPI_Send(data);
    SPI_Wait();
    return SPI_Receive();
}

void SPI_Init(u32 prio);
void SPI_Lock(u32 id);
void SPI_Unlock(u32 id);
void SPIi_ReturnResult(u16 command, u16 result);
BOOL SPIi_CheckException(SPIDeviceType type);
void SPIi_GetException(SPIDeviceType type);
void SPIi_ReleaseException(SPIDeviceType type);
BOOL SPIi_SetEntry(SPIDeviceType type, u32 process, u16 args, ...);
BOOL SPIi_CheckEntry(void);

#ifdef __cplusplus
}
#endif

#endif
