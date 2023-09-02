#include <nitro.h>
#include <nitro/utest.h>

static char assertMessage[UT_ASSERTMESSAGE_MAXLEN] = { '\0' };
static int assertMessageLen = 0;

char * UTi_GetMessage (void)
{
    return assertMessage[0] ? assertMessage : "???";
}

void UTi_ClearMessage (void)
{
    assertMessageLen = 0;
    assertMessage[0] = '\0';

    return;
}

void UTi_SetMessage (const char * filename, int linenum, const char * fmt, ...)
{
    va_list vlist;
    int n;
    int bufferLen;
    char * bufferTop;

    va_start(vlist, fmt);

    bufferLen = (signed)sizeof(assertMessage) - assertMessageLen;
    bufferTop = assertMessage + assertMessageLen;
    n = OS_SNPrintf(bufferTop, (unsigned)bufferLen, "%s:%d ", filename, linenum);
    assertMessageLen += (n > bufferLen) ? bufferLen : n;

    bufferLen = (signed)sizeof(assertMessage) - assertMessageLen;
    bufferTop = assertMessage + assertMessageLen;
    n = OS_VSNPrintf(bufferTop, (unsigned)bufferLen, fmt, vlist);
    assertMessageLen += (n > bufferLen) ? bufferLen : n;

    va_end(vlist);
}

void UTi_SetMessageWithFloat (const char * filename, int linenum, const char * fmt, ...)
{
    va_list vlist;
    int n;
    int bufferLen;
    char * bufferTop;

    va_start(vlist, fmt);

    bufferLen = (signed)sizeof(assertMessage) - assertMessageLen;
    bufferTop = assertMessage + assertMessageLen;
    n = OS_SNPrintf(bufferTop, (unsigned)bufferLen, "%s:%d ", filename, linenum);
    assertMessageLen += (n > bufferLen) ? bufferLen : n;

    bufferLen = (signed)sizeof(assertMessage) - assertMessageLen;
    bufferTop = assertMessage + assertMessageLen;
    n = vsnprintf(bufferTop, (unsigned)bufferLen, fmt, vlist);
    assertMessageLen += (n > bufferLen) ? bufferLen : n;

    va_end(vlist);
}

SDK_WEAK_SYMBOL void UTi_Printf (const char * fmt, ...)
{
#ifdef SDK_FINALROM
#pragma unused(fmt)
#else
    va_list vlist;

    va_start(vlist, fmt);
    OS_TVPrintf(fmt, vlist);
    va_end(vlist);
#endif
}

SDK_WEAK_SYMBOL void UTi_PrintfWithFloat (const char * fmt, ...)
{
#ifdef SDK_FINALROM
#pragma unused(fmt)
#else
    va_list vlist;

    va_start(vlist, fmt);
    OS_VPrintf(fmt, vlist);
    va_end(vlist);
#endif
}

SDK_WEAK_SYMBOL void UTi_PutString (const char * str)
{
#ifdef SDK_FINALROM
#pragma unused(str)
#else
    OS_PutString(str);
#endif
}
