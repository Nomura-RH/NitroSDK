#ifndef NITRO_CTRDG_ARM7_CTRDG_SP_H_
#define NITRO_CTRDG_ARM7_CTRDG_SP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SDK_COMPONENT_VIB

#ifdef SDK_COMPONENT_VIB
#define PULSE_NUM_MAX 6

#define VIB_ADDRESS 0x08001000
#define VIB_BITID   2

typedef enum {
    VIB_STOP = 0,
    VIB_START = 2
} VibSwitch;

typedef void (*CartridgePulloutCallback)(void);

typedef struct {
    u32 current_pos;
    u32 rest_pos;
    u32 rest_tick;
    u32 vib_tick[PULSE_NUM_MAX];
    u32 stop_tick[PULSE_NUM_MAX];
    BOOL is_enable;
    u32 repeat_num;
    u32 current_count;
    CartridgePulloutCallback cartridge_pullout_callback;
    u8 padding[20];
} CTRDGPulseVib;

SDK_COMPILER_ASSERT((sizeof(CTRDGPulseVib) % 32) == 0);

void CTRDG_VibPulseEdgeUpdate(void *ctrdg_ex_data);
#else
#define CTRDG_VibPulseEdgeUpdate(...) (void)0
#endif

void CTRDG_CheckPullOut_Polling(void);

#ifdef __cplusplus
}
#endif

#endif
