#ifndef	__WLPARAM_H_
#define	__WLPARAM_H_

typedef	struct {
	struct {
		u8			rsv[20+4*3+8*32];
	} taskMan;

	struct {
		u8			rsv[12*16+4*3+4*2];
	} heapMan;

//	struct
//	{
//	} funcTbl;

	u8	params[4];
} WlMan;

#endif	// __WLPARAM_H_


