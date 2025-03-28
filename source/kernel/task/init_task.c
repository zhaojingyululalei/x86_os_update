

#include "types.h"
#include "syscall/applib.h"
#include "task/signal.h"
#define NUM_CHILD 3 // 主进程创建 3 个子进程
#define SUB_CHILD 2 // 每个子进程再创建 2 个孙子进程

void create_sub_children()
{
    pid_t sub_pids[SUB_CHILD];

    for (int j = 0; j < SUB_CHILD; j++)
    {
        sub_pids[j] = fork();
        if (sub_pids[j] < 0)
        {
            printf("Fork failed\r\n");
            exit(1);
        }
        if (sub_pids[j] == 0)
        {
            // 孙子进程
            printf("Grandchild created: PID=%d, Parent PID=%d\r\n", getpid(), getppid());
            exit(j + 1); // 退出码设为 1 或 2
        }
    }

    // 子进程回收所有孙子进程
    for (int i = 0; i < SUB_CHILD; i++)
    {
        int status;
        pid_t pid = wait(&status);
        if (pid > 0)
        {
            printf("Grandchild reclaimed: PID=%d, Status=%d, Parent PID=%d\r\n",
                   pid, WEXITSTATUS(status), getpid());
        }
    }
    printf("Child exiting: PID=%d\r\n", getpid());
    exit(0);
}
int a = 10;
void fork_test(void)
{
    printf("Main process started: PID=%d\r\n", getpid());

    pid_t child_pids[NUM_CHILD];

    for (int i = 0; i < NUM_CHILD; i++)
    {
        child_pids[i] = fork();
        if (child_pids[i] < 0)
        {
            printf("Fork failed\r\n");
            exit(1);
        }
        if (child_pids[i] == 0)
        {
            // 子进程
            a = 20;

            printf("Child created: PID=%d, Parent PID=%d, a=%d\r\n", getpid(), getppid(), a);
            create_sub_children();
        }
    }

    // 主进程回收所有子进程
    for (int i = 0; i < NUM_CHILD; i++)
    {
        int status;
        pid_t pid = wait(&status);
        if (pid > 0)
        {
            printf("Child reclaimed: PID=%d, Status=%d, Main PID=%d\r\n",
                   pid, WEXITSTATUS(status), getpid());
        }
    }

    while (true)
    {
        sleep(3000);
        printf("Main process finished: PID=%d, a=%d\r\n", getpid(), 10);
    }
}

void signal_test(int signum)
{

    printf("Main process finished: PID=%d, signum=%d\r\n", getpid(), signum);

    sigreturn();
}
void signal_fork_test(void)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        printf("fork error\r\n");
    }
    else if (pid == 0)
    {

        while (true)
        {
            printf("i am child,pid=%d\r\n", getpid());
            sleep(2000);
        }

        exit(0);
    }
    else
    {
        // 父进程
        printf("i am father\r\n");
        kill(pid, SIGSTOP);
        printf("father send SIGSTOP\r\n");
        sleep(2000);
        kill(pid, SIGCONT);
    }
    // 主进程回收所有子进程
    for (;;)
    {
        int status;
        pid_t cid = wait(&status);
        if (cid > 0)
        {
            printf("Child reclaimed: PID=%d, Status=%d, Main PID=%d\r\n",
                   cid, WEXITSTATUS(status), getpid());
        }
    }
    // int a = 0;
    // while (true)
    // {
    //     printf("i am init task\r\n");
    //     signal(SIGUSR1, signal_test);
    //     raise(SIGUSR1);
    //     sleep(2000);
    // }
}
void malloc_test(void)
{
    // 父进程
    int *ret = malloc(4096);
    // while (1)
    // {
    printf("father ret = 0x%x\r\n", ret);
    sleep(1000);
    // }
    pid_t pid = fork();
    if (pid < 0)
    {
        printf("fork error\r\n");
    }
    else if (pid == 0)
    {

        int *ret = malloc(4096);
        while (true)
        {

            printf("child ret = 0x%x\r\n", ret);
            sleep(1000);
        }

        exit(0);
    }
    else
    {
        
        
    }
    // 主进程回收所有子进程
    for (;;)
    {
        int status;
        pid_t cid = wait(&status);
        if (cid > 0)
        {
            printf("Child reclaimed: PID=%d, Status=%d, Main PID=%d\r\n",
                   cid, WEXITSTATUS(status), getpid());
        }
    }
    // int* ret = malloc(4096);
    // printf("father ret = 0x%x\r\n",ret);
    // while (true)
    // {
    //     sleep(2000);
    // }
}
void execve_test(void)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        printf("fork error\r\n");
    }
    else if (pid == 0)
    {

        
        char *argv[] = {"hello","world",NULL};
        char *env[] = {"zhao","jing","yu",NULL};
        execve("/shell", argv, env);

        exit(0);
    }
    else
    {
        while (true)
        {
            printf("i am father\r\n");
            sleep(2000);
        }
    }
    // 主进程回收所有子进程
    for (;;)
    {
        int status;
        pid_t cid = wait(&status);
        if (cid > 0)
        {
            printf("Child reclaimed: PID=%d, Status=%d, Main PID=%d\r\n",
                   cid, WEXITSTATUS(status), getpid());
        }
    }
}
void init_task_main(void)
{
    execve_test();
}