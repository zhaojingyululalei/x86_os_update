#include "net_submit.h"
#include "net_tools/net_mmpool.h"
#include "string.h"
#include "net_msg.h"
#include "net_msg_queue.h"
#include "printk.h"
/*内存池结构*/
static mempool_t net_task_pool;
static uint8_t net_task_buf[APP_TASK_FUNC_MAX_SIZE*(sizeof(net_app_task_t)+sizeof(list_node_t))];

void net_submit_init(void)
{
    mempool_init(&net_task_pool,net_task_buf,APP_TASK_FUNC_MAX_SIZE,sizeof(net_app_task_t));
}

static net_app_task_t* net_task_alloc(void)
{
    return mempool_alloc_blk(&net_task_pool,-1);
}

static void net_task_free(net_app_task_t* net_task)
{
    memset(net_task,0,sizeof(net_app_task_t));
    mempool_free_blk(&net_task_pool,net_task);
}

void* net_task_submit(app_func_t func,void* arg)
{
    int ret;
    //获取结构
    net_app_task_t* task = net_task_alloc();
    memset(task,0,sizeof(net_app_task_t));
    if(!task)
    {
        dbg_error("net task pool memory out,please expand\r\n");
        
    }

    //赋值
    task->arg = arg;
    task->func = func;
    task->ret = NULL;
    semaphore_init(&task->sem,0);

    //封装成msg
    net_msg_t* msg = net_msg_create(NULL,NET_MSG_TYPE_NET_FUNC,task);
    if(!msg)
    {
        dbg_error("worker net_msg_t pool memory out\r\n");
        net_task_free(task);
        
    }
    //放入工作线程的消息队列
    ret = net_msg_enqueue(msg,-1);
    if(ret < 0)
    {
        dbg_error("worker net_msg_queue full,drop one net task\r\n");
        
    }

    //等待工作线程取出任务执行
    semaphore_wait(&task->sem);
    //工作线程执行完任务，唤醒
    void* return_value = task->ret;
    //释放
    net_task_free(task);
    return return_value;

}