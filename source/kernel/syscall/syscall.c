#include "syscall/syscall.h"
#include "kernel_test.h"
#include "printk.h"
#include "task/task.h"
#include "task/sche.h"
#include "irq/irq.h"
#include "fs/fs.h"
#include "cpu_cfg.h"
#include "task/signal.h"
#include "mem/malloc.h"
#include "fs/fs.h"
// 系统调用表
static const syscall_handler_t sys_table[] = {
    [SYS_test] = (syscall_handler_t)sys_calc_add,
    [SYS_write] = (syscall_handler_t)sys_write,
    [SYS_sleep] = (syscall_handler_t)sys_sleep,
    [SYS_yield] = (syscall_handler_t)sys_yield,
    [SYS_fork] = (syscall_handler_t)sys_fork,
    [SYS_getpid] = (syscall_handler_t)sys_getpid,
    [SYS_getppid] =(syscall_handler_t)sys_getppid,
    [SYS_wait] = (syscall_handler_t)sys_wait,
    [SYS_exit] = (syscall_handler_t)sys_exit,
    [SYS_geterrno]=(syscall_handler_t)task_get_errno,
    [SYS_signal] = (syscall_handler_t)sys_signal,
    [SYS_sigpromask] = (syscall_handler_t)sys_sigpromask,
    [SYS_raise] = (syscall_handler_t)sys_raise,
    [SYS_kill] = (syscall_handler_t)sys_kill,
    [SYS_sigpending] = (syscall_handler_t)sys_sigpending,
    [SYS_pause] = (syscall_handler_t)sys_pause,
    [SYS_malloc] = (syscall_handler_t)sys_malloc,
    [SYS_free] = (syscall_handler_t)sys_free,
    [SYS_open] = (syscall_handler_t)sys_open,
    [SYS_read] = (syscall_handler_t)sys_read,
    [SYS_lseek] = (syscall_handler_t)sys_lseek,
    [SYS_close] = (syscall_handler_t)sys_close,
    [SYS_execve] = (syscall_handler_t)sys_execve,
    [SYS_umask] = (syscall_handler_t)sys_umask,
};

int syscall_resolve(syscall_args_t* args)
{
    int ret;
    task_t* task = cur_task();
    int syscall_table_entry_cnt = sizeof(sys_table) / sizeof(sys_table[0]);
    if(args->id >= syscall_table_entry_cnt){
        dbg_error("task pid:%d,Unknown syscall: %d\r\n", task->pid, args->id);
    }
    irq_enable_global();//都没问题了，开中断，执行系统调用
    ret = sys_table[args->id](args->arg0,args->arg1,args->arg2,args->arg3,args->arg4,args->arg5);

    return ret;
    
}

