#ifndef __NET_THREADS_H
#define __NET_THREADS_H

#include "ipc/mutex.h"
#include "ipc/semaphore.h"
typedef mutex_t lock_t;
#define DEFINE_PROCESS_FUNC(name)            void* name(void* arg)
typedef void*(*process_func_t)(void*);
int lock(lock_t* lk);
int unlock(lock_t* lk);
int lock_init(lock_t* lk);
int lock_destory(lock_t* lk);

typedef sem_t semaphore_t;
int semaphore_init(semaphore_t* sem,uint32_t count);
int semaphore_destory(semaphore_t* sem);
int semaphore_wait(semaphore_t* sem);
/**
 * @return timeout<0 非阻塞，立即返回  timeout==0阻塞死等  timeout>0等够时间返回
 */
int time_wait(semaphore_t* sem, int timeout);
int semaphore_post(semaphore_t* sem);

typedef process_func_t thread_func_t;
void thread_create(thread_func_t func);

#endif