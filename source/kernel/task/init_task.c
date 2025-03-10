

#include "types.h"
#include "syscall/applib.h"
// int sys_call(syscall_args_t *args)
// {
//     int ret;

//     asm volatile (
//             "mov "
//             "sysenter\n\t"                // 触发系统调用
//             "mov %%eax, %0\n\t"           // 取返回值
//             : "=r"(ret)                   // 输出返回值
//             : "a"(args)                   // 将 args 的地址直接放入 EDI
//             : "%eax", "%ebx", "%ecx", "%edx", "%esi", "memory"
//     );

//     return ret;
// }
// extern void *sys_call(void *);

// int calc_add(int a, int b, int c, int d, int e)
// {
//     int ret;
//     syscall_args_t arg;
//     arg.id = SYS_test;
//     arg.arg0 = a;
//     arg.arg1 = b;
//     arg.arg2 = c;
//     arg.arg3 = d;
//     arg.arg4 = e;
//     ret = sys_call(&arg);
//     return ret;
// }
void init_task_main(void)
{
    int a = 10;
    int b = 0;
    int c = 0;
    while (true)
    {
        
        //sleep(1000);
        printf("calc ret = %d\r\n",c);
        //yield();
        c++;
        
    }
}