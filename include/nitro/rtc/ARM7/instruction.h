#ifndef NITRO_RTC_ARM7_INSTRUCTION_H_
#define NITRO_RTC_ARM7_INSTRUCTION_H_

#include <nitro/rtc/common/type.h>

#ifdef  __cplusplus
extern "C" {
#endif

void RTC_SetHourFormat(u32 param1);
void RTC_ReadDateTime(RTCRawData *param1);
void RTC_WriteDateTime(RTCRawData *param1);
void RTC_ReadDate(void *param1);
void RTC_ReadTime(void *param1);
void RTC_WriteTime(void *param1);
BOOL RTC_ReadPulse(void *param1);
BOOL RTC_WritePulse(void *param1);
BOOL RTC_ReadAlarm1(void *param1);
BOOL RTC_WriteAlarm1(void *param1);
BOOL RTC_ReadAlarm2(void *param1);
BOOL RTC_WriteAlarm2(void *param1);
void RTC_ReadStatus1(RTCRawStatus1 *param1);
void RTC_WriteStatus1(RTCRawStatus1 *param1);
void RTC_ReadStatus2(RTCRawStatus2 *param1);
void RTC_WriteStatus2(RTCRawStatus2 *param1);
void RTC_ReadAdjust(void *param1);
void RTC_WriteAdjust(void *param1);
void RTC_ReadFree(void *param1);
void RTC_WriteFree(void *param1);

#ifdef  __cplusplus
}
#endif

#endif
