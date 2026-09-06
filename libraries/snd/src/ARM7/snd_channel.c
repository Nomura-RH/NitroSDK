#include "nitro/snd/common/channel.h"
#include <nitro/hw/ARM7/ioreg_SND.h>

#define SURROUND_CHANNEL_MASK 0xFFF5

int sSurroundDecay = 0;
int sMasterPan = -1;

u8 sOrgVolume[SND_CHANNEL_NUM];
u8 sOrgPan[SND_CHANNEL_NUM];

static int CalcSurroundDecay(int vol, int pan);

void SND_SetupChannelPcm(int chNo, const void *dataaddr, SNDWaveFormat format, SNDChannelLoop loop, int loopStart, int loopLen, int volume, SNDChannelDataShift shift, int timer, int pan)
{
    const int offset = SND_CHANNEL_REG_OFFSET(chNo);

    SDK_MINMAX_ASSERT(chNo, SND_CHANNEL_MIN, SND_CHANNEL_MAX);
    SDK_MINMAX_ASSERT(loopLen, 0, SND_CHANNEL_LOOP_LEN_MAX);
    SDK_ASSERT(((u32)dataaddr & ~SND_CHANNEL_SAD_MASK) == 0);
    SDK_MINMAX_ASSERT(loopStart, 0, SND_CHANNEL_LOOP_START_MAX);
    SDK_MINMAX_ASSERT(loopLen, 0, SND_CHANNEL_LOOP_LEN_MAX);
    SDK_MINMAX_ASSERT(volume, SND_CHANNEL_VOLUME_MIN, SND_CHANNEL_VOLUME_MAX);
    SDK_MINMAX_ASSERT(timer, SND_CHANNEL_TIMER_MIN, SND_CHANNEL_TIMER_MAX);
    SDK_MINMAX_ASSERT(pan, SND_CHANNEL_PAN_MIN, SND_CHANNEL_PAN_MAX);

    sOrgPan[chNo] = pan;
    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }

    sOrgVolume[chNo] = volume;
    if (sSurroundDecay > 0 && ((1 << chNo) & SURROUND_CHANNEL_MASK)) {
        volume = CalcSurroundDecay(volume, pan);
    }

#ifdef SDK_TEG
    pan = 127 - pan;
#endif

    *((REGType32v *)(REG_SOUND0CNT_ADDR + offset)) = REG_SND_SOUND0CNT_FIELD(FALSE, format, loop, 0, pan, 0, shift, volume);

    *((REGType16v *)(REG_SOUND0TMR_ADDR + offset)) = (u16)(0x10000 - timer);
    *((REGType16v *)(REG_SOUND0RPT_PT_ADDR + offset)) = (u16)loopStart;
    *((REGType32v *)(REG_SOUND0RPT_LEN_ADDR + offset)) = (u32)loopLen;
    *((REGType32v *)(REG_SOUND0SAD_ADDR + offset)) = (u32)dataaddr;
}

void SND_SetupChannelPsg(int chNo, SNDDuty duty, int volume, SNDChannelDataShift shift, int timer, int pan)
{
    const int offset = SND_CHANNEL_REG_OFFSET(chNo);

    SDK_MINMAX_ASSERT(chNo, SND_PSG_CHANNEL_MIN, SND_PSG_CHANNEL_MAX);
    SDK_MINMAX_ASSERT(volume, SND_CHANNEL_VOLUME_MIN, SND_CHANNEL_VOLUME_MAX);
    SDK_MINMAX_ASSERT(timer, SND_CHANNEL_TIMER_MIN, SND_CHANNEL_TIMER_MAX);
    SDK_MINMAX_ASSERT(pan, SND_CHANNEL_PAN_MIN, SND_CHANNEL_PAN_MAX);

    sOrgPan[chNo] = pan;
    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }

    sOrgVolume[chNo] = volume;
    if (sSurroundDecay > 0 && ((1 << chNo) & SURROUND_CHANNEL_MASK)) {
        volume = CalcSurroundDecay(volume, pan);
    }

