

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
    char str[6] = "hello\0";
    int child_cnt = 3;
    for (int i = 0; i < 2; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            a = 16;
            // 子进程不应该继续循环 `fork()`
            int cid= getpid();
            int ppid = getppid();
            
            printf("i am child:%d,ppid=%d\r\n",getpid(),getppid());
            break;
        }
        else if (pid > 0)
        {
            int theid = getpid();
            a++;
            printf("i am father pid=%d,create child pid=%d\r\n",theid , pid);
        }
        else
        {
            printf("fork err\r\n");
        }
    }

    while (true)
    {

        sleep(6000);
        printf("a=%d\r\n", a);
    }
}