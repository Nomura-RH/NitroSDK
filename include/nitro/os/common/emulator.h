#ifndef NITRO_OS_EMULATOR_H_
#define NITRO_OS_EMULATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OSi_CONSOLE_NOT_DETECT    0xffffffff

#define OS_CONSOLE_MASK           0xf0000000
#define OS_CONSOLE_ENSATA         0x10000000
#define OS_CONSOLE_ISEMULATOR     0x20000000
#define OS_CONSOLE_ISDEBUGGER     0x40000000
#define OS_CONSOLE_NITRO          0x80000000

#define OS_CONSOLE_DEV_MASK       0x03000000
#define OS_CONSOLE_DEV_CARTRIDGE  0x01000000
#define OS_CONSOLE_DEV_CARD       0x02000000

#define OS_CONSOLE_SIZE_MASK      0x00000003
#define OS_CONSOLE_SIZE_4MB       0x00000001
#define OS_CONSOLE_SIZE_8MB       0x00000002

BOOL OS_IsRunOnEmulator(void);

u32 OS_GetConsoleType(void);

#ifdef __cplusplus
}
#endif

#endif
