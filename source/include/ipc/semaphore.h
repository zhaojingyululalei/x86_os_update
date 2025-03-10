#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#include "tools/list.h"
#include "types.h"
typedef struct _sem_t {
    
    int count;				// 信号量计数
    list_t wait_list;		// 等待的进程列表
    list_t tmwait_list;     //用于超时等待的链表
}sem_t;
extern list_t global_tmwait_list;

void sys_sem_init(sem_t *sem, int count);
void sys_sem_wait(sem_t *sem);
int sys_sem_trywait(sem_t *sem);
int sys_sem_timewait(sem_t *sem, uint32_t abs_timeout);
void sys_sem_notify(sem_t *sem);
#endif