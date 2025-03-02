#include "time/time.h"
#include "printk.h"
void time_test(void){
    int ret;
    tm_t tm;
    char buf[128] = {0};
    sys_get_clocktime(&tm);
    const char* format = "current time: %Y-%m-%d %H:%M:%S";
    ret = strtime(buf,128,format,&tm);
    dbg_info("ret = %d , str : %s\r\n",ret,buf);
}