#include "task/sche.h"
#include "printk.h"
#include "irq/irq.h"
#include "ipc/semaphore.h"
#include "time/time.h"
#include "errno.h"
/**
 * @brief 将任务加入就绪队列
 */
void task_set_ready(task_t *task)
{
    irq_state_t state = irq_enter_protection();
    task->state = TASK_STATE_READY;
    list_insert_last(&task_manager.ready_list, &task->node);
    task->list = &task_manager.ready_list;
    irq_leave_protection(state);
}
/**
 * @brief 将任务加入睡眠队列
 */
void task_set_sleep(task_t* task){
    irq_state_t state = irq_enter_protection();
    task->state = TASK_STATE_SLEEP;
    list_insert_last(&task_manager.sleep_list, &task->node);
    task->list = &task_manager.sleep_list;
    irq_leave_protection(state);
}
task_t *cur_task(void)
{
    task_t* task;
    irq_state_t state = irq_enter_protection();
    task = task_manager.cur_task;
    irq_leave_protection(state);
    return task;
}
void set_cur_task(task_t *task)
{
   irq_state_t state = irq_enter_protection();
    task_manager.cur_task = task;
    irq_leave_protection(state);
}
/**
 * @brief 获取下一个即将执行的任务
 */
task_t *next_task(void)
{
    task_t *cur = cur_task();
    task_t* next ;
    irq_state_t state = irq_enter_protection();
    //查看就绪队列中有没有任务
        //有任务：就让他执行
        //没任务：还让当前任务执行
    if(list_count(&task_manager.ready_list)){
        next = list_node_parent(task_manager.ready_list.first,task_t,node);
    }else{
        if(cur->state == TASK_STATE_RUNNING && !cur->list){
            //如果当前任务没有进入一些阻塞队列
            next = cur;
        }else{
            //否则让空闲任务执行（当前任务一定是加入到某个阻塞队列了）
            next = task_manager.idle;
        }
    }
    irq_leave_protection(state);
    return next;
    
}

/**
 * @brief 调度
 */
void schedule(void)
{
    //获取当前任务
    task_t *cur = cur_task();
    //ASSERT(cur->state == TASK_STATE_RUNNING || cur->state == TASK_STATE_SLEEP);
    irq_state_t state = irq_enter_protection();
    
    if(!cur){
        irq_leave_protection(state);
        dbg_error("cur task is null\r\n");
        return;//系统还没初始化任务调度
    }
    //获取下一个要执行的任务
    task_t *next = next_task();
    ASSERT(next!=NULL);
    
    if(next == cur){
        //不用调度
        irq_leave_protection(state);
        return;
    }
    ASSERT(next->state == TASK_STATE_READY);
    //如果当前任务是正在运行的普通任务
    if(cur->state == TASK_STATE_RUNNING){
        if(cur!=task_manager.idle){
            //空闲任务不用加入就绪队列
            task_set_ready(cur);
        }
        else{
            task_manager.idle->state = TASK_STATE_READY;
        }
        
    }else{
        //如果当前任务已经被加入了 睡眠或者其他阻塞队列
        //不用入就绪队列
        ;
    }

    if(next != task_manager.idle){
        //下一个要运行的任务要离开就绪队列
        list_remove(&task_manager.ready_list,&next->node);
        next->list =NULL;
    }
    
    //激活下一个任务的环境
    task_activate(next);
    next->state = TASK_STATE_RUNNING;
    irq_leave_protection(state);
    //task_swtich:
    //获取当前任务，并切换当前任务为next
    task_switch(next); 
}

/**
 * @brief 定时器检查睡眠时间片
 */
void clock_sleep_check(void){
    //遍历睡眠队列，只要sleepticks减到0，就送入就绪队列

    list_node_t *sleep_node = task_manager.sleep_list.first;
    while (sleep_node)
    {
        list_node_t* next = sleep_node->next;
        task_t* sleep_task= list_node_parent(sleep_node,task_t,node);
        ASSERT(sleep_task->state == TASK_STATE_SLEEP && sleep_task->list==&task_manager.sleep_list);
        if(!(--sleep_task->sleep_ticks)){
            list_remove(&task_manager.sleep_list,sleep_node);
            task_set_ready(sleep_task);
        }
        sleep_node = next;
    }
    
}

/**
 * @brief 遍历全局等待队列,把所有超时任务加入就绪队列
 */

void clock_gwait_check(void){
    if(!list_count(&global_tmwait_list)){
        return;
    }
    time_t cur_time = sys_time(NULL);
    list_node_t* wait_node = global_tmwait_list.first;
    while (wait_node)
    {
        list_node_t* next_node = wait_node->next;
        task_t* task = list_node_parent(wait_node,task_t,time_node);
        //检查是否超时
        if(cur_time >= task->wake_time){
            //超时唤醒 从两个等待队列中移除，并加入就绪队列
            list_remove(&global_tmwait_list,wait_node);
            ASSERT(task->state == TASK_STATE_WAITING);
            list_remove(task->list,&task->node);
            task_set_ready(task);

            task->err_num = ETIMEOUT;

        }
        wait_node = next_node;
    }
    
}
