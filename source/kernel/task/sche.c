#include "task/sche.h"
#include "printk.h"

void task_set_ready(task_t *task)
{
    task->state = TASK_STATE_READY;
    list_insert_last(&task_manager.ready_list, &task->node);
}

task_t *cur_task(void)
{
    return task_manager.cur_task;
}
void set_cur_task(task_t *task)
{
    task_manager.cur_task = task;
}
task_t *next_task(void)
{

    task_t *cur = cur_task();
    if (cur == task_manager.idle)
    {
        // 如果当前任务是空闲任务
        // 那么就查看就绪队列里有没有任务，有任务，返回第一个即可
        if (list_count(&task_manager.ready_list))
        {
            return list_node_parent(task_manager.ready_list.first, task_t, node);
            
        }
        else
        {
            // 如果就绪队列里还没任务，就接着执行idle
            return cur;
        }
    }
    // 如果不是空闲任务
    list_node_t *next_node = cur->node.next;
    if (next_node)
    {
        // 下一个任务存在就返回
        return list_node_parent(next_node, task_t, node);
    }
    else
    {
        // 链表里就一个任务
        return cur;
    }
}

void schedule(void)
{
    //获取当前任务
    task_t *cur = cur_task();
    if (cur->state != TASK_STATE_RUNNING)
    {
        dbg_error("task state err\r\n");
        return;
    }
    //获取下一个要执行的任务
    task_t *next = next_task();
    ASSERT(next!=NULL);
    if(next == cur){
        //不用调度
        return;
    }
    task_activate(next);
    //如果不是空闲任务
    if (cur != task_manager.idle)
    {
        // 空闲任务除外
        // 如果当前任务竟然不是readylist的第一个任务，那么就调度出错了
        if (task_manager.ready_list.first != &cur->node)
        {
            dbg_error("schedule err:cur task is not ready list first task\r\n");
            return;
        }
        list_remove(&task_manager.ready_list, &cur->node);      // 从就绪队列中移除
        list_insert_last(&task_manager.ready_list, &cur->node); // 插入就绪队列尾部
    }

    cur->state = TASK_STATE_READY; // 将running态改为ready态
    
    next->state = TASK_STATE_RUNNING;
    task_switch(next);
}