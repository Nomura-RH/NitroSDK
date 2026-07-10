
#include "nitro/snd/common/util.h"
#include <nitro/snd/common/work.h>
#include <nitro/snd/common/channel.h>
#include <nitro/snd/common/exchannel.h>

static const u8 sSampleDataShiftTable[4] = {
    0, 1, 2, 4
};
static const u8 sChannelAllocationOrder[SND_CHANNEL_NUM] = {
    4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13
};
static const u8 sAttackCoeffTable[19] = {
    0, 1, 5, 14, 26, 38, 51, 63, 73, 84, 92, 100, 109, 116, 123, 127, 132, 137, 143
};

static u32 sWeakLockedChannelMask;
static u32 sLockedChannelMask;

static u16 CalcDelayCoeff(int vol);

static int ExChannelSweepUpdate(SNDExChannel *chn, BOOL doPeriodicProc);
static int ExChannelLfoUpdate(SNDExChannel *chn, BOOL doPeriodicProc);
static void ExChannelStart(SNDExChannel *chn, int length);
static int ExChannelVolumeCmp(SNDExChannel *chn_a, SNDExChannel *chn_b);
static void ExChannelSetup(SNDExChannel *chn, SNDExChannelCallback callback, void *callbackUserData, int priority);

void SND_ExChannelInit(void)
{
    SNDExChannel *chn;
    s32 i;

    for (i = 0; i < SND_CHANNEL_NUM; ++i) {
        chn = &SNDi_Work.channel[i];

        chn->myNo = (u8)i;
        chn->sync_flag = 0;
        chn->active_flag = FALSE;
    }

    sLockedChannelMask = 0;
    sWeakLockedChannelMask = 0;
}


void SND_UpdateExChannel(void)
{
    SNDExChannel *chn;
    s32 i;

    for (i = 0; i < SND_CHANNEL_NUM; i++) {
        chn = &SNDi_Work.channel[i];

        if (chn->sync_flag == 0) {
            continue;
        }

        if (chn->sync_flag & SND_EX_CHANNEL_UPDATE_STOP) {
            SND_StopChannel(i, 0);
        }

        if (chn->sync_flag & SND_EX_CHANNEL_UPDATE_START) {
            switch (chn->type) {
            case SND_EX_CHANNEL_PCM:
                SND_SetupChannelPcm(i,
                    chn->data,
                    chn->wave.format,
                    chn->wave.loopflag ? 1 : 2,
                    (s32)chn->wave.loopstart,
                    (s32)chn->wave.looplen,
                    chn->volume & 0xFF,
                    chn->volume >> 8,
                    chn->timer,
                    chn->pan);
                break;
            case SND_EX_CHANNEL_PSG:
                SND_SetupChannelPsg(i,
                    chn->duty,
                    chn->volume & 0xFF,
                    chn->volume >> 8,
                    chn->timer,
                    chn->pan);
                break;
            case SND_EX_CHANNEL_NOISE:
                SND_SetupChannelNoise(i, chn->volume & 0xFF, chn->volume >> 8, chn->timer, chn->pan);
                break;
            }
        } else {
            if (chn->sync_flag & SND_EX_CHANNEL_UPDATE_TIMER) {
                SND_SetChannelTimer(i, chn->timer);
            } if (chn->sync_flag & SND_EX_CHANNEL_UPDATE_VOLUME) {
                SND_SetChannelVolume(i, chn->volume & 0xFF, chn->volume >> 8);
            } if (chn->sync_flag & SND_EX_CHANNEL_UPDATE_PAN) {
                SND_SetChannelPan(i, chn->pan);
            }
        }
    }

    for (i = 0; i < SND_CHANNEL_NUM; i++)
    {
        chn = &SNDi_Work.channel[i];

        if (!chn->sync_flag) {
            continue;
        }

        if (chn->sync_flag & SND_EX_CHANNEL_UPDATE_START) {
            reg_SND_SOUNDX(i, 8) |= 0x80;
        }
        chn->sync_flag = 0;
    }
}

