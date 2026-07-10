#ifndef __FONT_H_
#define __FONT_H_

#define COLOR_BLACK    0x0000
#define COLOR_BLUE     0x7c00
#define COLOR_GREEN    0x03e0
#define COLOR_CYAN     0xffe0
#define COLOR_RED      0x001f
#define COLOR_MAGENTA  0x7c1f
#define COLOR_YELLOW   0x83ff
#define COLOR_WHITE    0xffff
#define COLOR_GLAY     0x5ad6
#define COLOR_DBLUE    0x2c23
#define COLOR_DGREEN   0xa200
#define COLOR_DCYAN    0x6e80
#define COLOR_DRED     0x1018
#define COLOR_DMAGENTA 0x3018
#define COLOR_DYELLOW  0x03bf
#define COLOR_ORANGE   0x57ff

#define PLT_WHITE_BLACK  0
#define PLT_BLUE_YELLOW  1
#define PLT_RED_BLACK    2
#define PLT_GREEN_BLACK  3
#define PLT_BLACK_RED    4
#define PLT_YELLOW_BLACK 5
#define PLT_GRAY_BLACK   6
#define PLT_BLUE_BLACK   7
#define PLT_BLACK_WHITE  8
#define PLT_BLACK_GREEN  9

void InitializeFont(void);
void LoadFontForSubLcd(void);

#endif //__FONT_H_
