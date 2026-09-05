#ifndef NITRO_RTC_ARM7_INSTRUCTION_H_
#define NITRO_RTC_ARM7_INSTRUCTION_H_

#include <nitro/rtc/common/type.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_INSTRUCTION_READ   0x86
#define RTC_INSTRUCTION_WRITE  0x06
#define RTC_INSTRUCTION_STAT1  0x00
#define RTC_INSTRUCTION_STAT2  0x40
#define RTC_INSTRUCTION_FULL   0x20
#define RTC_INSTRUCTION_TIME   0x60
#define RTC_INSTRUCTION_ALARM1 0x10
#define RTC_INSTRUCTION_ALARM2 0x50
#define RTC_INSTRUCTION_PULSE  0x10
#define RTC_INSTRUCTION_ADJUST 0x30
#define RTC_INSTRUCTION_FREE   0x70

void RTC_Reset(void);
void RTC_SetHourFormat(u16 format);
void RTC_ReadDateTime(RTCRawData *data);
void RTC_WriteDateTime(const RTCRawData *data);
void RTC_ReadDate(RTCRawDate *date);
void RTC_ReadTime(RTCRawTime *time);
void RTC_WriteTime(const RTCRawTime *time);
BOOL RTC_ReadPulse(RTCRawPulse *pulse);
BOOL RTC_WritePulse(const RTCRawPulse *pulse);
BOOL RTC_ReadAlarm1(RTCRawAlarm *alarm);
BOOL RTC_WriteAlarm1(const RTCRawAlarm *alarm);
BOOL RTC_ReadAlarm2(RTCRawAlarm *alarm);
BOOL RTC_WriteAlarm2(const RTCRawAlarm *alarm);
void RTC_ReadStatus1(RTCRawStatus1 *stat);
void RTC_WriteStatus1(const RTCRawStatus1 *stat);
void RTC_ReadStatus2(RTCRawStatus2 *stat);
void RTC_WriteStatus2(const RTCRawStatus2 *stat);
void RTC_ReadAdjust(RTCRawAdjust *adjust);
void RTC_WriteAdjust(const RTCRawAdjust *adjust);
void RTC_ReadFree(RTCRawFree *free);
void RTC_WriteFree(const RTCRawFree *free);

#ifdef __cplusplus
}
#endif

#endif
