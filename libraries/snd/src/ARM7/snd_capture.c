#include "nitro/snd/common/capture.h"
#include <nitro/hw/ARM7/ioreg_SND.h>

void SND_SetupCapture(SNDCapture capture, SNDCaptureFormat format, void *buffer_addr, int length, BOOL repeat, SNDCaptureIn in, SNDCaptureOut out)
{
    const int offset = SND_CAPTURE_REG_OFFSET * capture;

    SDK_ASSERT(((u32)buffer_addr & ~SND_CAPTURE_DAD_MASK) == 0);
    SDK_MINMAX_ASSERT(length, 0, SND_CAPTURE_LEN_MAX);

    (*(REGType8v *)(REG_SNDCAP0CNT_ADDR + capture)) = REG_SND_SNDCAP0CNT_FIELD(0, format, repeat ? SND_CAPTURE_REPEAT_YES : SND_CAPTURE_REPEAT_NO, in, out);

    (*(REGType32v *)(REG_SNDCAP0DAD_ADDR + offset)) = (u32)buffer_addr;
    (*(REGType16v *)(REG_SNDCAP0LEN_ADDR + offset)) = (u16)length;
}

BOOL SND_IsCaptureActive(SNDCapture capture)
{
    return ((*(REGType8v *)(REG_SNDCAP0CNT_ADDR + capture)) & REG_SND_SNDCAP0CNT_E_MASK) != 0;
}