#ifdef SDK_TEG
    pan = 127 - pan;
#endif

    *((REGType32v *)(REG_SOUND0CNT_ADDR + offset)) = REG_SND_SOUND0CNT_FIELD(FALSE, SND_WAVE_FORMAT_PSG, 0, duty, pan, 0, shift, volume);

    *((REGType16v *)(REG_SOUND0TMR_ADDR + offset)) = (u16)(0x10000 - timer);
}

void SND_SetupChannelNoise(int chNo, int volume, SNDChannelDataShift shift, int timer, int pan)
{
    const int offset = SND_CHANNEL_REG_OFFSET(chNo);

    SDK_MINMAX_ASSERT(chNo, SND_NOISE_CHANNEL_MIN, SND_NOISE_CHANNEL_MAX);
    SDK_MINMAX_ASSERT(volume, SND_CHANNEL_VOLUME_MIN, SND_CHANNEL_VOLUME_MAX);
    SDK_MINMAX_ASSERT(timer, SND_CHANNEL_TIMER_MIN, SND_CHANNEL_TIMER_MAX);
    SDK_MINMAX_ASSERT(pan, SND_CHANNEL_PAN_MIN, SND_CHANNEL_PAN_MAX);

    sOrgPan[chNo] = pan;
    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }

    sOrgVolume[chNo] = volume;
    if (sSurroundDecay > 0 && ((1 << chNo) & SURROUND_CHANNEL_MASK)) {
        volume = CalcSurroundDecay(volume, pan);
    }

#ifdef SDK_TEG
    pan = 127 - pan;
#endif

    *((REGType32v *)(REG_SOUND0CNT_ADDR + offset)) = REG_SND_SOUND0CNT_FIELD(FALSE, SND_WAVE_FORMAT_NOISE, 0, 0, pan, 0, shift, volume);

    *((REGType16v *)(REG_SOUND0TMR_ADDR + offset)) = (u16)(0x10000 - timer);
}

void SND_StopChannel(int chNo, s32 flags)
{
    SDK_MINMAX_ASSERT(chNo, SND_CHANNEL_MIN, SND_CHANNEL_MAX);

    REGType32v *reg = (REGType32v *)(REG_SOUND0CNT_ADDR + SND_CHANNEL_REG_OFFSET(chNo));
    u32 ctrl = *reg;
    ctrl &= ~REG_SND_SOUND0CNT_E_MASK;
    if (flags & SND_CHANNEL_STOP_HOLD) {
        ctrl |= REG_SND_SOUND0CNT_HOLD_MASK;
    }
    *reg = ctrl;
}

#ifdef SDK_CW_WA_OPT4
#pragma optimization_level 0
#endif
void SND_SetChannelVolume(int chNo, int volume, SNDChannelDataShift shift)
{
    SDK_MINMAX_ASSERT(chNo, SND_CHANNEL_MIN, SND_CHANNEL_MAX);
    SDK_MINMAX_ASSERT(volume, SND_CHANNEL_VOLUME_MIN, SND_CHANNEL_VOLUME_MAX);

    sOrgVolume[chNo] = volume;
    if (sSurroundDecay > 0 && ((1 << chNo) & SURROUND_CHANNEL_MASK)) {
        int pan = *((REGType8v *)(REG_SOUND0CNT_PAN_ADDR + SND_CHANNEL_REG_OFFSET(chNo)));
        volume = CalcSurroundDecay(volume, pan);
    }

    *((REGType16v *)(REG_SOUND0CNT_VOL_16_ADDR + SND_CHANNEL_REG_OFFSET(chNo))) = REG_SND_SOUND0CNT_VOL_16_FIELD(0, shift, volume);
}
#ifdef SDK_CW_WA_OPT4
#pragma optimization_level 4
#endif

