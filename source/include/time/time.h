#ifndef __TIME_H
#define __TIME_H
#include "types.h"
typedef uint32_t time_t;
typedef struct _tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
}tm_t;
// BCD to Binary 转换宏
#define BCD_TO_BIN(val) ((val) = ((val) & 0x0F) + ((val) >> 4) * 10)
#define BIN_TO_BCD(val) ((((val) / 10) << 4) | ((val) % 10))
extern time_t startup_time;
time_t mktime(const tm_t *tm);
int localtime(tm_t *tm, const time_t time);
int strtime(char* buf,int buf_size,const char* format,const tm_t* time);
int sys_get_clocktime(tm_t* time);
time_t sys_time(tm_t *tm);
void time_init(void);


#endif