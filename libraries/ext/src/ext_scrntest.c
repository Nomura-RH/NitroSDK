#include <nitro.h>

typedef u16 CapturedPixel;

#define MASK_CAPTURED_PIX_R         0x001f
#define SHIFT_CAPTURED_PIX_R        0

#define MASK_CAPTURED_PIX_G         0x03e0
#define SHIFT_CAPTURED_PIX_G        5

#define MASK_CAPTURED_PIX_B         0x7C00
#define SHIFT_CAPTURED_PIX_B        10

#define MASK_CAPTURED_PIX_ALPHA     0x8000
#define SHIFT_CAPTURED_PIX_ALPHA    15

const static u16 EXT_SCRN_W_  = 256;
const static u16 EXT_SCRN_H_  = 192;
const static u32 COUNTER_MAX_ = 0xFFFFFFFE;

static u32 calcScreenShotCheckSum_(void);

static void startCapture_(void);
static CapturedPixel *getCapturedPixel_(u16 x, u16 y);
static CapturedPixel *getCapturedBufferBase_(void);
static u8 getCapturedPixR_(const CapturedPixel *pPix);
static u8 getCapturedPixG_(const CapturedPixel *pPix);
static u8 getCapturedPixB_(const CapturedPixel *pPix);
static BOOL getCapturedPixAlpha_(const CapturedPixel *pPix);

static GXDispMode getDispMode_(void);
static GXCaptureMode getCaptureMode_(GXDispMode mode);
static GXCaptureSrcB getCaptureSrcB_(GXDispMode mode);
static GXCaptureDest getCaptureDest_(void);

static u32 frameCounter = 0x0;
static GXVRamLCDC vramForCapture_ = GX_VRAM_LCDC_C;

void EXT_Printf (const char *fmt, ...)
{
	va_list vlist;

	OS_Printf("<AUTO-TEST> ");
	va_start(vlist, fmt);
	OS_VPrintf(fmt, vlist);
	va_end(vlist);
}

void EXT_CompPrint (const char *src1, const char *src2_fmt, ...)
{
	va_list vlist;
	char src2[256];
	s32 i;

	va_start(vlist, src2_fmt);
	(void)OS_VSPrintf(src2, src2_fmt, vlist);
	va_end(vlist);

	for (i = 0;; i++) {
		if (src1[i] != src2[i]) {
			EXT_Printf("PrintString = \"%s\" : \"%s\"\n", src1, src2);
			EXT_Printf("PrintCompare Test [Fail]\n");
			return;
		}

		if (src1[i] == '\0' && src2[i] == '\0') {
			break;
		}
	}

	EXT_Printf("PrintCompare Test [Success]\n");
}

void EXT_TestScreenShot (u32 testFrame, u32 checkSum)
{
	const u32 prevTestFrame = testFrame - 1;
	const u32 nextTestFrame = testFrame + 1;

	SDK_ASSERTMSG((testFrame > 0) && (testFrame < (u32)COUNTER_MAX_), "illegal input value for numFrames in EXT_TestScreenShot");

	if (frameCounter == prevTestFrame) {
		startCapture_();
	}

	{
		if (frameCounter == nextTestFrame) {
			u32 currentSum = 0x0;

			EXT_Printf("ScreenShot Test at frameCounter = %d\n", testFrame);

			currentSum = calcScreenShotCheckSum_();

			EXT_Printf("CheckSum = %X\n", currentSum);
			if (checkSum == currentSum) {
				EXT_Printf("ScreenShot Test [Success]\n");
			} else {
				EXT_Printf("ScreenShot Test [Fail]\n");
			}

		}
	}

}

void EXT_TestTickCounter (void)
{
	frameCounter++;
}

void EXT_TestResetCounter (void)
{
	frameCounter = 0;
}

void EXT_TestSetVRAMForScreenShot (GXVRamLCDC vram)
{
	SDK_ASSERTMSG(vram == GX_VRAM_LCDC_A || vram == GX_VRAM_LCDC_B || vram == GX_VRAM_LCDC_C
	              || vram == GX_VRAM_LCDC_D, "Currentry VRAM_A B C D are supported for capturing.");

	vramForCapture_ = vram;
}

static void startCapture_ (void)
{
	const GXDispMode dispMode = getDispMode_();

	GX_SetCapture(GX_CAPTURE_SIZE_256x192,
	              getCaptureMode_(dispMode), GX_CAPTURE_SRCA_2D3D, getCaptureSrcB_(dispMode),
	              getCaptureDest_(), 16, 0);

	{
		char vram = '*';
		switch (vramForCapture_) {
		case GX_VRAM_LCDC_A:
			vram = 'A';
			break;
		case GX_VRAM_LCDC_B:
			vram = 'B';
			break;
		case GX_VRAM_LCDC_C:
			vram = 'C';
			break;
		case GX_VRAM_LCDC_D:
			vram = 'D';
			break;
		default:
			SDK_INTERNAL_ERROR("UnExpected VRAM type in startCapture_()");
			break;
		}
		EXT_Printf("Capture to VRAM %c for ScreenShot Test \n", vram);
	}
}

