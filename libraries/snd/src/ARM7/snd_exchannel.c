#include "nitro/snd/common/exchannel.h"
#include "nitro/snd/common/util.h"
#include "nitro/snd/common/work.h"
#include "nitro/snd/common/channel.h"

#ifdef SDK_FROM_TOOL
#define HW_CPU_CLOCK_ARM7 33513982
#define HW_CPU_CLOCK_ARM9 67027964

#define SDK_ASSERT(exp)                  ((void)0)
#define SDK_NULL_ASSERT(exp)             ((void)0)
#define SDK_MINMAX_ASSERT(exp, min, max) ((void)0)
#endif

#define SND_EX_CHANNEL_ENVDECAY_INIT (SND_VOLUME_DB_MIN << SND_EX_CHANNEL_ENV_DECAY_SHIFT)

#define SND_EX_CHANNEL_KEY_INIT          60
#define SND_EX_CHANNEL_ORIGINAL_KEY_INIT 60
#define SND_EX_CHANNEL_VELOCITY_INIT     127
#define SND_EX_CHANNEL_PAN_INIT          0

#define SND_EX_CHANNEL_ATTACK_INIT  127
#define SND_EX_CHANNEL_DECAY_INIT   127
#define SND_EX_CHANNEL_SUSTAIN_INIT 127
#define SND_EX_CHANNEL_RELEASE_INIT 127

#define FREQ_C4      261.6255653
#define PSG_TIMER_C4 ((u16)(SND_TIMER_CLOCK / (8 * FREQ_C4)))

static u32 sLockChannel;
static u32 sWeakLockChannel;

static u16 CalcRelease(int release);
static s32 SweepMain(SNDExChannel *ch_p, BOOL doPeriodicProc);
static s32 LfoMain(SNDExChannel *ch_p, BOOL doPeriodicProc);
static void StartExChannel(SNDExChannel *ch_p, s32 length);
static BOOL CompareExChannelVolume(const SNDExChannel *cp1, const SNDExChannel *cp2);
static void InitAllocExChannel(SNDExChannel *ch_p, SNDExChannelCallback callback, void *callbackData, int prio);

void SND_ExChannelInit(void)
{
    for (int ch = 0; ch < SND_CHANNEL_NUM; ch++) {
        SNDExChannel *ch_p = &SNDi_Work.channel[ch];

        ch_p->myNo = ch;
        ch_p->sync_flag = 0;
        ch_p->active_flag = FALSE;
    }

    sLockChannel = 0;
    sWeakLockChannel = 0;
}

void SND_UpdateExChannel(void)
{
    SNDExChannel *ch_p;

    for (int ch = 0; ch < SND_CHANNEL_NUM; ch++) {
        ch_p = &SNDi_Work.channel[ch];

        if (!ch_p->sync_flag) {
            continue;
        }

        if (ch_p->sync_flag & SND_EX_CHANNEL_UPDATE_STOP) {
            SND_StopChannel(ch, FALSE);
        }

        if (ch_p->sync_flag & SND_EX_CHANNEL_UPDATE_START) {
            switch (ch_p->type) {
            case SND_EX_CHANNEL_PCM:
                SND_SetupChannelPcm(
                    ch,
                    ch_p->data,
                    ch_p->wave.format,
                    ch_p->wave.loopflag ? SND_CHANNEL_LOOP_REPEAT : SND_CHANNEL_LOOP_1SHOT,
                    ch_p->wave.loopstart,
                    ch_p->wave.looplen,
                    ch_p->volume & 0xFF,
                    ch_p->volume >> 8,
                    ch_p->timer,
                    ch_p->pan);
                break;

            case SND_EX_CHANNEL_PSG:
                SND_SetupChannelPsg(ch, ch_p->duty, ch_p->volume & 0xFF, ch_p->volume >> 8, ch_p->timer, ch_p->pan);
                break;

            case SND_EX_CHANNEL_NOISE:
                SND_SetupChannelNoise(ch, ch_p->volume & 0xFF, ch_p->volume >> 8, ch_p->timer, ch_p->pan);
                break;
            }
        } else {
            if (ch_p->sync_flag & SND_EX_CHANNEL_UPDATE_TIMER) {
                SND_SetChannelTimer(ch, ch_p->timer);
            }
            if (ch_p->sync_flag & SND_EX_CHANNEL_UPDATE_VOLUME) {
                SND_SetChannelVolume(ch, ch_p->volume & 0xFF, ch_p->volume >> 8);
            }
            if (ch_p->sync_flag & SND_EX_CHANNEL_UPDATE_PAN) {
                SND_SetChannelPan(ch, ch_p->pan);
            }
        }
    }

    for (int ch = 0; ch < SND_CHANNEL_NUM; ch++) {
        ch_p = &SNDi_Work.channel[ch];

        if (!ch_p->sync_flag) {
            continue;
        }

        if (ch_p->sync_flag & SND_EX_CHANNEL_UPDATE_START) {
            SND_StartChannel(ch);
        }

        ch_p->sync_flag = 0;
    }
}

