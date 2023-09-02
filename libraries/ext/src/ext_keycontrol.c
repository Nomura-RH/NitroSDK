#include <nitro/types.h>
#include <nitro/ext.h>

void EXT_AutoKeys (const EXTKeys *sequence, u16 *cont, u16 *trig)
{
	static u16 absolute_cnt = 0;
	static u16 last_key = 0;
	u16 cnt;

	cnt = absolute_cnt;
	while (cnt >= sequence->count) {
		if (sequence->count == 0) {
			return;
		}
		cnt -= sequence->count;
		sequence++;
	}

	*cont |= sequence->key;
	*trig = (u16)(~last_key & *cont);
	last_key = *cont;
	absolute_cnt++;
}