static GXDispMode getDispMode_ (void)
{
	const GXDispMode ret = (GXDispMode)((reg_GX_DISPCNT & REG_GX_DISPCNT_MODE_MASK) >> REG_GX_DISPCNT_MODE_SHIFT);
	GX_DISPMODE_ASSERT(ret);

	return ret;
}

static GXCaptureMode getCaptureMode_ (GXDispMode mode)
{
	if (mode == GX_DISPMODE_GRAPHICS) {
		return GX_CAPTURE_MODE_A;
	} else {
		return GX_CAPTURE_MODE_B;
	}
}

static GXCaptureSrcB getCaptureSrcB_ (GXDispMode mode)
{
	if (mode == GX_DISPMODE_GRAPHICS) {
		return (GXCaptureSrcB)0;
	} else {
		if (mode == GX_DISPMODE_MMEM) {
			return GX_CAPTURE_SRCB_MRAM;
		} else {
			return GX_CAPTURE_SRCB_VRAM_0x00000;
		}
	}
}

static GXCaptureDest getCaptureDest_ (void)
{
	switch (vramForCapture_) {
	case GX_VRAM_LCDC_A:
		return GX_CAPTURE_DEST_VRAM_A_0x00000;
	case GX_VRAM_LCDC_B:
		return GX_CAPTURE_DEST_VRAM_B_0x00000;
	case GX_VRAM_LCDC_C:
		return GX_CAPTURE_DEST_VRAM_C_0x00000;
	case GX_VRAM_LCDC_D:
		return GX_CAPTURE_DEST_VRAM_D_0x00000;
	default:
		SDK_INTERNAL_ERROR("UnExpected VRAM type in getCaptureDest_()");
		return GX_CAPTURE_DEST_VRAM_C_0x00000;
	}
}

static u32 calcScreenShotCheckSum_ (void)
{
	u16 i, j;
	u32 sum = 0x0;
	const CapturedPixel *pPx = NULL;

	for (j = 0; j < EXT_SCRN_H_; j++) {
		for (i = 0; i < EXT_SCRN_W_; i++) {
			pPx = getCapturedPixel_(i, j);
			SDK_NULL_ASSERT(pPx);
			sum += (u32)(*pPx) * (i + j);
		}
	}

	return sum;
}

static CapturedPixel *getCapturedPixel_ (u16 x, u16 y)
{
	SDK_MINMAX_ASSERT(x, 0, EXT_SCRN_W_);
	SDK_MINMAX_ASSERT(y, 0, EXT_SCRN_H_);

	return (CapturedPixel *) (getCapturedBufferBase_()) + ((EXT_SCRN_W_ * y) + x);
}

static CapturedPixel *getCapturedBufferBase_ (void)
{
	switch (vramForCapture_) {
	case GX_VRAM_LCDC_A:
		return (CapturedPixel *) HW_LCDC_VRAM_A;
	case GX_VRAM_LCDC_B:
		return (CapturedPixel *) HW_LCDC_VRAM_B;
	case GX_VRAM_LCDC_C:
		return (CapturedPixel *) HW_LCDC_VRAM_C;
	case GX_VRAM_LCDC_D:
		return (CapturedPixel *) HW_LCDC_VRAM_D;
	default:
		SDK_INTERNAL_ERROR("UnExpected VRAM type in getCapturedBufferBase_()");
		return (CapturedPixel *) NULL;
	}
}

static u8 getCapturedPixR_ (const CapturedPixel *pPix)
{
	SDK_NULL_ASSERT(pPix);
	return (u8)((MASK_CAPTURED_PIX_R & *pPix) >> SHIFT_CAPTURED_PIX_R);
}

static u8 getCapturedPixG_ (const CapturedPixel *pPix)
{
	SDK_NULL_ASSERT(pPix);
	return (u8)((MASK_CAPTURED_PIX_G & *pPix) >> SHIFT_CAPTURED_PIX_G);
}

static u8 getCapturedPixB_ (const CapturedPixel *pPix)
{
	SDK_NULL_ASSERT(pPix);
	return (u8)((MASK_CAPTURED_PIX_B & *pPix) >> SHIFT_CAPTURED_PIX_B);
}

static BOOL getCapturedPixAlpha_ (const CapturedPixel *pPix)
{
	SDK_NULL_ASSERT(pPix);
	return (BOOL)((MASK_CAPTURED_PIX_R & *pPix) >> SHIFT_CAPTURED_PIX_ALPHA);
}