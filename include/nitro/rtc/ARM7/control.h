#ifndef NITRO_RTC_ARM7_CONTROL_H_
#define NITRO_RTC_ARM7_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_MESSAGE_ARRAY_MAX  4
#define RTC_THREAD_STACK_SIZE  256
#define RTC_POLLING_STACK_SIZE 256
#define RTC_POLLING_SPAN_TICK  5000

typedef struct RTCWork {
    OSMessageQueue msgQueue;
    OSMessage msgArray[RTC_MESSAGE_ARRAY_MAX];
    OSThread thread;
    u64 stack[RTC_THREAD_STACK_SIZE / sizeof(u64)];
    BOOL busy;
    u16 command;
    OSThread polling;
    OSThreadQueue pollingQueue;
#ifndef SDK_THREAD_INFINITY
#if (OS_THREAD_MAX_NUM <= 16)
    u8 reserved[2];
#endif
#endif
    u64 pollingStack[RTC_POLLING_STACK_SIZE / sizeof(u64)];
    OSAlarm pollingAlarm;
} RTCWork;

void RTC_Init(u32 priority);

#ifdef __cplusplus
}
#endif

#endif
