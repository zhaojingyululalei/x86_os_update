#ifndef __MUTEX_H
#define __MUTEX_H

#include "types.h"
#include "tools/list.h"

/**
 * @brief 不可重入锁
 */
typedef struct _mutex_t {
    
    int count;				// 信号量计数
    list_t wait_list;		// 等待的进程列表
}mutex_t;

/**
 * @brief 可重入锁
 */
typedef struct _recursive_mutex_t{
    list_node_t* owner; //任务
    int locked_count;
    list_t wait_list;
}recursive_mutex_t;
void sys_mutex_init (mutex_t * mutex);
void sys_mutex_lock (mutex_t * mutex);
void sys_mutex_unlock (mutex_t * mutex);
int sys_mutex_destory (mutex_t * mutex);

void sys_recur_mutex_init (recursive_mutex_t * mutex);
void sys_recur_mutex_lock (recursive_mutex_t * mutex);
void sys_recur_mutex_unlock (recursive_mutex_t * mutex);
int sys_recur_mutex_destory (recursive_mutex_t * mutex);
#endif