void SND_ExChannelMain(BOOL doPeriodicProc)
{
    SNDExChannel *chn;
    s32 i;
    s32 vol;
    s32 pitch;
    s32 pan;
    s32 lfo;
    s32 newTimer;

    for (i = 0; i < SND_CHANNEL_NUM; ++i) {
        vol = 0;
        pitch = 0;
        pan = 0;
        chn = &SNDi_Work.channel[i];

        if (!chn->active_flag)
            continue;

        if (chn->start_flag) {
            chn->sync_flag |= SND_EX_CHANNEL_UPDATE_START;
            chn->start_flag = FALSE;
        } else if (!SND_IsChannelActive(i)) {
            if (chn->callback) {
                chn->callback(chn, 1, chn->callback_data);
            } else {
                chn->prio = 0;
            }
            chn->volume = 0;
            chn->active_flag = FALSE;
            continue;
        }

        vol += SNDi_DecibelSquareTable[chn->velocity];
        pitch += (chn->key - chn->original_key) * 0x40;

        vol += SND_UpdateExChannelEnvelope(chn, doPeriodicProc);
        pitch += ExChannelSweepUpdate(chn, doPeriodicProc);

        vol += chn->user_decay;
        vol += chn->user_decay2;
        pitch += chn->user_pitch;

        lfo = ExChannelLfoUpdate(chn, doPeriodicProc);

        switch (chn->lfo.param.target) {
        case SND_LFO_VOLUME:
            if (vol > -0x8000) {
                vol += lfo;
            }
            break;
        case SND_LFO_PAN:
            pan += lfo;
            break;
        case SND_LFO_PITCH:
            pitch += lfo;
            break;
        }

        pan += chn->init_pan;
        if (chn->pan_range != 127) {
            pan = (pan * chn->pan_range + 0x40) >> 7;
        }
        pan += chn->user_pan;

        if (chn->env_status == SND_ENV_RELEASE && vol <= -723) {
            chn->sync_flag = SND_EX_CHANNEL_UPDATE_STOP;
            if (chn->callback) {
                chn->callback(chn, 1, chn->callback_data);
            } else {
                chn->prio = 0;
            }
            chn->volume = 0;
            chn->active_flag = 0;
        } else {
            vol = SND_CalcChannelVolume(vol);
            newTimer = SND_CalcTimer(chn->wave.timer, pitch);

            if (chn->type == SND_EX_CHANNEL_PSG) {
                newTimer = (u16)(newTimer & 0xfffc);
            }

            pan += 0x40;
            if (pan < 0) {
                pan = 0;
            } else if (pan > 127) {
                pan = 127;
            }

            if (vol != chn->volume) {
                chn->volume = (u16)vol;
                chn->sync_flag |= SND_EX_CHANNEL_UPDATE_VOLUME;
            }
            if (newTimer != chn->timer) {
                chn->timer = (u16)newTimer;
                chn->sync_flag |= SND_EX_CHANNEL_UPDATE_TIMER;
            }
            if (pan != chn->pan) {
                chn->pan = (u8)pan;
                chn->sync_flag |= SND_EX_CHANNEL_UPDATE_PAN;
            }
        }
    }
}

BOOL SND_StartExChannelPcm(SNDExChannel *chn, const struct SNDWaveParam *wave, const void *data, s32 length)
{
    chn->type = SND_EX_CHANNEL_PCM;
    chn->wave = *wave;
    chn->data = data;
    ExChannelStart(chn, length);
    return TRUE;
}

BOOL SND_StartExChannelPsg(SNDExChannel *chn, SNDDuty duty, s32 length)
{
    if (chn->myNo < 8) {
        return FALSE;
    } else if (chn->myNo > 13) {
        return FALSE;
    } else {
        chn->type = SND_EX_CHANNEL_PSG;
        chn->duty = duty;
        chn->wave.timer = 8006;
        ExChannelStart(chn, length);
        return TRUE;
    }
}

BOOL SND_StartExChannelNoise(SNDExChannel *chn, s32 length)
{
    if (chn->myNo < 14) {
        return FALSE;
    } else if (chn->myNo > 15) {
        return FALSE;
    } else {
        chn->type = SND_EX_CHANNEL_NOISE;
        chn->wave.timer = 8006;
        ExChannelStart(chn, length);
        return TRUE;
    }
}

s32 SND_UpdateExChannelEnvelope(SNDExChannel *chn, BOOL step)
{
    s32 sustain;

    if (step) {
        switch (chn->env_status) {
        case SND_ENV_ATTACK:
            chn->env_decay = -((-chn->env_decay * chn->attack) >> 8);
            if (chn->env_decay == 0) {
                chn->env_status = SND_ENV_DECAY;
            }
            break;
        case SND_ENV_DECAY:
            sustain = SNDi_DecibelSquareTable[chn->sustain] << 7;
            chn->env_decay -= chn->decay;
            if (chn->env_decay <= sustain) {
                chn->env_decay = sustain;
                chn->env_status = SND_ENV_SUSTAIN;
            }
            break;
        case SND_ENV_SUSTAIN:
            break;
        case SND_ENV_RELEASE:
            chn->env_decay -= chn->release;
            break;
        }
    }

    return chn->env_decay >> 7;
}

