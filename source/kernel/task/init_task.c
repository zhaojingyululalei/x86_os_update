

#include "types.h"
#include "syscall/applib.h"

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
void init_task_main(void)
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
            printf("Child created: PID=%d, Parent PID=%d\r\n", getpid(), getppid());
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
        printf("Main process finished: PID=%d\r\n", getpid());
    }
}