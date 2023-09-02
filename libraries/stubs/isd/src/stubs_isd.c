#include <nitro.h>

void ISDPrintf(const char *, ...);
void ISDPrint(const char *);

void _ISDbgLib_Initialize(void);
void _ISDbgLib_AllocateEmualtor(void);
void _ISDbgLib_FreeEmulator(void);
void _ISDbgLib_RegistOverlayInfo(void);
void _ISDbgLib_UnregistOverlayInfo(void);

SDK_WEAK_SYMBOL void ISDPrintf (const char *, ...)
{
}

SDK_WEAK_SYMBOL void ISDPrint (const char *)
{
}

SDK_WEAK_SYMBOL void _ISDbgLib_Initialize (void)
{
}

SDK_WEAK_SYMBOL void _ISDbgLib_AllocateEmualtor (void)
{
}

SDK_WEAK_SYMBOL void _ISDbgLib_FreeEmulator (void)
{
}

SDK_WEAK_SYMBOL void _ISDbgLib_RegistOverlayInfo (void)
{
}

SDK_WEAK_SYMBOL void _ISDbgLib_UnregistOverlayInfo (void)
{
}
