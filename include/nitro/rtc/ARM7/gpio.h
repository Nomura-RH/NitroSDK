#ifndef NITRO_RTC_ARM7_GPIO_H_
#define NITRO_RTC_ARM7_GPIO_H_

#ifdef  __cplusplus
extern "C" {
#endif

void RTCi_GpioStart(void);
void RTCi_GpioSendCommand(u32 param1, u32 param2);
void RTCi_GpioSendData(void *param1, u32 param2);
void RTCi_GpioReceiveData(void *param1, u32 param2);
void RTCi_GpioEnd(void);

#ifdef  __cplusplus
}
#endif

#endif
