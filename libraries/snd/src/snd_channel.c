#include <nitro/hw/ARM7/ioreg_SND.h>
#include <nitro/snd/common/channel.h>

int sMasterPan = -1;

u8 sOrgVolume[SND_CHANNEL_NUM];
u8 sOrgPan[SND_CHANNEL_NUM];
int sSurroundDecay;

int CalcSurroundDecay(int vol, int pan);

void SND_SetupChannelPcm(int chnIdx,
        const void * data,
        SNDWaveFormat format,
        SNDChannelLoop loop,
        int loopStart,
        int loopLength,
        int volume,
        SNDChannelDataShift shift,
        int timer,
        int pan)
{
    int off = chnIdx * 0x10;

    sOrgPan[chnIdx] = (u8)pan;
    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }

    sOrgVolume[chnIdx] = (u8)volume;
    if (sSurroundDecay > 0 && (1 << chnIdx) & 0xfff5) {
        volume = CalcSurroundDecay(volume, pan);
    }

    reg_SND_SOUNDX(off, NONE, CNT, OFF) = REG_SND_SOUND0CNT_FIELD(0, format, loop, 0, pan, 0, shift, volume);
    reg_SND_SOUNDX(off, NONE, TMR, OFF) = (u16)(0x10000 - timer);
    reg_SND_SOUNDX(off, PT, RPT, OFF) = (u16)loopStart;
    reg_SND_SOUNDX(off, LEN, RPT, OFF) = (u32)loopLength;
    reg_SND_SOUNDX(off, NONE, SAD, OFF) = (u32)data;
}

void SND_SetupChannelPsg(int chnIdx, SNDDuty duty, int volume, SNDChannelDataShift shift, int timer, int pan)
{
    int off = chnIdx * 0x10;

    sOrgPan[chnIdx] = (u8)pan;
    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }
    
    sOrgVolume[chnIdx] = (u8)volume;
    if (sSurroundDecay > 0 && (1 << chnIdx) & 0xfff5) {
        volume = CalcSurroundDecay(volume, pan);
    }

    reg_SND_SOUNDX(off, NONE, CNT, OFF) = REG_SND_SOUND0CNT_FIELD(0, 3, 0, duty, pan, 0, shift, volume);
    reg_SND_SOUNDX(off, NONE, TMR, OFF) = (u16)(0x10000 - timer);
}

void SND_SetupChannelNoise(int chnIdx, int volume, SNDChannelDataShift shift, int timer, int pan)
{
    int off = chnIdx * 0x10;

    sOrgPan[chnIdx] = (u8)pan;
    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }
    sOrgVolume[chnIdx] = (u8)volume;
    if (sSurroundDecay > 0 && (1 << chnIdx) & 0xfff5) {
        volume = CalcSurroundDecay(volume, pan);
    }

    reg_SND_SOUNDX(off, NONE, CNT, OFF) = REG_SND_SOUND0CNT_FIELD(0, 3, 0, 0, pan, 0, shift, volume);
    reg_SND_SOUNDX(off, NONE, TMR, OFF) = (u16)(0x10000 - timer);
}

void SND_StopChannel(int idx, s32 hold)
{
    vu32 *reg = &reg_SND_SOUNDX(idx);

    u32 v = *reg;

    v &= ~0x80000000;

    if (hold & 1) {
        v |= 0x8000;
    }

    *reg = v;
}

void SND_SetChannelVolume(int chnIdx, int vol, SNDChannelDataShift shift)
{
    sOrgVolume[chnIdx] = (u8)vol;

    if (sSurroundDecay > 0 && (1 << chnIdx) & 0xFFF5) {
        int pan = reg_SND_SOUNDX(chnIdx, PAN);
        vol = CalcSurroundDecay(vol, pan);
    }

    reg_SND_SOUNDX(chnIdx, VOL_16) = (u16)((shift << 8) | vol);
}

void SND_SetChannelTimer(int chnIdx, int timer)
{
    reg_SND_SOUNDX(chnIdx, NONE, TMR) = (u16)(0x10000 - timer);
}

void SND_SetChannelPan(int chnIdx, int pan)
{
    sOrgPan[chnIdx] = (u8)pan;

    if (sMasterPan >= 0) {
        pan = sMasterPan;
    }

    reg_SND_SOUNDX(chnIdx, PAN) = (u8)pan;

    if (sSurroundDecay > 0 && (1 << chnIdx) & 0xfff5) {
        reg_SND_SOUNDX(chnIdx, VOL) = (u8)CalcSurroundDecay(sOrgVolume[chnIdx], pan);
    }
}

BOOL SND_IsChannelActive(int chNo)
{
    return (reg_SND_SOUNDX(chNo, 8) & 0x80) != 0;
}

void SND_SetMasterPan(int pan)
{
    sMasterPan = pan;

    if (pan >= 0) {
        for (int i = 0; i < SND_CHANNEL_NUM; ++i) {
            reg_SND_SOUNDX(i, PAN) = (u8)pan;
        }
    } else {
        for (int i = 0; i < SND_CHANNEL_NUM; ++i) {
            reg_SND_SOUNDX(i, PAN) = sOrgPan[i];
        }
    }
}

u32 SND_GetChannelControl(int chNo)
{
    return reg_SND_SOUNDX(chNo);
}

void SNDi_SetSurroundDecay(int decay)
{
    sSurroundDecay = decay;

    for (int i = 0; i < SND_CHANNEL_NUM; ++i) {
        if ((1 << i) & 0xfff5) {
            int pan = reg_SND_SOUNDX(i, PAN);
            reg_SND_SOUNDX(i, VOL) = (u8)CalcSurroundDecay(sOrgVolume[i], pan);
        }
    }
}

int CalcSurroundDecay(int vol, int pan)
{
    if (pan < 24) {
        return vol * (sSurroundDecay * (pan + 40) + ((0x7fff - sSurroundDecay) << 6)) >> 21;
    } else if (pan <= 104) {
        return vol;
    } else {
        return vol * (-sSurroundDecay * (pan - 40) + ((sSurroundDecay + 0x7fff) << 6)) >> 21;
    }
}
