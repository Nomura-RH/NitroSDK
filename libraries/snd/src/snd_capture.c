#include <nitro/hw/ARM7/ioreg_SND.h>
#include <nitro/snd/common/capture.h>

void SND_SetupCapture(SNDCapture idx, SNDCaptureFormat format, void *captureData, int size, BOOL loop, SNDCaptureIn capCtrlSrc, SNDCaptureOut capCtrlDst)
{
    int off = idx * 8;

    reg_SND_SNDCAPXCNT(idx) = (u8)((format << 3) | ((loop ? SND_CAPTURE_REPEAT_YES : SND_CAPTURE_REPEAT_NO) << 2) | (capCtrlSrc << 1) | capCtrlDst);
    *(REGType32v *)(REG_SNDCAP0DAD_ADDR + off) = (u32)captureData;
    *(REGType16v *)(REG_SNDCAP0LEN_ADDR + off) = (u16) size;
}

BOOL SND_IsCaptureActive(SNDCapture capture)
{
    return (reg_SND_SNDCAPXCNT(capture) & 0x80) != 0;
}
