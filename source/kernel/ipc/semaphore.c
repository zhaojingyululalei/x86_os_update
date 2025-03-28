#include "ipc/semaphore.h"
#include "irq/irq.h"
#include "task/task.h"
#include "task/sche.h"
#include "string.h"
#include "time/time.h"
#include "tools/list.h"
list_t global_tmwait_list;
void sys_sem_init(sem_t *sem, int count)
{
    memset(sem, 0, sizeof(sem_t));
    sem->count = count;
    list_init(&sem->wait_list);
    list_init(&sem->tmwait_list);
}

void sys_sem_wait(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();

    if (sem->count > 0)
    {
        sem->count--;
    }
    else
    {
        // 从就绪队列中移除，然后加入信号量的等待队列
        task_t *curr = cur_task();
        curr->state = TASK_STATE_WAITING;
        list_insert_last(&sem->wait_list, &curr->node);
        curr->list = &sem->wait_list;
        sys_yield();
    }

    irq_leave_protection(irq_state);
}
/**
 * @result 0:成功获得资源   -1:没有获得资源
 */
int sys_sem_trywait(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();

    if (sem->count > 0)
    {
        sem->count--;
        irq_leave_protection(irq_state);
        return 0;
    }
    else
    {
        irq_leave_protection(irq_state);
        return -1;
    }
}
/**
 * @brief 等待一定时间，没唤醒自动返回
 * @result 0:成功获取 -1:超时返回
 * @note 传入的是绝对时间，而且单位是秒，不是毫秒
 */
int sys_sem_timewait(sem_t *sem, time_t abs_timeout)
{
    irq_state_t irq_state = irq_enter_protection();

    if (sem->count > 0)
    {
        sem->count--; // 有资源直接返回
        irq_leave_protection(irq_state);
        return 0;
    }
    else
    {
        // 没有资源 需要加入两个队列
        // 1.sem中的wait_list
        // 2.全局的global_tmwait_list
        // 从就绪队列中移除，然后加入信号量的等待队列
        task_t *curr = cur_task();
        curr->state = TASK_STATE_WAITING;
        list_insert_last(&sem->wait_list, &curr->node);
        curr->list = &sem->wait_list;

        list_insert_last(&global_tmwait_list, &curr->time_node);
        curr->wake_time = abs_timeout;
        sys_yield();
        irq_leave_protection(irq_state);
        //现在要区分他是超时唤醒，还是资源到达唤醒
        time_t curtime = sys_time(NULL);
        if(curtime >= abs_timeout){
            return -1; // 超时返回
        }
        else{
            return 0;
        }
        
    }
}
void sys_sem_notify(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();

    if (list_count(&sem->tmwait_list))
    {
        // 有进程等待，需要同时移除 sem.tmwait_list和global_tmwait_list,然后将任务加入就绪队列
        list_node_t *fnode = list_remove_first(&sem->tmwait_list);
        task_t *task = list_node_parent(fnode, task_t, node);

        list_remove(&global_tmwait_list,&task->time_node);
        task_set_ready(task);
        sys_yield();
    }
    else
    {
        if (list_count(&sem->wait_list))
        {
            // 有进程等待，则唤醒加入就绪队列
            list_node_t *fnode = list_remove_first(&sem->wait_list);
            task_t *task = list_node_parent(fnode, task_t, node);
            task_set_ready(task);
            sys_yield();
        }
        else
        {
            sem->count++;
        }
    }

    irq_leave_protection(irq_state);
}

int sys_sem_count(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();
    int count = sem->count;
    irq_leave_protection(irq_state);
    return count;
}