void SND_ExChannelMain(BOOL doPeriodicProc)
{
    SNDExChannel *ch_p;

    for (int ch = 0; ch < SND_CHANNEL_NUM; ch++) {
        s32 decay = 0;
        s32 pitch = 0;
        s32 pan = 0;

        ch_p = &SNDi_Work.channel[ch];

        if (!ch_p->active_flag) {
            continue;
        }

        if (ch_p->start_flag) {
            ch_p->sync_flag |= SND_EX_CHANNEL_UPDATE_START;

            ch_p->start_flag = FALSE;
        } else {
            if (!SND_IsChannelActive(ch)) {
                if (ch_p->callback != NULL) {
                    ch_p->callback(ch_p, SND_EX_CHANNEL_CALLBACK_FINISH, ch_p->callback_data);
                } else {
                    ch_p->prio = SND_EX_CHANNEL_STOP_PRIO;
                }
                ch_p->volume = 0;
                ch_p->active_flag = FALSE;
                continue;
            }
        }

        decay += SND_CalcDecibelSquare(ch_p->velocity);

        pitch += (ch_p->key - ch_p->original_key) << SND_PITCH_DIVISION_BIT;

        decay += SND_UpdateExChannelEnvelope(ch_p, doPeriodicProc);

        pitch += SweepMain(ch_p, doPeriodicProc);

        decay += ch_p->user_decay;
        decay += ch_p->user_decay2;
        pitch += ch_p->user_pitch;

        s32 lfo_value = LfoMain(ch_p, doPeriodicProc);

        switch (ch_p->lfo.param.target) {
        case SND_LFO_VOLUME:
            if (decay > -32768) {
                decay += lfo_value;
            }
            break;
        case SND_LFO_PITCH:
            pitch += lfo_value;
            break;
        case SND_LFO_PAN:
            pan += lfo_value;
            break;
        }

        pan += ch_p->init_pan;
        if (ch_p->pan_range != 127) {
            pan *= ch_p->pan_range;
            pan += 64;
            pan >>= 7;
        }
        pan += ch_p->user_pan;

        if (ch_p->env_status == SND_ENV_RELEASE) {
            if (decay <= SND_VOLUME_DB_MIN) {
                ch_p->sync_flag = SND_EX_CHANNEL_UPDATE_STOP;

                if (ch_p->callback != NULL) {
                    ch_p->callback(ch_p, SND_EX_CHANNEL_CALLBACK_FINISH, ch_p->callback_data);
                } else {
                    ch_p->prio = SND_EX_CHANNEL_STOP_PRIO;
                }
                ch_p->volume = 0;
                ch_p->active_flag = FALSE;
                continue;
            }
        }

        u16 volume = SND_CalcChannelVolume(decay);
        u16 timer = SND_CalcTimer(ch_p->wave.timer, pitch);

        if (ch_p->type == SND_EX_CHANNEL_PSG) {
            timer &= 0xFFFC;
        }

        pan += SND_CHANNEL_PAN_CENTER;
        if (pan < 0) {
            pan = 0;
        } else if (pan > SND_CHANNEL_PAN_MAX) {
            pan = SND_CHANNEL_PAN_MAX;
        }

        if (volume != ch_p->volume) {
            ch_p->volume = volume;
            ch_p->sync_flag |= SND_EX_CHANNEL_UPDATE_VOLUME;
        }
        if (timer != ch_p->timer) {
            ch_p->timer = timer;
            ch_p->sync_flag |= SND_EX_CHANNEL_UPDATE_TIMER;
        }
        if (pan != ch_p->pan) {
            ch_p->pan = pan;
            ch_p->sync_flag |= SND_EX_CHANNEL_UPDATE_PAN;
        }
    }
}

