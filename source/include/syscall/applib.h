#ifndef __APP_LIB_H
#define __APP_LIB_H

#include "types.h"
#include "time/time.h"
#include "stdarg.h"
#include "task/signal.h"
typedef int pid_t;
#define stdin   0
#define stdout  1
#define stderr  2

enum
{
    EOK = 0,           // 没错
    EPERM = 1,         // 操作没有许可
    ENOENT = 2,        // 文件或目录不存在
    ESRCH = 3,         // 指定的进程不存在
    EINTR = 4,         // 中断的函数调用
    EIO = 5,           // 输入输出错误
    ENXIO = 6,         // 指定设备或地址不存在
    E2BIG = 7,         // 参数列表太长
    ENOEXEC = 8,       // 执行程序格式错误
    EBADF = 9,         // 文件描述符错误
    ECHILD = 10,       // 子进程不存在
    EAGAIN = 11,       // 资源暂时不可用
    ENOMEM = 12,       // 内存不足
    EACCES = 13,       // 没有许可权限
    EFAULT = 14,       // 地址错
    ENOTBLK = 15,      // 不是块设备文件
    EBUSY = 16,        // 资源正忙
    EEXIST = 17,       // 文件已存在
    EXDEV = 18,        // 非法连接
    ENODEV = 19,       // 设备不存在
    ENOTDIR = 20,      // 不是目录文件
    EISDIR = 21,       // 是目录文件
    EINVAL = 22,       // 参数无效
    ENFILE = 23,       // 系统打开文件数太多
    EMFILE = 24,       // 打开文件数太多
    ENOTTY = 25,       // 不恰当的 IO 控制操作(没有 tty 终端)
    ETXTBSY = 26,      // 不再使用
    EFBIG = 27,        // 文件太大
    ENOSPC = 28,       // 设备已满（设备已经没有空间）
    ESPIPE = 29,       // 无效的文件指针重定位
    EROFS = 30,        // 文件系统只读
    EMLINK = 31,       // 连接太多
    EPIPE = 32,        // 管道错
    EDOM = 33,         // 域(domain)出错
    ERANGE = 34,       // 结果太大
    EDEADLK = 35,      // 避免资源死锁
    ENAMETOOLONG = 36, // 文件名太长
    ENOLCK = 37,       // 没有锁定可用
    ENOSYS = 38,       // 功能还没有实现
    ENOTEMPTY = 39,    // 目录不空
    ETIMEOUT = 62,        // 超时
    EFSUNK,            // 文件系统未知
    EMSGSIZE = 90,     // 消息过长
    ERROR = 99,        // 一般错误
    EEOF,              // 读写文件结束

    // 网络错误
    EADDR,     // 地址错误
    EPROTO,    // 协议错误
    EOPTION,   // 选项错误
    EFRAG,     // 分片错误
    ESOCKET,   // 套接字错误
    EOCCUPIED, // 被占用
    ENOTCONN,  // 没有连接
    ERESET,    // 连接重置
    ECHKSUM,   // 校验和错误

    // 错误数量，应该在枚举的最后
    ENUM,
};


// 系统调用函数
#define WEXITSTATUS(status) ((status) & 0xFF)

int write(int fd, const char *buf, size_t len);
void sleep(uint32_t ms);
void yield(void);
int fork(void);
int getpid(void);
int getppid(void);
int wait(int* status);
void exit(int status);
int geterrno(void);
int signal(int signum,void (*signal_handler)(int));
int sigpromask(int how,const sigset_t *set, sigset_t *old);
int raise(int signum);
int kill(int pid, int signum);
int sigpending(sigset_t* set);
int pause(void);
void* malloc(size_t size);
void free(void* ptr);
static inline void sigreturn(void)
{
    asm volatile(
        "int $0x40");
}

#define errno (geterrno())
// printf 相关
void printf(char *fmt, ...);

// 字符串相关
void tmp_strcpy(char *dest, const char *src);
void tmp_strncpy(char *dest, const char *src, int size);
int tmp_strncmp(const char *s1, const char *s2, int size);
int tmp_strlen(const char *str);
char *tmp_strchr(const char *str, int c);
char *tmp_strtok(char *str, const char *delim);

// 内存操作
void tmp_memcpy(void *dest, const void *src, int size);
void tmp_memmove(void *dest, const void *src, int size);
void tmp_memset(void *dest, uint8_t v, int size);
int tmp_memcmp(const void *d1, const void *d2, int size);

// 数字转换
void tmp_itoa(char *buf, uint32_t num, int base);
void tmp_itoa_with_padding(char *buf, uint32_t num, int base, int width, int zero_padding);

// 格式化输出
int tmp_sprintf(char *buffer, const char *fmt, ...);
int tmp_snprintf(char *buffer, int buf_size, const char *fmt, ...);
int tmp_vsprintf(char *buffer, const char *fmt, va_list args);
int tmp_vsnprintf(char *buffer, int buf_len, const char *fmt, va_list args);



void wait_one_tick(void);
#endif