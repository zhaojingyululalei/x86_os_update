#ifndef __SCHE_H
#define __SCHE_H
#include "task/task.h"
void task_set_ready(task_t* task);
void task_set_sleep(task_t* task);
void task_set_wait(task_t* task);
void task_set_stop(task_t* task);
task_t *cur_task(void);
task_t* parent_task(void);
task_t *next_task(void);
void set_cur_task(task_t* task);

void schedule(void);
void clock_sleep_check(void);
void clock_gwait_check(void);
#endif