BOOL SND_StartExChannelPcm(SNDExChannel *ch_p, const struct SNDWaveParam *wave, const void *data, s32 length)
{
    ch_p->type = SND_EX_CHANNEL_PCM;
    ch_p->wave = *wave;
    ch_p->data = data;

    StartExChannel(ch_p, length);

    return TRUE;
}

BOOL SND_StartExChannelPsg(SNDExChannel *ch_p, SNDDuty duty, s32 length)
{
    if (ch_p->myNo < SND_PSG_CHANNEL_MIN) {
        return FALSE;
    }
    if (ch_p->myNo > SND_PSG_CHANNEL_MAX) {
        return FALSE;
    }

    ch_p->type = SND_EX_CHANNEL_PSG;
    ch_p->duty = duty;

    ch_p->wave.timer = PSG_TIMER_C4;

    StartExChannel(ch_p, length);

    return TRUE;
}

BOOL SND_StartExChannelNoise(SNDExChannel *ch_p, s32 length)
{
    if (ch_p->myNo < SND_NOISE_CHANNEL_MIN) {
        return FALSE;
    }
    if (ch_p->myNo > SND_NOISE_CHANNEL_MAX) {
        return FALSE;
    }

    ch_p->type = SND_EX_CHANNEL_NOISE;
    ch_p->wave.timer = PSG_TIMER_C4;

    StartExChannel(ch_p, length);

    return TRUE;
}

s32 SND_UpdateExChannelEnvelope(SNDExChannel *ch_p, BOOL doPeriodicProc)
{
    if (doPeriodicProc) {
        switch (ch_p->env_status) {
        case SND_ENV_ATTACK: {
            s32 env_decay_s32 = -ch_p->env_decay;

            env_decay_s32 *= ch_p->attack;
            env_decay_s32 >>= 8;

            ch_p->env_decay = -env_decay_s32;

            if (ch_p->env_decay == 0) {
                ch_p->env_status = SND_ENV_DECAY;
            }
            break;
        }

        case SND_ENV_DECAY: {
            const s32 sustain_decay = SND_CalcDecibelSquare(ch_p->sustain) << SND_EX_CHANNEL_ENV_DECAY_SHIFT;

            ch_p->env_decay -= ch_p->decay;
            if (ch_p->env_decay > sustain_decay) {
                break;
            }
            ch_p->env_decay = sustain_decay;
            ch_p->env_status = SND_ENV_SUSTAIN;
            break;
        }

        case SND_ENV_SUSTAIN:
            break;

        case SND_ENV_RELEASE:
            ch_p->env_decay -= ch_p->release;
            break;
        }
    }

    return ch_p->env_decay >> SND_EX_CHANNEL_ENV_DECAY_SHIFT;
}

void SND_SetExChannelAttack(SNDExChannel *ch_p, int attack)
{
    static const u8 attack_table[127 - 109 + 1] = {
        0, 1, 5, 14, 26, 38, 51, 63, 73, 84, 92, 100, 109, 116, 123, 127, 132, 137, 143
    };

    SDK_MINMAX_ASSERT(attack, 0, 127);

    if (attack < 109) {
        ch_p->attack = (u8)(255 - attack);
    } else {
        ch_p->attack = attack_table[127 - attack];
    }
}

