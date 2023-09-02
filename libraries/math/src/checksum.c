#include <nitro/math/checksum.h>

static inline u32 MATHi_Checksum16Update (const void *input, u32 length)
{
	u32 sum;
	u32 n;
	BOOL fSwap = FALSE;

	sum = 0;

	if (((u32)input) & 1) {
		sum += (*((u8 *)input)++ << 8);
		length--;
		fSwap = TRUE;
	}

	while ((length >> 17) > 0) {
		length -= (1 << 17);
		for (n = (1 << 16); n > 0; n--) {
			sum += *((u16 *)input)++;
		}

		sum = (sum >> 16) + (sum & 0xffff);
		sum = (u16)((sum >> 16) + sum);
	}

	for (n = (length >> 1); n > 0; n--) {
		sum += *((u16 *)input)++;
	}

	if (length & 1) {
		sum += *((u8 *)input);
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum = (sum >> 16) + (sum & 0xffff);

	if (fSwap) {
		sum = ((sum << 24) | (sum << 8)) >> 16;
	}

	return sum;
}

void MATH_Checksum16Update (MATHChecksum16Context *context, const void *input, u32 length)
{
	u32 sum;

	sum = MATHi_Checksum16Update(input, length);

	sum += *context;
	sum += (sum >> 16);

	*context = (u16)sum;
}

u16 MATH_CalcChecksum16 (const void *data, u32 dataLength)
{
	u32 sum;

	sum = MATHi_Checksum16Update(data, dataLength);
	return (u16)(~sum);
}

static inline u32 MATHi_Checksum8Update (const void *input, u32 length)
{
	u32 sum;
	u32 n;

	sum = 0;

	if (((u32)input) & 1) {
		sum += *((u8 *)input)++;
		length--;
	}

	while ((length >> 17) > 0) {
		length -= (1 << 17);
		for (n = (1 << 16); n > 0; n--) {
			sum += *((u16 *)input)++;
		}

		sum = (sum >> 16) + (sum & 0xffff);
		sum = (u16)(sum + (sum >> 16));
	}

	for (n = (length >> 1); n > 0; n--) {
		sum += *((u16 *)input)++;
	}

	if (length & 1) {
		sum += *((u8 *)input);
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return (u16)sum;
}

void MATH_Checksum8Update (MATHChecksum8Context *context, const void *input, u32 length)
{
	u32 sum;

	sum = MATHi_Checksum8Update(input, length);

	sum += *context;
	sum += (sum >> 16);

	*context = (u16)sum;
}

u8 MATH_CalcChecksum8 (const void *data, u32 dataLength)
{
	u32 sum;

	sum = MATHi_Checksum8Update(data, dataLength);

	sum = (sum >> 8) + (sum & 0xff);
	sum += (sum >> 8);

	return (u8)(~sum);
}
