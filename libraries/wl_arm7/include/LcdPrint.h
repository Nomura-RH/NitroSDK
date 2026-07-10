#ifndef __LCD_PRINT_H_
#define __LCD_PRINT_H_

#include "Font.h"

#define LCD_RAM_STR_ADRS LCD_SHARE_MEM_ADRS
#define LCD_RAM_SIZE     LCD_SHARE_MEM_SIZE

#define LCD_BG1_SCR_ADRS (LCD_RAM_STR_ADRS)
#define LCD_BG1_VOFS     (LCD_RAM_STR_ADRS + 0x1000)

#define reg_serlcd_bnry *(vu16 *)(LCD_BG1_SCR_ADRS + 0)
#define reg_serlcd_curr *(vu16 *)(LCD_BG1_SCR_ADRS + 32)
#define reg_serlcd_buf  ((vu16 *)(LCD_BG1_SCR_ADRS + 32 + 32))

#define SERLCD_BUFSIZE   0x2000
#define SERLCD_BNRY_MASK (SERLCD_BUFSIZE / 2 - 1)
#define SERLCD_CURR_MASK (SERLCD_BUFSIZE / 2 - 1)

void InitializeLcdPrint(void);
void LcdBg1PutChar(u8 pltt, int ch);
void LcdBg1Puts(u8 pltt, char *pStr);
void LcdBg1Print(u8 pltt, const char *fmt, ...);

#define BG_SC_PLTT_SHIFT 12

#endif //__LCD_PRINT_H_
