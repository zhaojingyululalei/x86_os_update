

#include "ipc/mutex.h"
#include "ipc/semaphore.h"
#include "printk.h"
#include "task/task.h"
#include "time/time.h"
#include "errno.h"
sem_t sem;
mutex_t mutex;
void func2(void)
{
    irq_enable_global();
    int b = 0;

    while (1)
    {

        sys_mutex_lock(&mutex);
        sys_sleep(2000);
        dbg_info("i am func2\r\n");
        sys_mutex_unlock(&mutex);
    }
}
void func1(void)
{
    irq_enable_global();
    

   
   
    // time_t cur_time = sys_time(NULL);
    // int ret = sys_sem_timewait(&sem,cur_time+5);
    while (true)
    {
         sys_mutex_lock(&mutex);
        dbg_info("i am func 1,time wait ,err=%d\r\n", errno);
        sys_sleep(1000);
        sys_mutex_unlock(&mutex);
    }
}
void ipc_test(void)
{
    sys_sem_init(&sem, 1);
    sys_mutex_init(&mutex);
    create_kernel_task((ph_addr_t)func1, "func one", TASK_PRIORITY_DEFAULT, NULL);
    create_kernel_task((ph_addr_t)func2, "func two", TASK_PRIORITY_DEFAULT, NULL);
}