#include "stdlib.h"
#include "string.h"
#include "syscall/syscall.h"
#include "task/signal.h"
#include "mem/malloc.h"
extern void *sys_call(void *);

static void wait_one_tick(void){
    int a = 0; //等一个时间片，让实时中断去检查信号
    for (int i = 0; i < 0xFFFFF; i++)
    {
        a += 1;
        a += 2;
        a += 3;
    }
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

void sleep(uint32_t ms)
{
    syscall_args_t arg;
    arg.id = SYS_sleep;
    arg.arg0 = ms ;
    sys_call(&arg);
    wait_one_tick();
    return;
}

void yield(void)
{
    syscall_args_t arg;
    arg.id = SYS_yield;
    sys_call(&arg);
    return;
}
int fork(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_fork;
    ret = sys_call(&arg);
    return ret;
}
int getpid(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_getpid;
    ret = sys_call(&arg);
    return ret;
}
int getppid(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_getppid;
    ret = sys_call(&arg);
    return ret;
}
int wait(int *status)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_wait;
    arg.arg0 = status;
    ret = sys_call(&arg);
    return ret;
}
void exit(int status)
{
    syscall_args_t arg;
    arg.id = SYS_exit;
    arg.arg0 = status;
    sys_call(&arg);
}
int geterrno(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_geterrno;
    ret = sys_call(&arg);
    return ret;
}
int signal(int signum, void (*signal_handler)(int))
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_signal;
    arg.arg0 = signum;
    arg.arg1 = signal_handler;
    ret = sys_call(&arg);
    return ret;
}
int sigpromask(int how, const sigset_t *set, sigset_t *old)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_sigpromask;
    arg.arg0 = how;
    arg.arg1 = set;
    arg.arg2 = old;
    ret = sys_call(&arg);
    return ret;
}
int raise(int signum)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_raise;
    arg.arg0 = signum;
    ret = sys_call(&arg);
    return ret;
}
int kill(int pid, int signum)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_kill;
    arg.arg0 = pid;
    arg.arg1 = signum;
    ret = sys_call(&arg);
    return ret;
}
int sigpending(sigset_t *set)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_sigpending;
    arg.arg0 = set;
    ret = sys_call(&arg);
    return ret;
}
int pause(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_pause;
    ret = sys_call(&arg);
    wait_one_tick();
    return ret;
}
void* malloc(size_t size){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_malloc;
    arg.arg0 = size;
    ret = sys_call(&arg);
    return ret;
}
void free(void* ptr){
  
    syscall_args_t arg;
    arg.id = SYS_free;
    arg.arg0 = ptr;
    sys_call(&arg);
}
int open(const char *path, int flags, uint32_t mode){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_open;
    arg.arg0 = path;
    arg.arg1 = flags;
    arg.arg2 = mode;
    ret = sys_call(&arg);
    return ret;
}
int read(int fd, char *buf, int len){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_read;
    arg.arg0 = fd;
    arg.arg1 = buf;
    arg.arg2 = len;
    ret = sys_call(&arg);
    return ret;
}
int lseek(int fd, int offset, int whence){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_lseek;
    arg.arg0 = fd;
    arg.arg1 = offset;
    arg.arg2 = whence;
    ret = sys_call(&arg);
    return ret;
}
int close(int fd){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_close;
    arg.arg0 = fd;
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

