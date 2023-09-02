#include <nitro.h>
#include <nitro/fx/fx_trig.h>

void MATH_MakeFFTSinTable (fx16 *sinTable, u32 nShift)
{
	u32 i;
	u32 n = 1U << nShift;
	u32 nq3 = (n >> 1) + (n >> 2);
	s32 w;
	u32 dw;

	SDK_ASSERT(nShift < 32);

	dw = 65536U >> nShift;
	for (i = 0, w = 0; i < nq3; i++, w += dw) {
		sinTable[i] = FX_SinIdx(w);
	}
}

#define SWAP_FX32(a, b) {fx32 t; t = (a); (a) = (b); (b) = t;}
void MATHi_FFT (fx32 *data, u32 nShift, const fx16 *sinTable)
{
	u32 i, j;
	u32 n = 1U << nShift;
	u32 n2 = n << 1;
	u32 nq = n >> 2;

	{
		u32 rev = 0;
		u32 shift = 32 - nShift - 1;
		for (i = 0; i < n2; i += 2) {
			j = rev >> shift;
			if (i < j) {

				SWAP_FX32(data[i], data[j]);
				SWAP_FX32(data[i + 1], data[j + 1]);
			}

			{
				u32 s;
				s = MATH_CLZ(~rev);
				rev ^= (((s32)(0x80000000U)) >> s);
			}
		}
	}

	{
		u32 div, dt;

		dt = n;

		for (div = 2; div < n2; div <<= 1) {
			u32 k;
			u32 di = div * 2;
			u32 t;
			dt >>= 1;

			for (i = 0; i < n2; i += di) {
				fx32 xr, xi, yr, yi;
				u32 j = i + div;

				xr = data[i];
				xi = data[i + 1];
				yr = data[j];
				yi = data[j + 1];
				data[i] = xr + yr;
				data[i + 1] = xi + yi;
				data[j] = xr - yr;
				data[j + 1] = xi - yi;
			}

			t = dt;

			for (k = 2; k < div; k += 2) {
				if (k != div / 2) {
					fx32 wr, wi, w1, w2;
					wr = sinTable[t + nq];
					wi = -sinTable[t];

					t += dt;

					w1 = wr + wi;
					w2 = wr - wi;

					for (i = k; i < n2; i += di) {
						fx32 xr, xi, yr, yi;
						u32 j = i + div;

						xr = data[j];
						xi = data[j + 1];
						{

							fx32 t = FX_Mul(wr, xr + xi);
							yr = t - FX_Mul(w1, xi);
							yi = t - FX_Mul(w2, xr);
						}

						xr = data[i];
						xi = data[i + 1];
						data[i] = xr + yr;
						data[i + 1] = xi + yi;
						data[j] = xr - yr;
						data[j + 1] = xi - yi;
					}
				} else {
					t += dt;

					for (i = k; i < n2; i += di) {
						fx32 xr, xi, yr, yi;
						u32 j = i + div;

						xr = data[i];
						xi = data[i + 1];
						yr = data[j];
						yi = data[j + 1];

						data[i] = xr + yi;
						data[i + 1] = xi - yr;
						data[j] = xr - yi;
						data[j + 1] = xi + yr;
					}
				}
			}
		}
	}
}

void MATHi_IFFT (fx32 *data, u32 nShift, const fx16 *sinTable)
{
	u32 i, j;
	u32 n = 1U << nShift;
	u32 n2 = n << 1;
	u32 nq = n >> 2;

	{
		u32 rev = 0;
		u32 shift = 32 - nShift - 1;
		for (i = 0; i < n2; i += 2) {
			j = rev >> shift;
			if (i < j) {

				SWAP_FX32(data[i], data[j]);
				SWAP_FX32(data[i + 1], data[j + 1]);
			}

			{
				u32 s;
				s = MATH_CLZ(~rev);
				rev ^= (((s32)(0x80000000U)) >> s);
			}
		}
	}

	{
		u32 div, dt;

		dt = n;

		for (div = 2; div < n2; div <<= 1) {
			u32 k;
			u32 di = div * 2;
			u32 t;
			dt >>= 1;

			for (i = 0; i < n2; i += di) {
				fx32 xr, xi, yr, yi;
				u32 j = i + div;

				xr = data[i];
				xi = data[i + 1];
				yr = data[j];
				yi = data[j + 1];
				data[i] = xr + yr;
				data[i + 1] = xi + yi;
				data[j] = xr - yr;
				data[j + 1] = xi - yi;
			}

			t = dt;

			for (k = 2; k < div; k += 2) {
				if (k != div / 2) {
					fx32 wr, wi, w1, w2;
					wr = sinTable[t + nq];
					wi = sinTable[t];
					t += dt;

					w1 = wr + wi;
					w2 = wr - wi;

					for (i = k; i < n2; i += di) {
						fx32 xr, xi, yr, yi;
						u32 j = i + div;

						xr = data[j];
						xi = data[j + 1];
						{

							fx32 t = FX_Mul(wr, xr + xi);
							yr = t - FX_Mul(w1, xi);
							yi = t - FX_Mul(w2, xr);
						}

						xr = data[i];
						xi = data[i + 1];
						data[i] = xr + yr;
						data[i + 1] = xi + yi;
						data[j] = xr - yr;
						data[j + 1] = xi - yi;
					}
				} else {
					t += dt;

					for (i = k; i < n2; i += di) {
						fx32 xr, xi, yr, yi;
						u32 j = i + div;

						xr = data[i];
						xi = data[i + 1];
						yr = data[j];
						yi = data[j + 1];
						data[i] = xr - yi;
						data[i + 1] = xi + yr;
						data[j] = xr + yi;
						data[j + 1] = xi - yr;
					}
				}
			}
		}
	}
}