void SND_SetExChannelDecay(SNDExChannel *ch_p, int decay)
{
    SDK_MINMAX_ASSERT(decay, 0, 127);

    ch_p->decay = CalcRelease(decay);
}

void SND_SetExChannelSustain(SNDExChannel *ch_p, int sustain)
{
    SDK_MINMAX_ASSERT(sustain, 0, 127);

    ch_p->sustain = sustain;
}

void SND_SetExChannelRelease(SNDExChannel *ch_p, int release)
{
    SDK_MINMAX_ASSERT(release, 0, 127);

    ch_p->release = CalcRelease(release);
}

void SND_ReleaseExChannel(SNDExChannel *ch_p)
{
    ch_p->env_status = SND_ENV_RELEASE;
}

BOOL SND_IsExChannelActive(SNDExChannel *ch_p)
{
    SDK_NULL_ASSERT(ch_p);

    return ch_p->active_flag;
}

SNDExChannel *SND_AllocExChannel(u32 chBitMask, int prio, BOOL strongRequest, SNDExChannelCallback callback, void *callbackData)
{
    static const u8 channel_order[SND_CHANNEL_NUM] = {
        4, 5, 6, 7,
        2, 0,
        3, 1,
        8, 9, 10, 11, 14, 12, 15, 13
    };

    chBitMask &= ~sLockChannel;
    if (!strongRequest) {
        chBitMask &= ~sWeakLockChannel;
    }

    SNDExChannel *ch_p = NULL;
    for (int i = 0; i < SND_CHANNEL_NUM; i++) {
        int chNo = channel_order[i];

        if ((chBitMask & (1 << chNo)) == 0) {
            continue;
        }

        SNDExChannel *ch2_p = &SNDi_Work.channel[chNo];

        if (ch_p == NULL) {
            ch_p = ch2_p;
            continue;
        }

        if (ch2_p->prio > ch_p->prio) {
            continue;
        }
        if (ch2_p->prio == ch_p->prio) {
            if (CompareExChannelVolume(ch_p, ch2_p) >= 0) {
                continue;
            }
        }

        ch_p = ch2_p;
    }

    if (ch_p == NULL) {
        return NULL;
    }
    if (prio < ch_p->prio) {
        return NULL;
    }

    if (ch_p->callback != NULL) {
        ch_p->callback(ch_p, SND_EX_CHANNEL_CALLBACK_DROP, ch_p->callback_data);
    }
    ch_p->sync_flag = SND_EX_CHANNEL_UPDATE_STOP;
    ch_p->active_flag = FALSE;

    InitAllocExChannel(ch_p, callback, callbackData, prio);

    return ch_p;
}

void SND_FreeExChannel(SNDExChannel *ch_p)
{
    if (ch_p == NULL) {
        return;
    }

    ch_p->callback = NULL;
    ch_p->callback_data = NULL;
}

void SND_StopUnlockedChannel(u32 chBitMask, u32 flags)
{
    SNDExChannel *ch_p;
    u32 mask = chBitMask;

    for (int chNo = 0; chNo < SND_CHANNEL_NUM && mask != 0; chNo++, mask >>= 1) {
        if ((mask & 1) == 0) {
            continue;
        }

        ch_p = &SNDi_Work.channel[chNo];

        if ((sLockChannel & (1 << chNo)) == 0) {
            if (ch_p->callback != NULL) {
                ch_p->callback(ch_p, SND_EX_CHANNEL_CALLBACK_DROP, ch_p->callback_data);
            }

            SND_StopChannel(chNo, FALSE);
            ch_p->prio = SND_EX_CHANNEL_STOP_PRIO;
            SND_FreeExChannel(ch_p);

            ch_p->sync_flag = 0;
            ch_p->active_flag = FALSE;
        }
    }
}

