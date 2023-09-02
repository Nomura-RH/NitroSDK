#include <nitro/fx/fx_trig.h>
#include <nitro/fx/fx_cp.h>
#include <nitro/fx/fx_const.h>

#define ARRAY_SIZE_90 1024
#define ARRAY_SIZE_180 2048
#define ARRAY_SIZE_270 3072
#define ARRAY_SIZE_360 4096

#define SINCOS_TABLE_SIZE (4096 * 2)

u16 FX_AsinIdx (fx32 x)
{
	int left;
	int right;
	int mid;
	fx16 value;
	int tmp = 0;

	SDK_MINMAX_ASSERT(x, FX32_SIN270, FX32_SIN90);

	if (x >= 0) {
		left = 0;
		right = ARRAY_SIZE_90;
	} else {
		left = ARRAY_SIZE_270;
		right = ARRAY_SIZE_360;
	}

	value = (fx16)x;

	while (left <= right) {
		mid = (left + right) / 2;
		if (FX_SinCosTable_[mid * 2] == value) {
			break;
		} else if (FX_SinCosTable_[mid * 2] < value)   {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	tmp = (mid * 2) * 65536 / SINCOS_TABLE_SIZE;
	return (u16)tmp;
}

u16 FX_AcosIdx (fx32 x)
{
	int left;
	int right;
	int mid;
	fx16 value;
	int tmp = 0;

	SDK_MINMAX_ASSERT(x, FX32_COS180, FX32_COS0);

	if (x >= 0) {
		left = 0;
		right = ARRAY_SIZE_90;
	} else {
		left = ARRAY_SIZE_90;
		right = ARRAY_SIZE_180;
	}

	value = (fx16)x;

	while (left <= right) {
		mid = (left + right) / 2;
		if (FX_SinCosTable_[mid * 2 + 1] == value) {
			break;
		} else if (FX_SinCosTable_[mid * 2 + 1] < value)   {
			right = mid - 1;
		} else {
			left = mid + 1;
		}
	}

	tmp = (mid * 2 + 1) * 65536 / SINCOS_TABLE_SIZE;
	return (u16)tmp;
}