void MATH_FFT (fx32 *data, u32 nShift, const fx16 *sinTable)
{
	u32 i;
	u32 n = 1U << nShift;
	u32 n2 = n * 2;

	MATHi_FFT(data, nShift, sinTable);

	for (i = 0; i < n2; i++) {
		data[i] >>= nShift;
	}
}

void MATH_IFFT (fx32 *data, u32 nShift, const fx16 *sinTable)
{
	MATHi_IFFT(data, nShift, sinTable);
}

void MATH_FFTReal (fx32 *data, u32 nShift, const fx16 *sinTable, const fx16 *sinTable2)
{
	u32 i, j, k;
	u32 n = 1U << nShift;
	u32 nq = n >> 2;

	MATHi_FFT(data, nShift - 1, sinTable2);

	for (k = 1; k < nq; k++) {
		fx32 xr, xi, yr, yi, zr, zi;
		fx32 wr, wi;

		i = k * 2;
		j = n - i;

		wr = sinTable[k + nq];
		wi = sinTable[k];

		{
			fx32 d1r, d1i, d2r, d2i;
			d1r = data[i];
			d1i = data[i + 1];
			d2r = data[j];
			d2i = data[j + 1];
			yr = (d1r + d2r) >> 1;
			yi = (d1i - d2i) >> 1;
			zr = (d1i + d2i) >> 1;
			zi = -(d1r - d2r) >> 1;
		}

		{
			fx32 t = FX_Mul(wr, zr + zi);
			xr = t - FX_Mul(wr + wi, zi);
			xi = t - FX_Mul(wr - wi, zr);
		}

		data[i] = yr + xr;
		data[i + 1] = yi + xi;
		data[j] = yr - xr;
		data[j + 1] = -yi + xi;
	}
	{
		fx32 xr, xi;
		xr = data[0];
		xi = data[1];
		data[0] = xr + xi;
		data[1] = xr - xi;
	}

	for (i = 0; i < n; i++) {
		data[i] >>= nShift;
	}

}

void MATH_IFFTReal (fx32 *data, u32 nShift, const fx16 *sinTable, const fx16 *sinTable2)
{
	u32 i, j, k;
	u32 n = 1U << nShift;
	u32 nq = n >> 2;

	for (k = 1; k < nq; k++) {
		fx32 yr, yi, zr, zi, xr, xi;
		fx32 wr, wi;

		i = k * 2;
		j = n - i;

		wr = sinTable[k + nq];
		wi = -sinTable[k];

		{
			fx32 d1r, d1i, d2r, d2i;
			d1r = data[i];
			d1i = data[i + 1];
			d2r = data[j];
			d2i = data[j + 1];
			yr = (d1r + d2r);
			yi = (d1i - d2i);
			zr = -(d1i + d2i);
			zi = (d1r - d2r);
		}

		{
			fx32 t = FX_Mul(wr, zr + zi);
			xr = t - FX_Mul(wr + wi, zi);
			xi = t - FX_Mul(wr - wi, zr);
		}

		data[i] = yr + xr;
		data[i + 1] = yi + xi;
		data[j] = yr - xr;
		data[j + 1] = -yi + xi;
	}
	{
		fx32 xr, xi;
		xr = data[0];
		xi = data[1];
		data[0] = (xr + xi);
		data[1] = (xr - xi);
		data[n / 2] <<= 1;
		data[n / 2 + 1] <<= 1;
	}

	MATHi_IFFT(data, nShift - 1, sinTable2);
}