void SND_LockChannel(u32 chBitMask, u32 flags)
{
    SNDExChannel *ch_p;
    u32 mask = chBitMask;

    for (int chNo = 0; chNo < SND_CHANNEL_NUM && mask != 0; chNo++, mask >>= 1) {
        if ((mask & 1) == 0) {
            continue;
        }

        ch_p = &SNDi_Work.channel[chNo];

        if ((sLockChannel & (1 << chNo)) == 0) {
            if (ch_p->callback != NULL) {
                ch_p->callback(ch_p, SND_EX_CHANNEL_CALLBACK_DROP, ch_p->callback_data);
            }

            SND_StopChannel(chNo, FALSE);
            ch_p->prio = SND_EX_CHANNEL_STOP_PRIO;
            SND_FreeExChannel(ch_p);

            ch_p->sync_flag = 0;
            ch_p->active_flag = FALSE;
        }
    }

    if (flags & SND_LOCK_IMPLIED_ALLOC_CHANNEL) {
        sWeakLockChannel |= chBitMask;
    } else {
        sLockChannel |= chBitMask;
    }
}

void SND_UnlockChannel(u32 chBitMask, u32 flags)
{
    if (flags & SND_LOCK_IMPLIED_ALLOC_CHANNEL) {
        sWeakLockChannel &= ~chBitMask;
    } else {
        sLockChannel &= ~chBitMask;
    }
}

u32 SND_GetLockedChannel(u32 flags)
{
    if (flags & SND_LOCK_IMPLIED_ALLOC_CHANNEL) {
        return sWeakLockChannel;
    } else {
        return sLockChannel;
    }
}

void SND_InvalidateWave(const void *start, const void *end)
{
    for (u8 ch = 0; ch < SND_CHANNEL_NUM; ch++) {
        SNDExChannel *ch_p = &SNDi_Work.channel[ch];

        if (!ch_p->active_flag) {
            continue;
        }

        if (ch_p->type != SND_EX_CHANNEL_PCM) {
            continue;
        }

        if (start <= ch_p->data && ch_p->data <= end) {
            ch_p->start_flag = FALSE;
            SND_StopChannel(ch, FALSE);
        }
    }
}

void SND_InitLfoParam(SNDLfoParam *lfo)
{
    lfo->target = SND_LFO_PITCH;
    lfo->depth = 0;
    lfo->range = 1;
    lfo->speed = 16;
    lfo->delay = 0;
}

void SND_StartLfo(SNDLfo *lfo)
{
    lfo->counter = 0;
    lfo->delay_counter = 0;
}

void SND_UpdateLfo(SNDLfo *lfo)
{
    if (lfo->delay_counter < lfo->param.delay) {
        lfo->delay_counter++;
        return;
    }

    u32 offset = lfo->counter;
    offset += lfo->param.speed << 6;
    offset >>= 8;
    while (offset >= SND_SIN_PERIOD) {
        offset -= SND_SIN_PERIOD;
    }
    offset <<= 8;

    lfo->counter += lfo->param.speed << 6;
    lfo->counter &= 0xFF;

    lfo->counter |= offset;
}

s32 SND_GetLfoValue(SNDLfo *lfo)
{
    if (lfo->param.depth == 0) {
        return 0;
    }
    if (lfo->delay_counter < lfo->param.delay) {
        return 0;
    }

    u32 offset = lfo->counter;
    offset >>= 8;

    return SND_SinIdx((int)offset) * lfo->param.depth * lfo->param.range;
}

static u16 CalcRelease(int release)
{
    SDK_MINMAX_ASSERT(release, 0, 127);

    if (release == 127) {
        return 65535;
    }
    if (release == 126) {
        return 120 << SND_EX_CHANNEL_ENV_DECAY_SHIFT;
    }

    if (release < 50) {
        return (release << 1) + 1;
    } else {
        return (60 << SND_EX_CHANNEL_ENV_DECAY_SHIFT) / (126 - release);
    }
}

