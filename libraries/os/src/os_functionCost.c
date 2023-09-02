#include <nitro/os.h>

#if defined(OS_PROFILE_AVAILABLE) && defined(OS_PROFILE_FUNCTION_COST)

#ifdef OS_NO_FUNCTIONCOST
SDK_WEAK_SYMBOL asm void __PROFILE_ENTRY (void){
    bx lr
}

SDK_WEAK_SYMBOL asm void __PROFILE_EXIT (void){
    bx lr
}
#else

extern u32 OSi_GetTick_noProfile(void);
extern void OSi_SetSystemCallbackInSwitchThread(void * callback);

extern BOOL OSi_IsThreadInitialized;

static OSFunctionCostInfo * OSi_GetFunctionCostInfo(void);
static void OSi_SetStatistics(void * statBuf, OSFunctionCost * entry, OSFunctionCost * exit);
static void OSi_CalcThreadInterval(register OSThread * saveThread, register OSThread * loadThread);

static OSFunctionCostInfo * OSi_DefaultFunctionCostInfo;

#define OSi_FUNCTIONCOST_CHECKNUM_BUFFER    0xfddb597d

#include <nitro/code32.h>

asm BOOL OS_EnableFunctionCost (void)
{
    stmfd sp !, {lr}
    bl OSi_GetFunctionCostInfo
    ldmfd sp !, {lr}
    cmp r0, #0
    bxeq lr
    mov r2, r0
    ldrh r0, [r2, #OSFunctionCostInfo.enable]
    mov r1, #1
    strh r1, [r2, #OSFunctionCostInfo.enable]
    bx lr
}

#include <nitro/codereset.h>
#include <nitro/code32.h>

asm BOOL OS_DisableFunctionCost (void)
{
    stmfd sp !, {lr}
    bl OSi_GetFunctionCostInfo
    ldmfd sp !, {lr}
    cmp r0, #0
    bxeq lr
    mov r2, r0
    ldrh r0, [r2, #OSFunctionCostInfo.enable]
    mov r1, #0
    strh r1, [r2, #OSFunctionCostInfo.enable]
    bx lr
}

#include <nitro/codereset.h>
#include <nitro/code32.h>

asm BOOL OS_RestoreFunctionCost (BOOL)
{
    stmfd sp !, {r0, lr}
    bl OSi_GetFunctionCostInfo
    mov r1, r0
    ldmfd sp !, {r0, lr}
    cmp r1, #0
    bxeq lr
    ldrh r2, [r1, #OSFunctionCostInfo.enable]
    strh r0, [r1, #OSFunctionCostInfo.enable]
    mov r0, r2
    bx lr
}

#include <nitro/codereset.h>

#pragma profile off

void OS_InitFunctionCost (void * buf, u32 size)
{
    OSFunctionCostInfo ** infoPtr;

    SDK_ASSERTMSG(((u32)buf & 3) == 0, "FunctionCost buffer must be aligned by 4");
    SDK_ASSERTMSG((size >= OSi_COSTINFO_SIZEOF_HEADERPART + sizeof(OSFunctionCost)),
                  "to small FunctionCost buffer");

    if (!OS_IsTickAvailable()) {
        OS_Panic("OS_InitFunctionCost: need tick.\n");
    }

#ifdef SDK_NO_THREAD
    infoPtr = &OSi_DefaultFunctionCostInfo;
#else
    SDK_ASSERTMSG(OS_IsThreadAvailable(), "OS_InitFunctionCost: thread system not initialized.");
    infoPtr = (OSFunctionCostInfo **)&(OS_GetCurrentThread()->profiler);
    OSi_SetSystemCallbackInSwitchThread(OSi_CalcThreadInterval);
#endif
    *infoPtr = (OSFunctionCostInfo *)buf;
    (*infoPtr)->limit = (OSFunctionCost *)((u32)buf
                                           + OSi_COSTINFO_SIZEOF_HEADERPART
                                           + OS_CalcFunctionCostLines(size) * sizeof(OSFunctionCost));

    (*infoPtr)->enable = (u16)TRUE;

#ifdef OSi_FUNCTIONCOST_THREAD_INTERVAL
    (*infoPtr)->breakTime = 0;
#endif
    OSi_ClearThreadFunctionCostBuffer(NULL);
}

#pragma profile reset

int OS_CalcFunctionCostLines (u32 size)
{
    int n = (int)((size - OSi_COSTINFO_SIZEOF_HEADERPART) / sizeof(OSFunctionCost));
    return (n < 0) ? 0 : n;
}

u32 OS_CalcFunctionCostBufferSize (int lines)
{
    SDK_ASSERT(lines >= 0);
    return OSi_COSTINFO_SIZEOF_HEADERPART + lines * sizeof(OSFunctionCost);
}

#pragma profile off

void OSi_ClearThreadFunctionCostBuffer (OSThread * thread)
{
    OSFunctionCostInfo * infoPtr;

    if (!thread) {
        infoPtr = OSi_GetFunctionCostInfo();
    } else {
        infoPtr = (OSFunctionCostInfo *)(thread->profiler);
    }

    if (infoPtr) {
        infoPtr->current = infoPtr->array;
        *(u32 *)(infoPtr->limit - 1) = OSi_FUNCTIONCOST_CHECKNUM_BUFFER;
    }
}

#pragma profile reset

#include <nitro/code32.h>

static asm OSFunctionCostInfo * OSi_GetFunctionCostInfo (void)
{
#ifdef SDK_NO_THREAD
    ldr r0, = OSi_DefaultFunctionCostInfo
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

#ifndef OS_NO_FUNCTIONCOST
    #pragma profile off

    BOOL OS_CheckFunctionCostBuffer (void * buf)
    {
        OSFunctionCostInfo * infoPtr = buf;

        if (*(u32 *)(infoPtr->limit - 1) != OSi_FUNCTIONCOST_CHECKNUM_BUFFER) {
            return FALSE;
        }

        return TRUE;
    }

    #pragma profile reset
#endif

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
    stmfd sp !, {r1 - r4, lr}
    mrs r4, cpsr
    orr r1, r4, #HW_PSR_IRQ_DISABLE
    msr cpsr_c, r1
    stmfd sp !, {r0}
    bl OSi_GetFunctionCostInfo
    mov r1, r0
    ldmfd sp !, {r0}
    cmp r1, #0
    beq _exit
    ldrh r2, [r1, #OSFunctionCostInfo.enable]
    cmp r2, #0
    beq _exit
    ldr r2, [r1, #OSFunctionCostInfo.current]
    cmp r2, #0
    beq _exit
    ldr r3, [r1, #OSFunctionCostInfo.limit]
    cmp r2, r3
    bpl _exit
    str r0, [r2, #OSFunctionCost.entry.name]
    stmfd sp !, {r1 - r2}
    bl OSi_GetTick_noProfile
    ldmfd sp !, {r1 - r2}
    str r0, [r2, #OSFunctionCost.entry.time]
#ifdef OSi_FUNCTIONCOST_THREAD_INTERVAL
    mov r3, #0
    str r3, [r2, #OSFunctionCost.exit.interval]
#endif
    add r2, r2, #OSi_SIZEOF_FUNCTIONCOST
    str r2, [r1, #OSFunctionCostInfo.current]
_exit:
    msr cpsr_c, r4
    ldmfd sp !, {r1 - r4, lr}
    bx lr
}

#include <nitro/codereset.h>
#include <nitro/code32.h>

asm void __PROFILE_EXIT (void)
{
    stmfd sp !, {r0 - r3, lr}
#ifndef SDK_NO_THREAD
    ldr r0, = OSi_IsThreadInitialized
    ldr r0, [r0, #0]
    cmp r0, #0
    beq _exit
#endif
    mrs r3, cpsr
    orr r1, r3, #HW_PSR_IRQ_DISABLE
    msr cpsr_c, r1
    bl OSi_GetFunctionCostInfo
    mov r1, r0
    cmp r1, #0
    beq _exit_ri
    ldrh r2, [r1, #OSFunctionCostInfo.enable]
    cmp r2, #0
    beq _exit_ri
    ldr r2, [r1, #OSFunctionCostInfo.current]
    cmp r2, #0
    beq _exit_ri
    ldr r0, [r1, #OSFunctionCostInfo.limit]
    cmp r2, r0
    bpl _exit_ri
    ldr r0, = OSi_FUNCTIONCOST_EXIT_TAG
    str r0, [r2, #OSFunctionCost.exit.tag]
    stmfd sp !, {r1 - r3}
    bl OSi_GetTick_noProfile
    ldmfd sp !, {r1 - r3}
    str r0, [r2, #OSFunctionCost.exit.time]
    add r2, r2, #OSi_SIZEOF_FUNCTIONCOST
    str r2, [r1, #OSFunctionCostInfo.current]
_exit_ri:
    msr cpsr_c, r3
_exit:
    ldmfd sp !, {r0 - r3, lr}
    bx lr
}

#include <nitro/codereset.h>

#pragma profile off

void OS_InitStatistics (void * statBuf, u32 size)
{
    OSFunctionCostStatisticsInfo * infoPtr = statBuf;
    u32 * p;

    if (!infoPtr || size <= OSi_STATISTICS_LEAST_SIZE) {
        return;
    }

    infoPtr->size = ((size - OSi_STATISTICS_SIZEOF_HEADERPART) / sizeof(OSFunctionCostStatistics))
                    * sizeof(OSFunctionCostStatistics) + OSi_STATISTICS_SIZEOF_HEADERPART;
    infoPtr->limit = (OSFunctionCostStatistics *)((u32)infoPtr + infoPtr->size);

    p = (u32 *)infoPtr->array;
    while ((u32)p < (u32)infoPtr->limit) {
        *p++ = 0;
    }

    *(u32 *)((OSFunctionCostStatistics *)p - 1) = OSi_FUNCTIONCOST_CHECKNUM_BUFFER;
}

#pragma profile reset
#pragma profile off

static void OSi_SetStatistics (void * statBuf, OSFunctionCost * entry, OSFunctionCost * exit)
{
    OSFunctionCostStatisticsInfo * infoPtr = statBuf;
    OSFunctionCostStatistics * p = infoPtr->array;
    u32 time;

    time = exit->exit.time - entry->entry.time;

#ifdef OSi_FUNCTIONCOST_THREAD_INTERVAL
    time = (time > entry->entry.interval) ? (u32)(time - entry->entry.interval) : 0;
#endif

    while ((u32)p < (u32)infoPtr->limit) {
        if (!p->name) {
            p->name = entry->entry.name;
            p->count = 1;
            p->time = time;
            break;
        }

        if (p->name == entry->entry.name) {
            p->count++;
            p->time += time;
            break;
        }

        p++;
    }
}

#pragma profile reset
#pragma profile off

void OS_CalcThreadStatistics (void * statBuf, OSThread * thread)
{
    OSFunctionCostInfo * infoPtr;
    OSFunctionCost * p, * array, * limit;

    if (!thread) {
        infoPtr = OSi_GetFunctionCostInfo();
    } else {
        infoPtr = (OSFunctionCostInfo *)(thread->profiler);
    }

    if (!infoPtr || !(limit = infoPtr->current)) {
        return;
    }

    p = array = (OSFunctionCost *)&(infoPtr->array);
    while (p < limit) {
        u32 name = p->entry.name;

        if (name == OSi_FUNCTIONCOST_EXIT_TAG) {
            int cnt = 0;
            OSFunctionCost * bp = p - 1;

            while (bp >= array) {
                if (bp->entry.name == OSi_FUNCTIONCOST_EXIT_TAG) {
                    cnt++;
                } else {
                    if (--cnt < 0) {
                        OSi_SetStatistics(statBuf, bp, p);
                        break;
                    }
                }
                bp--;
            }
        } else {
            if (!name)
                continue;
        }
        p++;
    }

    OSi_ClearThreadFunctionCostBuffer(thread);
}

#pragma profile reset

#ifndef OS_NO_FUNCTIONCOST
    #pragma profile off

    BOOL OS_CheckStatisticsBuffer (void * statBuf)
    {
        OSFunctionCostStatisticsInfo * infoPtr = statBuf;

        if (*(u32 *)(infoPtr->limit - 1) != OSi_FUNCTIONCOST_CHECKNUM_BUFFER) {
            return FALSE;
        }

        return TRUE;
    }

    #pragma profile reset
#endif

#pragma profile off

void OS_DumpStatistics (void * statBuf)
{
    OSFunctionCostStatisticsInfo * infoPtr = statBuf;
    OSFunctionCostStatistics * p = infoPtr->array;
    BOOL displayed = FALSE;

#ifdef SDK_NO_THREAD
    return;
#endif

    OS_Printf("---- functionCost statistics\n");

    while ((u32)p < (u32)infoPtr->limit) {
        if (p->name && (p->name != OSi_FUNCTIONCOST_CHECKNUM_BUFFER)) {
            displayed = TRUE;
            OS_Printf("%s: count %d, cost %lld\n", p->name, p->count, p->time);
        }

        p++;
    }

    if (!displayed) {
        OS_Printf("no data\n");
    }
}

#pragma profile reset

#include <nitro/code32.h>

#pragma profile off

static asm void OSi_CalcThreadInterval (register OSThread * saveThread, register OSThread * loadThread)
{
#pragma unused(saveThread, loadThread)
#ifndef SDK_NO_THREAD
#ifdef OSi_FUNCTIONCOST_THREAD_INTERVAL
    stmfd sp !, {lr}
    cmp r0, #0
    beq _skip_saveThread
    ldr r2, [r0, #OSThread.profiler]
    cmp r2, #0
    beq _skip_saveThread
    ldrh r3, [r2, #OSFunctionCostInfo.enable]
    cmp r3, #0
    beq _skip_saveThread
    stmfd sp !, {r1 - r3}
    bl OSi_GetTick_noProfile
    ldmfd sp !, {r1 - r3}
    str r0, [r2, #OSFunctionCostInfo.breakTime]
_skip_saveThread:
    cmp r1, #0
    beq _skip_loadThread
    ldr r2, [r1, #OSThread.profiler]
    cmp r2, #0
    beq _skip_loadThread
    ldrh r3, [r2, #OSFunctionCostInfo.enable]
    cmp r3, #0
    beq _skip_loadThread
    ldr r3, [r2, #OSFunctionCostInfo.current]
    cmp r3, #0
    beq _skip_loadThread
    sub r1, r3, #OSi_SIZEOF_FUNCTIONCOST
    add r0, r2, #OSFunctionCostInfo.array[0]
    cmp r1, r0
    bmi _skip_loadThread
    stmfd sp !, {r1 - r3}
    bl OSi_GetTick_noProfile
    ldmfd sp !, {r1 - r3}
    ldr r3, [r2, #OSFunctionCostInfo.breakTime]
    sub r3, r0, r3
    ldr r0, [r1, #OSFunctionCost.entry.interval]
    add r0, r0, r3
    str r0, [r1, #OSFunctionCost.entry.interval]
_skip_loadThread:
    ldmfd sp !, {lr}
#endif
    bx lr
#endif
}

#pragma profile reset
#include <nitro/codereset.h>

#endif
#endif
