#ifndef __WLSYS_H_
#define __WLSYS_H_

#include "Platform.h"
#include <nitro_sp.h>

#undef NULL
#define NULL 0

#ifdef __INSYSROM__
#undef DEBUG
#define DEBUG DEBUG_WL
#endif

#include "Global.h"
#include "Revision.h"

#include "Param.h"

/*
#ifdef	SDK_ARM7
#ifdef	__INSYSROM__
#pragma define_section wllib "wllib_code" "wllib_data"
#pragma	section wllib begin
#endif
#endif
*/

void WlessLibReboot(void);

#endif // __WLSYS_H_
