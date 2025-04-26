#include "net_threads.h"

int lock(lock_t* lk)
{
    
    sys_mutex_lock(lk);
    return 0;
}
int unlock(lock_t* lk)
{
    
    sys_mutex_unlock(lk);
    return 0;
}

int lock_init(lock_t* lk)
{
    sys_mutex_init(lk);
    return 0;
}

int lock_destory(lock_t* lk)
{
    
    sys_mutex_destory(lk);
    return 0;
}

int semaphore_init(semaphore_t* sem,uint32_t count)
{
    sys_sem_init(sem,count);
    return 0;
}
int semaphore_destory(semaphore_t* sem)
{
    return 0;
}

int semaphore_wait(semaphore_t* sem)
{
    sys_sem_wait(sem);
    return 0;
}

#include "rtc.h"
/*毫秒*/
int time_wait(semaphore_t *sem, int timeout) {
    if(!sem)
        return -1;
    irq_state_t state = irq_enter_protection();
    if (timeout < 0) {
        // timeout < 0 时，尝试非阻塞地获取信号量
        int ret = sys_sem_trywait(sem);
        if (ret == 0) {
            irq_leave_protection(state);
            return 0;  // 成功获取信号量
        } else {
            irq_leave_protection(state);
            return -1;  // 没有资源，立即返回
        }
    } else if (timeout == 0) {
        // timeout == 0 时，一直阻塞等待直到获取到信号量
        sys_sem_wait(sem);
        irq_leave_protection(state);
        return 0;
    } else {
        // timeout > 0 时，设置超时时间
        tm_t time,tmo_time;time_t now_sec,tmo_sec;
        // 获取当前时间
        sys_get_clocktime(&time);
        now_sec = sys_mktime(&time);
        // 设置绝对超时时间
        
        uint32_t diff = (timeout + (999)) / 1000; //ms--->s 向上去整
        tmo_sec = now_sec + diff;
        sys_local_time(&tmo_time,tmo_sec);

        

        // 等待信号量，超时返回
        int ret = sys_sem_timedwait(sem, &tmo_time);
        if (ret == 0) {
            irq_leave_protection(state);
            return 0;  // 成功获取信号量
        } else if (ret == 1) {
            irq_leave_protection(state);
            return -3;  // 超时返回
        } else {
            irq_leave_protection(state);
            return -1;  // 其他错误
        }
    }
}

int semaphore_post(semaphore_t* sem)
{
    sys_sem_notify(sem);
    return 0;   
}

void thread_create(thread_func_t func)
{
   task_t* task = task_alloc();
   create_kernel_process(task,func);
}