void SND_SetExChannelAttack(SNDExChannel *chn, int attack)
{
    if (attack < 109) {
        chn->attack = (u8)(255 - attack);
    } else {
        chn->attack = sAttackCoeffTable[127 - attack];
    }
}

void SND_SetExChannelDecay(SNDExChannel *chn, int decay)
{
    chn->decay = CalcDelayCoeff(decay);
}

void SND_SetExChannelSustain(SNDExChannel *chn, int sustain)
{
    chn->sustain = (u8)sustain;
}

void SND_SetExChannelRelease(SNDExChannel *chn, int release)
{
    chn->release = CalcDelayCoeff(release);
}

void SND_ReleaseExChannel(SNDExChannel *chn)
{
    chn->env_status = SND_ENV_RELEASE;
}

BOOL SND_IsExChannelActive(SNDExChannel *chn)
{
    return chn->active_flag;
}

SNDExChannel *SND_AllocExChannel(u32 channelMask, int priority, BOOL flags, SNDExChannelCallback callback, void *callbackUserData)
{
    SNDExChannel *chnPrev;
    int i;

    channelMask &= ~sLockedChannelMask;
    if (flags == 0) {
        channelMask &= ~sWeakLockedChannelMask;
    }

    chnPrev = NULL;

    for (i = 0; i < SND_CHANNEL_NUM; ++i) {
        SNDExChannel *chn;
        int channelCandidate;

        channelCandidate = sChannelAllocationOrder[i];

        if ((channelMask & (1 << channelCandidate)) == 0) {
            continue;
        }

        chn = &SNDi_Work.channel[channelCandidate];

        if (chnPrev == NULL) {
            chnPrev = chn;
            continue;
        }

        if (chn->prio > chnPrev->prio) {
            continue;
        }

        if (chn->prio == chnPrev->prio && ExChannelVolumeCmp(chnPrev, chn) >= 0) {
            continue;
        }

        chnPrev = chn;
    }

    if (chnPrev == NULL) {
        return NULL;
    }

    if (priority < chnPrev->prio) {
        return NULL;
    }

    if (chnPrev->callback) {
        chnPrev->callback(chnPrev, 0, chnPrev->callback_data);
    }

    chnPrev->sync_flag = SND_EX_CHANNEL_UPDATE_STOP;
    chnPrev->active_flag = FALSE;

    ExChannelSetup(chnPrev, callback, callbackUserData, priority);
    return chnPrev;
}

void SND_FreeExChannel(SNDExChannel *chn)
{
    if (chn) {
        chn->callback = NULL;
        chn->callback_data = NULL;
    }
}

void SND_StopUnlockedChannel(u32 channelMask, u32 weak)
{
    (void)weak;

    SNDExChannel *chn;

    for (int i = 0; i < SND_CHANNEL_NUM && channelMask != 0; ++i, channelMask >>= 1) {
        if ((channelMask & 1) == 0) {
            continue;
        }

        chn = &SNDi_Work.channel[i];

        if (sLockedChannelMask & (1 << i)) {
            continue;
        }

        if (chn->callback) {
            chn->callback(chn, 0, chn->callback_data);
        }

        SND_StopChannel(i, 0);
        chn->prio = 0;
        SND_FreeExChannel(chn);
        chn->sync_flag = 0;
        chn->active_flag = 0;
    }
}

void SND_LockChannel(u32 channelMask, u32 weak)
{
    SNDExChannel *chn;
    u32 j = channelMask;
    int i = 0;
    
    for (; i < SND_CHANNEL_NUM && j != 0; ++i, j >>= 1) {
        if ((j & 1) == 0) {
            continue;
        }

        chn = &SNDi_Work.channel[i];

        if (sLockedChannelMask & (1 << i)) {
            continue;
        }

        if (chn->callback) {
            chn->callback(chn, 0, chn->callback_data);
        }

        SND_StopChannel(i, 0);
        chn->prio = 0;
        SND_FreeExChannel(chn);
        chn->sync_flag = 0;
        chn->active_flag = 0;
    }

    if (weak & 1) {
        sWeakLockedChannelMask |= channelMask;
    } else {
        sLockedChannelMask |= channelMask;
    }
}

void SND_UnlockChannel(u32 channelMask, u32 weak)
{
    if (weak & 1) {
        sWeakLockedChannelMask &= ~channelMask;
    } else {
        sLockedChannelMask &= ~channelMask;
    }
}

u32 SND_GetLockedChannel(u32 weak)
{
    if (weak & 1) {
        return sWeakLockedChannelMask;
    } else {
        return sLockedChannelMask;
    }
}

