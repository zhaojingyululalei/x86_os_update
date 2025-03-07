#ifndef __SCHE_H
#define __SCHE_H
#include "task/task.h"
void task_set_ready(task_t* task);
task_t *cur_task(void);
task_t *next_task(void);
extern void task_switch(task_t* next);
void schedule(void);
void set_cur_task(task_t* task);
#endif