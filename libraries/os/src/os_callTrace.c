#include <nitro/os.h>

#if defined(OS_PROFILE_AVAILABLE) && defined(OS_PROFILE_CALL_TRACE)

#ifdef OS_NO_CALLTRACE

SDK_WEAK_SYMBOL asm void __PROFILE_ENTRY (void){
    bx lr
}

SDK_WEAK_SYMBOL asm void __PROFILE_EXIT (void){
    bx lr
}

void OS_DumpThreadCallTrace (const OSThread *)
{
}
#else

OSCallTraceInfo * OSi_GetCallTraceInfo(void);
void OSi_DumpCurrentLr(u32 lr, int strIndex);
void OSi_DumpOneInfo(OSCallTrace * p);
void OSi_DumpFullInfo(OSCallTraceInfo * info);
void OSi_DumpCallTrace_core(u32 returnAddress);
void OSi_DumpThreadCallTrace_core(const OSThread * thread, u32 returnAddress);
void OSi_Abort_CallTraceFull(u32 name, u32 returnAddress, u32 r0, u32 sp);
void OSi_Abort_CallTraceLess(u32 returnAddress);

static OSCallTraceInfo * OSi_DefaultCallTraceInfo;

#define OSi_STR_DUMPCALLTRACE          0
#define OSi_STR_DUMPTHREADCALLTRACE    1

extern BOOL OSi_IsThreadInitialized;

#include <nitro/code32.h>

