#ifndef __SYS_CALL_H
#define __SYS_CALL_H
#include "types.h"
typedef struct _syscall_args_t
{
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;
} syscall_args_t;
typedef struct _syscall_frame_t{
    int eax;
    int edx;
    int ecx;
}syscall_frame_t;
typedef int (*syscall_handler_t)(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3,uint32_t arg4,uint32_t arg5);

#define SYS_test            0
#define SYS_write           1
#define SYS_sleep           2
#define SYS_yield           3
#define SYS_fork            4
#define SYS_getpid          5
#define SYS_getppid         6
#define SYS_wait            7
#define SYS_exit            8
#define SYS_geterrno        9
#define SYS_signal          10
#define SYS_sigpromask      11
#define SYS_raise           12
#define SYS_kill            13
#define SYS_sigpending      14
#define SYS_pause           15
#define SYS_malloc          16
#define SYS_free            17
#define SYS_open            18
#define SYS_read            19
#define SYS_lseek           20
#define SYS_close           21
#define SYS_execve          22

#endif