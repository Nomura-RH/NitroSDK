
    
#include <nitro/os/common/systemCall.h>
#include <nitro/snd/common/channel.h>
#include <nitro/snd/common/global.h>
#include <nitro/hw/ARM7/ioreg_SND.h>
#include <nitro/os/common/system.h>

#define SND_Disable() reg_SND_SOUNDCNT_8 &= ~0x80

void SND_Enable(void)
{
    reg_SND_SOUNDCNT_8 |= 0x80;
}

void SND_Shutdown(void)
{
    SND_Disable();

    for (int i = 0; i < SND_CHANNEL_NUM; ++i) {
        SND_StopChannel(i, 1);
    }

    reg_SND_SNDCAPXCNT(0) = 0;
    reg_SND_SNDCAPXCNT(1) = 0;
}

void SND_BeginSleep(void)
{
    SND_Disable();
    SVC_ResetSoundBias(0x80);
    OS_SpinWait(0x40000);
    PMi_ResetControl(1);
    reg_SND_POWCNT &= ~1;
}

void SND_EndSleep(void)
{
    reg_SND_POWCNT |= 1;
    PMi_SetControl(1);
    SVC_SetSoundBias(0x100);
    OS_SpinWait(0x7ab80);
    SND_Enable();
}

void SND_SetMasterVolume(int vol)
{
    reg_SND_SOUNDCNT_VOL = (u8)vol;
}

void SND_SetOutputSelector(SNDOutput left, SNDOutput right, SNDChannelOut channel1, SNDChannelOut channel3)
{
    int masterEnable = (reg_SND_SOUNDCNT_8 & 0x80) ? 1 : 0;
    reg_SND_SOUNDCNT_8 = REG_SND_SOUNDCNT_8_FIELD(masterEnable, channel3, channel1, right, left);
}