asm BOOL OS_EnableCallTrace (void)
{
    stmfd sp !, {lr}
    bl OSi_GetCallTraceInfo
    ldmfd sp !, {lr}
    cmp r0, #0
    bxeq lr
    mov r2, r0
    ldrh r0, [r2, #OSCallTraceInfo.enable]
    mov r1, #1
    strh r1, [r2, #OSCallTraceInfo.enable]
    bx lr
}

#include <nitro/codereset.h>
#include <nitro/code32.h>

asm BOOL OS_DisableCallTrace (void)
{
    stmfd sp !, {lr}
    bl OSi_GetCallTraceInfo
    ldmfd sp !, {lr}
    cmp r0, #0
    bxeq lr
    mov r2, r0
    ldrh r0, [r2, #OSCallTraceInfo.enable]
    mov r1, #0
    strh r1, [r2, #OSCallTraceInfo.enable]
    bx lr
}

#include <nitro/codereset.h>
#include <nitro/code32.h>

asm BOOL OS_RestoreCallTrace (BOOL)
{
    stmfd sp !, {r0, lr}
    bl OSi_GetCallTraceInfo
    mov r1, r0
    ldmfd sp !, {r0, lr}
    cmp r1, #0
    bxeq lr
    ldrh r2, [r1, #OSCallTraceInfo.enable]
    strh r0, [r1, #OSCallTraceInfo.enable]
    mov r0, r2
    bx lr
}

#include <nitro/codereset.h>

#pragma profile off
void OS_InitCallTrace (void * buf, u32 size, OSCallTraceMode mode)
{
    OSCallTraceInfo ** infoPtr;

    SDK_ASSERTMSG(((u32)buf & 3) == 0, "CallTrace buffer must be aligned by 4");
    SDK_ASSERTMSG((size >= OSi_TRACEINFO_SIZEOF_HEADERPART + sizeof(OSCallTrace)),
                  "to small CallTrace buffer");

#ifdef SDK_NO_THREAD
    infoPtr = &OSi_DefaultCallTraceInfo;
#else
    SDK_ASSERTMSG(OS_IsThreadAvailable(), "CallTrace need thread system.");
    infoPtr = (OSCallTraceInfo **)&(OS_GetCurrentThread()->profiler);
#endif

    *infoPtr = (OSCallTraceInfo *)buf;
    (*infoPtr)->current = (*infoPtr)->array;
    (*infoPtr)->limit = (OSCallTrace *)((u32)buf
                                        + OSi_TRACEINFO_SIZEOF_HEADERPART
                                        + OS_CalcCallTraceLines(size) * sizeof(OSCallTrace));
    (*infoPtr)->enable = (u16)TRUE;
    (*infoPtr)->circular = (u16)mode;
#ifdef OS_CALLTRACE_LEVEL_AVAILABLE
    (*infoPtr)->level = 0;
#endif

    if (mode == OS_CALLTRACE_LOG) {
        OS_ClearCallTraceBuffer();
    }
}

#pragma profile reset

int OS_CalcCallTraceLines (u32 size)
{
    int n = (int)((size - OSi_TRACEINFO_SIZEOF_HEADERPART) / sizeof(OSCallTrace));
    return (n < 0) ? 0 : n;
}

u32 OS_CalcCallTraceBufferSize (int lines)
{
    SDK_ASSERT(lines >= 0);
    return OSi_TRACEINFO_SIZEOF_HEADERPART + lines * sizeof(OSCallTrace);
}

#pragma profile off
void OS_ClearCallTraceBuffer (void)
{
    OSCallTraceInfo * info = OSi_GetCallTraceInfo();

    if (info && info->circular) {
        OSCallTrace * p;

        info->current = info->array;
        for (p = info->current; p < info->limit; p++) {
            p->name = (u32)NULL;
        }
    }
}

#pragma profile reset
#pragma profile off

void OSi_SetCallTraceEntry (const char * name, u32 lr)
{
    OSCallTraceInfo * info = OSi_GetCallTraceInfo();

    if (info && info->circular) {
        OSCallTrace * p = info->current;

        p->name = (u32)name;
        p->returnAddress = lr;

#ifdef OS_CALLTRACE_RECORD_R0
        p->r0 = 0;
#endif
#ifdef OS_CALLTRACE_RECORD_R1
        p->r1 = 0;
#endif
#ifdef OS_CALLTRACE_RECORD_R2
        p->r2 = 0;
#endif
#ifdef OS_CALLTRACE_RECORD_R3
        p->r3 = 0;
#endif
        p++;

        if ((u32)p >= (u32)info->limit) {
            p = info->array;
        }

        info->current = p;
    }
}

#pragma profile reset

#include <nitro/code32.h>

asm OSCallTraceInfo * OSi_GetCallTraceInfo (void)
{
#ifdef SDK_NO_THREAD
    ldr r0, = OSi_DefaultCallTraceInfo
    ldr r0, [r0, #0]
    bx lr
#else
    ldr r0, = OSi_ThreadInfo
    ldr r0, [r0, #OSThreadInfo.current]
    cmp r0, #0
    ldrne r0, [r0, #OSThread.profiler]
    bx lr
#endif
}

#include <nitro/codereset.h>
#include <nitro/code32.h>

asm void __PROFILE_ENTRY (void)
{
#ifndef SDK_NO_THREAD
    stmfd sp !, {r0}
    ldr r0, = OSi_IsThreadInitialized
    ldr r0, [r0, #0]
    cmp r0, #0
    ldmfd sp !, {r0}
    bxeq lr
#endif
    stmfd sp !, {r1 - r3}
    stmfd sp !, {r0, lr}
    bl OSi_GetCallTraceInfo
    mov r1, r0
    ldmfd sp !, {r0, lr}
    cmp r1, #0
    beq _exit
    ldrh r2, [r1, #OSCallTraceInfo.enable]
    cmp r2, #0
    beq _exit
    ldr r2, [r1, #OSCallTraceInfo.current]
    cmp r2, #0
    beq _exit
    ldrh r3, [r1, #OSCallTraceInfo.circular]
    cmp r3, #0
    bne _circular_skip_overcheck
#ifdef OS_CALLTRACE_CHECK_OVERSTACK
    ldr r3, [r1, #OSCallTraceInfo.limit]
    cmp r2, r3
    ldrpl r1, [sp, #16]
    ldrpl r2, [sp, #12]
    movpl r3, sp
    bpl OSi_Abort_CallTraceFull
#endif
_circular_skip_overcheck:
    str r0, [r2, #OSCallTrace.name]
    ldr r0, [sp, #16]
    str r0, [r2, #OSCallTrace.returnAddress]
#ifdef OS_CALLTRACE_RECORD_R0
    ldr r0, [sp, #12]
    str r0, [r2, #OSCallTrace.r0]
#endif
#ifdef OS_CALLTRACE_RECORD_R1
    ldr r0, [sp, #0]
    str r0, [r2, #OSCallTrace.r1]
#endif
#ifdef OS_CALLTRACE_RECORD_R2
    ldr r0, [sp, #4]
    str r0, [r2, #OSCallTrace.r2]
#endif
#ifdef OS_CALLTRACE_RECORD_R3
    ldr r0, [sp, #8]
    str r0, [r2, #OSCallTrace.r3]
#endif
#ifdef OS_CALLTRACE_LEVEL_AVAILABLE
    ldr r0, [r1, #OSCallTraceInfo.level]
    str r0, [r2, #OSCallTrace.level]
    add r0, r0, #1
    str r0, [r1, #OSCallTraceInfo.level]
#endif
    add r2, r2, #OSi_SIZEOF_CALLTRACE
    ldrh r3, [r1, #OSCallTraceInfo.circular]
    cmp r3, #0
    beq _notcircular_skip_ring
    ldr r3, [r1, #OSCallTraceInfo.limit]
    cmp r2, r3
    bmi _store_current
    add r2, r1, #OSCallTraceInfo.array
_notcircular_skip_ring:
_store_current:
    str r2, [r1, #OSCallTraceInfo.current]
_exit:
    ldmfd sp !, {r1 - r3}
    bx lr
}

#include <nitro/codereset.h>
#include <nitro/code32.h>

asm void __PROFILE_EXIT (void)
{
#ifdef OS_CALLTRACE_CHECK_OVERSTACK
    stmfd sp !, {r0 - r2, lr}
#else
    stmfd sp !, {r0 - r1, lr}
#endif

#ifndef SDK_NO_THREAD
    ldr r0, = OSi_IsThreadInitialized
    ldr r0, [r0, #0]
    cmp r0, #0
    beq _exit
#endif
    bl OSi_GetCallTraceInfo
    cmp r0, #0
    beq _exit
    ldrh r1, [r0, #OSCallTraceInfo.enable]
    cmp r1, #0
    beq _exit
#ifdef OS_CALLTRACE_LEVEL_AVAILABLE
    ldr r1, [r0, #OSCallTraceInfo.level]
    sub r1, r1, #1
    movmi r1, #0
    str r1, [r0, #OSCallTraceInfo.level]
#endif
    ldrh r1, [r0, #OSCallTraceInfo.circular]
    cmp r1, #0
    bne _exit
    ldr r1, [r0, #OSCallTraceInfo.current]
    sub r1, r1, #OSi_SIZEOF_CALLTRACE
#ifdef OS_CALLTRACE_CHECK_OVERSTACK
    add r2, r0, #OSCallTraceInfo.array
    cmp r1, r2
    ldrmi r0, [sp, #12]
    bmi OSi_Abort_CallTraceLess
#endif
    str r1, [r0, #OSCallTraceInfo.current]
_exit:
#ifdef OS_CALLTRACE_CHECK_OVERSTACK
    ldmfd sp !, {r0 - r2, lr}
#else
    ldmfd sp !, {r0 - r1, lr}
#endif
    bx lr
}

#include <nitro/codereset.h>

const char * OSi_DumpCurrentLr_str[] = {
    "OS_DumpCallTrace", "OS_DumpThreadCallTrace",
};

#pragma profile off

void OSi_DumpCurrentLr (u32 lr, int strIndex)
{
    BOOL e = OS_DisableCallTrace();
    OS_Printf("%s: lr=%08x\n", OSi_DumpCurrentLr_str[strIndex], lr);
    (void)OS_RestoreCallTrace(e);
}

#pragma profile reset
#pragma profile off

#define OSi_CALLTRACE_MAX_INDENT 10

void OSi_DumpOneInfo (OSCallTrace * p)
{
#ifdef OS_CALLTRACE_LEVEL_AVAILABLE
    {
        int n = (int)p->level;
        if (n > OSi_CALLTRACE_MAX_INDENT) {
            n = OSi_CALLTRACE_MAX_INDENT;
        }

        while (n) {
            int space = n;
            if (space >= 10) {
                space = 10;
            }
            OS_Printf("%s", &("          \0"[10 - space]));
            n -= space;
        }
    }
#endif

    OS_Printf("%s: lr=%08x"
#ifdef OS_CALLTRACE_RECORD_R0
              ", r0=%08x"
#endif
#ifdef OS_CALLTRACE_RECORD_R1
              ", r1=%08x"
#endif
#ifdef OS_CALLTRACE_RECORD_R2
              ", r2=%08x"
#endif
#ifdef OS_CALLTRACE_RECORD_R3
              ", r3=%08x"
#endif
              "\n", (char *)(p->name), p->returnAddress
#ifdef OS_CALLTRACE_RECORD_R0
              , p->r0
#endif
#ifdef OS_CALLTRACE_RECORD_R1
              , p->r1
#endif
#ifdef OS_CALLTRACE_RECORD_R2
              , p->r2
#endif
#ifdef OS_CALLTRACE_RECORD_R3
              , p->r3
#endif
              );
}

#pragma profile reset
#pragma profile off

void OSi_DumpFullInfo (OSCallTraceInfo * info)
{
    if (info && info->current) {
        OSCallTrace * p = info->current - 1;

        while (1) {
            if (info->circular && (u32)p < (u32)info->array) {
                p = info->limit - 1;
            }

            if ((u32)p < (u32)info->array
                || 0x2000000 > (u32)(p->name) || 0x2400000 < (u32)(p->name)) {
                break;
            }

            OSi_DumpOneInfo(p);

            if ((u32)p == (u32)(info->current)) {
                break;
            }

            p--;
        }
    }
}

#pragma profile reset

#include <nitro/code32.h>

asm void OS_DumpCallTrace (void)
{
    mov r0, lr
    b OSi_DumpCallTrace_core

}

#include <nitro/codereset.h>

#pragma profile off

void OSi_DumpCallTrace_core (u32 returnAddress)
{
    BOOL e = OS_DisableCallTrace();

    OSCallTraceInfo * info = OSi_GetCallTraceInfo();
    SDK_ASSERTMSG(info && info->current, "Not Initialize functionTrace");

    if (returnAddress) {
        OSi_DumpCurrentLr(returnAddress, OSi_STR_DUMPCALLTRACE);
    }
    OSi_DumpFullInfo(info);

    (void)OS_RestoreCallTrace(e);
}

#pragma profile reset

#include <nitro/code32.h>

asm void OS_DumpThreadCallTrace (const OSThread * thread)
{
    mov r1, lr
    b OSi_DumpThreadCallTrace_core
}

#include <nitro/codereset.h>

void OSi_DumpThreadCallTrace_core (const OSThread * thread, u32 returnAddress)
{
    OSCallTraceInfo * info;
    BOOL e;

#ifdef SDK_NO_THREAD
    return;
#endif

    if (!thread) {
        thread = OS_GetCurrentThread();
    }

    if (thread) {
        info = (OSCallTraceInfo *)thread->profiler;
    }

    if (!thread || !info || !info->current) {
        return;
    }

    if (thread != OS_GetCurrentThread()) {
        returnAddress = thread->context.r[14];
    }

    e = OS_DisableCallTrace();

    if (returnAddress) {
        OSi_DumpCurrentLr(returnAddress, OSi_STR_DUMPTHREADCALLTRACE);
    }
    OSi_DumpFullInfo(info);

    (void)OS_RestoreCallTrace(e);
}

#pragma profile off

void OSi_Abort_CallTraceFull (u32 name, u32 returnAddress, u32 r0, u32 sp)
{
#pragma unused(r0)
    BOOL e = OS_DisableCallTrace();

    OS_Printf("***Error: CallTrace stack overflow");
    OS_Printf(" in %s: lr=%08x"
#ifdef OS_CALLTRACE_RECORD_R0
              ", r0=%08x"
#endif
#ifdef OS_CALLTRACE_RECORD_R1
              ", r1=%08x"
#endif
#ifdef OS_CALLTRACE_RECORD_R2
              ", r2=%08x"
#endif
#ifdef OS_CALLTRACE_RECORD_R3
              ", r3=%08x"
#endif
              "\n", (char *)(name), returnAddress
#ifdef OS_CALLTRACE_RECORD_R0
              , r0
#endif
#ifdef OS_CALLTRACE_RECORD_R1
              , *((u32 *)sp)
#endif
#ifdef OS_CALLTRACE_RECORD_R2
              , *((u32 *)sp + 1)
#endif
#ifdef OS_CALLTRACE_RECORD_R3
              , *((u32 *)sp + 2)
#endif
              );

#ifdef SDK_NO_THREAD
    OSi_DumpCallTrace_core(0);
#else
    OSi_DumpThreadCallTrace_core(NULL, 0);
#endif
    OS_Terminate();
}

#pragma profile reset
#pragma profile off

void OSi_Abort_CallTraceLess (u32 returnAddress)
{
    BOOL e = OS_DisableCallTrace();

    OS_Printf("***Error: CallTrace stack underflow.");
    OS_Printf(" (lr=%08x)\n", returnAddress);

    OS_Terminate();
}

#pragma profile reset

#endif
#endif
