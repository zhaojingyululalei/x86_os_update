#include "syscall/syscall.h"
#include "kernel_test.h"
#include "printk.h"
#include "task/task.h"
#include "task/sche.h"
#include "irq/irq.h"
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