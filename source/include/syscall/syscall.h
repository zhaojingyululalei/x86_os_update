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

#endif