void SND_SetChannelTimer(int chNo, int timer)
{
    SDK_MINMAX_ASSERT(chNo, SND_CHANNEL_MIN, SND_CHANNEL_MAX);
    SDK_MINMAX_ASSERT(timer, SND_CHANNEL_TIMER_MIN, SND_CHANNEL_TIMER_MAX);

    *((REGType16v *)(REG_SOUND0TMR_ADDR + SND_CHANNEL_REG_OFFSET(chNo))) = (u16)(0x10000 - timer);
}

void SND_SetChannelPan(int chNo, int pan)
{
    SDK_MINMAX_ASSERT(chNo, SND_CHANNEL_MIN, SND_CHANNEL_MAX);
    SDK_MINMAX_ASSERT(pan, SND_CHANNEL_PAN_MIN, SND_CHANNEL_PAN_MAX);

    sOrgPan[chNo] = pan;
    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }

#ifdef SDK_TEG
    pan = 127 - pan;
#endif

    *((REGType8v *)(REG_SOUND0CNT_PAN_ADDR + SND_CHANNEL_REG_OFFSET(chNo))) = (u8)pan;

    if (sSurroundDecay > 0 && ((1 << chNo) & SURROUND_CHANNEL_MASK)) {
        int volume = CalcSurroundDecay(sOrgVolume[chNo], pan);
        *((REGType8v *)(REG_SOUND0CNT_VOL_ADDR + SND_CHANNEL_REG_OFFSET(chNo))) = (u8)volume;
    }
}

BOOL SND_IsChannelActive(int chNo)
{
    SDK_MINMAX_ASSERT(chNo, SND_CHANNEL_MIN, SND_CHANNEL_MAX);

    u8 reg = *((REGType8v *)(REG_SOUND0CNT_8_ADDR + SND_CHANNEL_REG_OFFSET(chNo)));

    return (reg & REG_SND_SOUND0CNT_8_E_MASK) ? TRUE : FALSE;
}

void SND_SetMasterPan(int pan)
{
    int chNo;
    int offset;

    sMasterPan = pan;

    if (pan >= 0) {
        for (chNo = 0; chNo < SND_CHANNEL_NUM; chNo++) {
            offset = SND_CHANNEL_REG_OFFSET(chNo);
            *((REGType8v *)(REG_SOUND0CNT_PAN_ADDR + offset)) = (u8)pan;
        }
    } else {
        for (chNo = 0; chNo < SND_CHANNEL_NUM; chNo++) {
            offset = SND_CHANNEL_REG_OFFSET(chNo);
            *((REGType8v *)(REG_SOUND0CNT_PAN_ADDR + offset)) = sOrgPan[chNo];
        }
    }
}

u32 SND_GetChannelControl(int chNo)
{
    SDK_MINMAX_ASSERT(chNo, SND_CHANNEL_MIN, SND_CHANNEL_MAX);

    return *((REGType32v *)(REG_SOUND0CNT_ADDR + SND_CHANNEL_REG_OFFSET(chNo)));
}

void SNDi_SetSurroundDecay(int decay)
{
    int chNo;
    int volume;
    int pan;
    int offset;

    sSurroundDecay = decay;

    for (chNo = 0; chNo < SND_CHANNEL_NUM; chNo++) {
        if ((1 << chNo) & SURROUND_CHANNEL_MASK) {
            offset = SND_CHANNEL_REG_OFFSET(chNo);
            pan = *((REGType8v *)(REG_SOUND0CNT_PAN_ADDR + offset));

            volume = CalcSurroundDecay(sOrgVolume[chNo], pan);

            *((REGType8v *)(REG_SOUND0CNT_VOL_ADDR + offset)) = (u8)volume;
        }
    }
}

int CalcSurroundDecay(int volume, int pan)
{
    if (pan < 24) {
        volume *= sSurroundDecay * (pan + 40) + ((32767 - sSurroundDecay) << 6);
        volume >>= 15 + 6;
    } else if (pan > 104) {
        volume *= -sSurroundDecay * (pan - 40) + ((32767 + sSurroundDecay) << 6);
        volume >>= 15 + 6;
    }

    return volume;
}
