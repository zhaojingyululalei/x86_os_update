#ifndef __NET_SUBMIT_H
#define __NET_SUBMIT_H

#include "types.h"
#include "net_tools/net_threads.h"

#define APP_TASK_FUNC_MAX_SIZE  50
typedef void* (*app_func_t) (void* arg);
typedef struct _net_app_task_t
{
    app_func_t func;
    void* arg;
    void* ret;
    semaphore_t sem;//等待工作线程执行完成，获取返回值
}net_app_task_t;

void net_submit_init(void);
void* net_task_submit(app_func_t func,void* arg);


#endif