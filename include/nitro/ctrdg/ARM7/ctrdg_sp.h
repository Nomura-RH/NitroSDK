#ifndef NITRO_CTRDG_ARM7_CTRDG_SP_H_
#define NITRO_CTRDG_ARM7_CTRDG_SP_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define PULSE_NUM_MAX 6

#define VIB_ADDRESS 0x08001000

typedef void (*CartridgePulloutCallback) (void);

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

void CTRDG_VibPulseEdgeUpdate(void *);
void CTRDG_CheckPullOut_Polling(void);

#ifdef  __cplusplus
}
#endif

#endif
