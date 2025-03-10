#include "ipc/mutex.h"
#include "irq/irq.h"
#include "task/task.h"
#include "task/sche.h"
void sys_mutex_init(mutex_t *mutex)
{
    mutex->count = 1;
    list_init(&mutex->wait_list);
}
void sys_mutex_lock(mutex_t *mutex)
{
    irq_state_t irq_state = irq_enter_protection();

    if (mutex->count > 0)
    {
        mutex->count--;
    }
    else
    {
        // 从就绪队列中移除，然后加入信号量的等待队列
        task_t *curr = cur_task();
        curr->state = TASK_STATE_WAITING;
        list_insert_last(&mutex->wait_list, &curr->node);
        curr->list = &mutex->wait_list;
        sys_yield();
    }

    irq_leave_protection(irq_state);
}
void sys_mutex_unlock(mutex_t *mutex)
{
    irq_state_t irq_state = irq_enter_protection();
    if (list_count(&mutex->wait_list))
    {
        // 有进程等待，则唤醒加入就绪队列
        list_node_t *fnode = list_remove_first(&mutex->wait_list);
        task_t *task = list_node_parent(fnode, task_t, node);
        task_set_ready(task);
        sys_yield();
    }
    else
    {
        mutex->count++;
    }
     irq_leave_protection(irq_state);
}

int sys_mutex_destory(mutex_t *mutex){
    if(mutex->count==0){
        //确保没有任务持有锁
        return -1;
    }
    else if(list_count(&mutex->wait_list)){
        //确保没有任务正在等待锁
        return -2;
    }else{
        return 0;
    }
}


void sys_recur_mutex_init (recursive_mutex_t * mutex){
    mutex->locked_count = 0;
    mutex->owner = NULL;
    list_init(&mutex->wait_list);
}
void sys_recur_mutex_lock (recursive_mutex_t * mutex){
    irq_state_t  irq_state = irq_enter_protection();

    task_t * curr = cur_task();
    if (mutex->locked_count == 0) {
        // 没有任务占用，占用之
        mutex->locked_count = 1;
        mutex->owner = &curr->node;
    } else if (mutex->owner == &curr->node) {
        // 已经为当前任务所有，只增加计数
        mutex->locked_count++;
    } else {
        // 有其它任务占用，则进入队列等待
        curr->state = TASK_STATE_WAITING;
        list_insert_last(&mutex->wait_list, &curr->node);
        curr->list = &mutex->wait_list;
        sys_yield();
    }

    irq_leave_protection(irq_state);
}
void sys_recur_mutex_unlock (recursive_mutex_t * mutex){
    irq_state_t  irq_state = irq_enter_protection();

    // 只有锁的拥有者才能释放锁
    task_t * curr = cur_task();
    if (mutex->owner == &curr->node) {
        if (--mutex->locked_count == 0) {
            // 减到0，释放锁
            mutex->owner = NULL;

            // 如果队列中有任务等待，则立即唤醒并占用锁
            if (list_count(&mutex->wait_list)) {
                list_node_t * task_node = list_remove_first(&mutex->wait_list);
                task_t * task = list_node_parent(task_node, task_t, node);
                task_set_ready(task);

                // 在这里占用，而不是在任务醒后占用，因为可能抢不到
                mutex->locked_count = 1;
                mutex->owner = &task->node;
                //在这里立马调度，怕别的进程抢不到
                sys_yield();
            }
        }
    }

    irq_leave_protection(irq_state);
}
int sys_recur_mutex_destory (recursive_mutex_t * mutex){
    if(mutex->locked_count!=0){
        //说明有进程持有锁
        return -1;
    }else if(list_count(&mutex->wait_list)){
        //说明有进程等待锁
        return -2;
    }else{
        return 0;
    }
}

