#include "nitro/snd/common/global.h"
#include "nitro/snd/common/capture.h"
#include "nitro/snd/common/channel.h"
#include <nitro/hw/ARM7/ioreg_SND.h>
#include <nitro/os/common/system.h>
#include <nitro/os/common/systemCall.h>
#include <nitro/spi/common/pm_common.h>

#define SOUND_BIAS_WAIT_COUNT     128
#define SOUND_BIAS_LEVEL          0x200
#define SOUND_BIAS_CYCLE_PER_LOOP 4

void SND_Enable(void)
{
    reg_SND_SOUNDCNT_8 |= REG_SND_SOUNDCNT_8_E_MASK;
}

void SND_Disable(void)
{
    reg_SND_SOUNDCNT_8 &= ~REG_SND_SOUNDCNT_8_E_MASK;
}

void SND_Shutdown(void)
{
    SND_Disable();

    for (int i = 0; i < SND_CHANNEL_NUM; ++i) {
        SND_StopChannel(i, SND_CHANNEL_STOP_HOLD);
    }

    SND_StopCapture(SND_CAPTURE_0);
    SND_StopCapture(SND_CAPTURE_1);
}

void SND_BeginSleep(void)
{
    SND_Disable();

    SVC_ResetSoundBias(SOUND_BIAS_WAIT_COUNT);
    OS_SpinWait(SOUND_BIAS_CYCLE_PER_LOOP * SOUND_BIAS_WAIT_COUNT * SOUND_BIAS_LEVEL);

    PMi_ResetControl(PMIC_CTL_SND_PWR);

    reg_SND_POWCNT &= ~REG_SND_POWCNT_SPE_MASK;
}

void SND_EndSleep(void)
{
    reg_SND_POWCNT |= REG_SND_POWCNT_SPE_MASK;

    PMi_SetControl(PMIC_CTL_SND_PWR);

    SVC_SetSoundBias(SOUND_BIAS_WAIT_COUNT * 2);

    OS_SpinWait(OS_MilliSecondsToTicks(15) * 64);

    SND_Enable();
}

void SND_SetMasterVolume(int volume)
{
    SDK_MINMAX_ASSERT(volume, 0, SND_MASTER_VOLUME_MAX);

    reg_SND_SOUNDCNT_VOL = (u8)volume;
}

void SND_SetOutputSelector(SNDOutput left, SNDOutput right, SNDChannelOut channel1, SNDChannelOut channel3)
{
    BOOL enable = (reg_SND_SOUNDCNT_8 & REG_SND_SOUNDCNT_8_E_MASK) ? TRUE : FALSE;

    reg_SND_SOUNDCNT_8 = REG_SND_SOUNDCNT_8_FIELD(enable, channel3, channel1, right, left);
}
