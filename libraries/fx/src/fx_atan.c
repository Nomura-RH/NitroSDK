#include <nitro/fx/fx_trig.h>
#include <nitro/fx/fx_cp.h>
#include <nitro/fx/fx_const.h>

const fx16 FX_AtanTable_[128 + 1] = {
	(fx16)0x0000,
	(fx16)0x0020,
	(fx16)0x0040,
	(fx16)0x0060,
	(fx16)0x0080,
	(fx16)0x00a0,
	(fx16)0x00c0,
	(fx16)0x00e0,
	(fx16)0x0100,
	(fx16)0x0120,
	(fx16)0x013f,
	(fx16)0x015f,
	(fx16)0x017f,
	(fx16)0x019f,
	(fx16)0x01be,
	(fx16)0x01de,
	(fx16)0x01fd,
	(fx16)0x021d,
	(fx16)0x023c,
	(fx16)0x025c,
	(fx16)0x027b,
	(fx16)0x029a,
	(fx16)0x02b9,
	(fx16)0x02d8,
	(fx16)0x02f7,
	(fx16)0x0316,
	(fx16)0x0335,
	(fx16)0x0354,
	(fx16)0x0372,
	(fx16)0x0391,
	(fx16)0x03af,
	(fx16)0x03cd,
	(fx16)0x03eb,
	(fx16)0x0409,
	(fx16)0x0427,
	(fx16)0x0445,
	(fx16)0x0463,
	(fx16)0x0481,
	(fx16)0x049e,
	(fx16)0x04bb,
	(fx16)0x04d9,
	(fx16)0x04f6,
	(fx16)0x0513,
	(fx16)0x052f,
	(fx16)0x054c,
	(fx16)0x0569,
	(fx16)0x0585,
	(fx16)0x05a1,
	(fx16)0x05be,
	(fx16)0x05da,
	(fx16)0x05f5,
	(fx16)0x0611,
	(fx16)0x062d,
	(fx16)0x0648,
	(fx16)0x0663,
	(fx16)0x067e,
	(fx16)0x0699,
	(fx16)0x06b4,
	(fx16)0x06cf,
	(fx16)0x06e9,
	(fx16)0x0703,
	(fx16)0x071e,
	(fx16)0x0738,
	(fx16)0x0751,
	(fx16)0x076b,
	(fx16)0x0785,
	(fx16)0x079e,
	(fx16)0x07b7,
	(fx16)0x07d0,
	(fx16)0x07e9,
	(fx16)0x0802,
	(fx16)0x081a,
	(fx16)0x0833,
	(fx16)0x084b,
	(fx16)0x0863,
	(fx16)0x087b,
	(fx16)0x0893,
	(fx16)0x08aa,
	(fx16)0x08c2,
	(fx16)0x08d9,
	(fx16)0x08f0,
	(fx16)0x0907,
	(fx16)0x091e,
	(fx16)0x0934,
	(fx16)0x094b,
	(fx16)0x0961,
	(fx16)0x0977,
	(fx16)0x098d,
	(fx16)0x09a3,
	(fx16)0x09b9,
	(fx16)0x09ce,
	(fx16)0x09e3,
	(fx16)0x09f9,
	(fx16)0x0a0e,
	(fx16)0x0a23,
	(fx16)0x0a37,
	(fx16)0x0a4c,
	(fx16)0x0a60,
	(fx16)0x0a74,
	(fx16)0x0a89,
	(fx16)0x0a9c,
	(fx16)0x0ab0,
	(fx16)0x0ac4,
	(fx16)0x0ad7,
	(fx16)0x0aeb,
	(fx16)0x0afe,
	(fx16)0x0b11,
	(fx16)0x0b24,
	(fx16)0x0b37,
	(fx16)0x0b49,
	(fx16)0x0b5c,
	(fx16)0x0b6e,
	(fx16)0x0b80,
	(fx16)0x0b92,
	(fx16)0x0ba4,
	(fx16)0x0bb6,
	(fx16)0x0bc8,
	(fx16)0x0bd9,
	(fx16)0x0beb,
	(fx16)0x0bfc,
	(fx16)0x0c0d,
	(fx16)0x0c1e,
	(fx16)0x0c2f,
	(fx16)0x0c3f,
	(fx16)0x0c50,
	(fx16)0x0c60,
	(fx16)0x0c71,
	(fx16)0x0c81,
	(fx16)0x0c91
};

fx16 FX_Atan (fx32 x)
{
	if (x >= 0) {
		if (x > FX32_ONE) {
			return (fx16)(6434 - FX_AtanTable_[FX_Inv(x) >> 5]);
		} else if (x < FX32_ONE)   {
			return FX_AtanTable_[x >> 5];
		} else {
			return (fx16)3217;
		}
	} else {
		if (x < -FX32_ONE) {
			return (fx16)(-6434 + FX_AtanTable_[FX_Inv(-x) >> 5]);
		} else if (x > -FX32_ONE)   {
			return (fx16) - FX_AtanTable_[-x >> 5];
		} else {
			return (fx16) - 3217;
		}
	}
}

fx16 FX_Atan2 (fx32 y, fx32 x)
{
	fx32 a, b, c;
	int sgn;

	if (y > 0) {
		if (x > 0) {
			if (x > y) {
				a = y;
				b = x;
				c = 0;
				sgn = 1;
			} else if (x < y)   {
				a = x;
				b = y;
				c = 6434;
				sgn = 0;
			} else {
				return (fx16)3217;
			}
		} else if (x < 0)   {
			x = -x;
			if (x < y) {
				a = x;
				b = y;
				c = 6434;
				sgn = 1;
			} else if (x > y)   {
				a = y;
				b = x;
				c = 12868;
				sgn = 0;
			} else {
				return (fx16)9651;
			}
		} else {
			return (fx16)6434;
		}
	} else if (y < 0)   {
		y = -y;
		if (x < 0) {
			x = -x;
			if (x > y) {
				a = y;
				b = x;
				c = -12868;
				sgn = 1;
			} else if (x < y)   {
				a = x;
				b = y;
				c = -6434;
				sgn = 0;
			} else {
				return (fx16) - 9651;
			}
		} else if (x > 0)   {
			if (x < y) {
				a = x;
				b = y;
				c = -6434;
				sgn = 1;
			} else if (x > y)   {
				a = y;
				b = x;
				c = 0;
				sgn = 0;
			} else {
				return (fx16) - 3217;
			}
		} else {
			return (fx16) - 6434;
		}
	} else {
		if (x >= 0) {
			return 0;
		} else {
			return (fx16)12868;
		}
	}

	if (b == 0)
		return 0;
	if (sgn)
		return (fx16)(c + FX_AtanTable_[FX_Div(a, b) >> 5]);
	else
		return (fx16)(c - FX_AtanTable_[FX_Div(a, b) >> 5]);
}
