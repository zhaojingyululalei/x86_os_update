#include "_stdlib.h"
#include "string.h"
#include "syscall/syscall.h"
extern void *sys_call(void *);

int calc_add(int a, int b, int c, int d, int e)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_test;
    arg.arg0 = a;
    arg.arg1 = b;
    arg.arg2 = c;
    arg.arg3 = d;
    arg.arg4 = e;
    ret = sys_call(&arg);
    return ret;
}
int write(int fd,const char* buf,size_t len){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_write;
    arg.arg0 = fd;
    arg.arg1 = buf;
    arg.arg2 = len;
    ret = sys_call(&arg);
    return ret;
}


void printf(char *fmt, ...)
{
#define PRINT_MAX_STR_BUF_SIZE 512
    static char str_buf[PRINT_MAX_STR_BUF_SIZE];
    va_list args;
    int offset = 0;
    // 清空缓冲区
    memset(str_buf, '\0', sizeof(str_buf));

    // 格式化日志信息
    va_start(args, fmt);
    vsprintf(str_buf + offset, fmt, args);
    va_end(args);
    //printf_inner(str_buf);
    write(stdout,str_buf,strlen(str_buf));
}