void SND_InvalidateWave(const void *start, const void *end)
{
    for (u8 i = 0; i < SND_CHANNEL_NUM; ++i) {
        SNDExChannel *chn = &SNDi_Work.channel[i];

        if (chn->active_flag && chn->type == SND_EX_CHANNEL_PCM && start <= chn->data && chn->data <= end) {
            chn->start_flag = FALSE;
            SND_StopChannel(i, 0);
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
        ++lfo->delay_counter;
    } else {
        u32 tmp = lfo->counter;
        tmp += lfo->param.speed << 6;
        tmp >>= 8;
        while (tmp >= 0x80) {
            tmp -= 0x80;
        }
        lfo->counter += lfo->param.speed << 6;
        lfo->counter &= 0xff;
        lfo->counter |= tmp << 8;
    }
}

s32 SND_GetLfoValue(SNDLfo *lfo)
{
    if (lfo->param.depth == 0) {
        return 0;
    } else if (lfo->delay_counter < lfo->param.delay) {
        return 0;
    } else {
        return SND_SinIdx((s32)((u32)lfo->counter >> 8)) * lfo->param.depth * lfo->param.range;
    }
}

static u16 CalcDelayCoeff(int vol)
{
    if (vol == 127) {
        return 0xffff;
    } else if (vol == 126) {
        return 0x3c00;
    } else if (vol < 50) {
        return (u16)(vol * 2 + 1);
    } else {
        return (u16)(0x1e00 / (126 - vol));
    }
}

static void ExChannelSetup(SNDExChannel *chn, SNDExChannelCallback callback, void *callbackUserData, int priority)
{
    chn->nextLink = NULL;
    chn->callback = callback;
    chn->callback_data = callbackUserData;
    chn->length = 0;
    chn->prio = (u8)priority;
    chn->volume = 127;
    chn->start_flag = FALSE;
    chn->auto_sweep = TRUE;
    chn->key = 60;
    chn->original_key = 60;
    chn->velocity = 127;
    chn->init_pan = 0;
    chn->user_decay = 0;
    chn->user_decay2 = 0;
    chn->user_pitch = 0;
    chn->user_pan = 0;
    chn->pan_range = 127;
    chn->sweep_pitch = 0;
    chn->sweep_length = 0;
    chn->sweep_counter = 0;

    SND_SetExChannelAttack(chn, 127);
    SND_SetExChannelDecay(chn, 127);
    SND_SetExChannelSustain(chn, 127);
    SND_SetExChannelRelease(chn, 127);
    SND_InitLfoParam(&chn->lfo.param);
}

static void ExChannelStart(SNDExChannel *chn, int length)
{
    chn->env_decay = -92544;
    chn->env_status = SND_ENV_ATTACK;
    chn->length = length;
    SND_StartLfo(&chn->lfo);
    chn->start_flag = TRUE;
    chn->active_flag = TRUE;
}

static int ExChannelVolumeCmp(SNDExChannel *chn_a, SNDExChannel *chn_b)
{
    int vol_a = chn_a->volume & 0xff;
    int vol_b = chn_b->volume & 0xff;

    vol_a <<= 4;
    vol_b <<= 4;

    vol_a >>= sSampleDataShiftTable[chn_a->volume >> 8];
    vol_b >>= sSampleDataShiftTable[chn_b->volume >> 8];

    if (vol_a != vol_b) {
        if (vol_a < vol_b) {
            return 1;
        } else {
            return -1;
        }
    }
    return 0;
}

static int ExChannelSweepUpdate(SNDExChannel *chn, BOOL doPeriodicProc)
{
    s64 result;

    if (chn->sweep_pitch == 0) {
        result = 0;
    } else if (chn->sweep_counter >= chn->sweep_length) {
        result = 0;
    } else {
        result = (s64)chn->sweep_pitch * (chn->sweep_length - chn->sweep_counter) / chn->sweep_length;

        if (doPeriodicProc && chn->auto_sweep) {
            ++chn->sweep_counter;
        }
    }

    return (int)result;
}

static int ExChannelLfoUpdate(SNDExChannel *chn, BOOL doPeriodicProc)
{
    s64 result = SND_GetLfoValue(&chn->lfo);

    if (result != 0) {
        switch (chn->lfo.param.target) {
        case SND_LFO_VOLUME:
            result *= 60;
            break;
        case SND_LFO_PITCH:
            result <<= 6;
            break;
        case SND_LFO_PAN:
            result <<= 6;
            break;
        }
        result >>= 14;
    }

    if (doPeriodicProc) {
        SND_UpdateLfo(&chn->lfo);
    }

    return (int)result;
}

