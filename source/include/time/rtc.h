#ifndef __RTC_H
#define __RTC_H
#include "types.h"
extern void exception_handler_rtc(void);
void set_alarm(uint32_t secs);
void rtc_init(void);
#endif