static void InitAllocExChannel(SNDExChannel *ch_p, SNDExChannelCallback callback, void *callbackData, int prio)
{
    SDK_MINMAX_ASSERT(prio, 0, 255);

    ch_p->nextLink = NULL;

    ch_p->callback = callback;
    ch_p->callback_data = callbackData;

    ch_p->length = 0;

    ch_p->prio = prio;
    ch_p->volume = 127;

    ch_p->start_flag = FALSE;
    ch_p->auto_sweep = TRUE;

    ch_p->key = SND_EX_CHANNEL_KEY_INIT;
    ch_p->original_key = SND_EX_CHANNEL_ORIGINAL_KEY_INIT;
    ch_p->velocity = SND_EX_CHANNEL_VELOCITY_INIT;
    ch_p->init_pan = SND_EX_CHANNEL_PAN_INIT;

    ch_p->user_decay = 0;
    ch_p->user_decay2 = 0;
    ch_p->user_pitch = 0;
    ch_p->user_pan = 0;
    ch_p->pan_range = 127;

    ch_p->sweep_pitch = 0;
    ch_p->sweep_length = 0;
    ch_p->sweep_counter = 0;

    SND_SetExChannelAttack(ch_p, SND_EX_CHANNEL_ATTACK_INIT);
    SND_SetExChannelDecay(ch_p, SND_EX_CHANNEL_DECAY_INIT);
    SND_SetExChannelSustain(ch_p, SND_EX_CHANNEL_SUSTAIN_INIT);
    SND_SetExChannelRelease(ch_p, SND_EX_CHANNEL_RELEASE_INIT);

    SND_InitLfoParam(&ch_p->lfo.param);
}

static void StartExChannel(SNDExChannel *ch_p, s32 length)
{
    ch_p->env_decay = SND_EX_CHANNEL_ENVDECAY_INIT;
    ch_p->env_status = SND_ENV_ATTACK;

    ch_p->length = length;

    SND_StartLfo(&ch_p->lfo);

    ch_p->start_flag = TRUE;
    ch_p->active_flag = TRUE;
}

static BOOL CompareExChannelVolume(const SNDExChannel *cp1, const SNDExChannel *cp2)
{
    static const u8 shift[4] = { 0, 1, 2, 4 };

    int vol1 = cp1->volume & 0xFF;
    int vol2 = cp2->volume & 0xFF;

    vol1 <<= 4;
    vol2 <<= 4;

    vol1 >>= shift[cp1->volume >> 8];
    vol2 >>= shift[cp2->volume >> 8];

    if (vol1 != vol2) {
        return vol1 < vol2 ? 1 : -1;
    }

    return 0;
}

static s32 SweepMain(SNDExChannel *ch_p, BOOL doPeriodicProc)
{
    if (ch_p->sweep_pitch == 0) {
        return 0;
    }
    if (ch_p->sweep_counter >= ch_p->sweep_length) {
        return 0;
    }

    s64 sweep_s64 = ch_p->sweep_pitch;
    sweep_s64 *= ch_p->sweep_length - ch_p->sweep_counter;
    sweep_s64 /= ch_p->sweep_length;

    if (doPeriodicProc && ch_p->auto_sweep) {
        ch_p->sweep_counter++;
    }

    return sweep_s64;
}

static s32 LfoMain(SNDExChannel *ch_p, BOOL doPeriodicProc)
{
    s64 lfo_value = SND_GetLfoValue(&ch_p->lfo);

    if (lfo_value != 0) {
        switch (ch_p->lfo.param.target) {
        case SND_LFO_VOLUME:
            lfo_value *= 60;
            break;
        case SND_LFO_PITCH:
            lfo_value *= (1 << SND_PITCH_DIVISION_BIT);
            break;
        case SND_LFO_PAN:
            lfo_value *= 64;
            break;
        }
        lfo_value >>= 14;
    }

    if (doPeriodicProc) {
        SND_UpdateLfo(&ch_p->lfo);
    }

    return lfo_value;
}
