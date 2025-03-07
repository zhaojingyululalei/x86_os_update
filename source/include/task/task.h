#ifndef __TASK_H
#define __TASK_H
#include "types.h"
#include "tools/list.h"
#include "tools/id.h"
#include "mem/pdt.h"
#define TASK_LIMIT_CNT  512
#define TASK_PRIORITY_DEFAULT   1
#define TASK_PID_START 0
#define TASK_PID_END    (TASK_PID_START+TASK_LIMIT_CNT-1)
#define STACK_MAGIC "magic"
#define STACK_MAGIC_LEN 5
struct _task_t;
typedef struct _task_manager_t{
    id_pool_t pid_pool;//给任务分配pid用的id池
    struct _task_t* cur_task;//指示当前任务
    //只要创建了任务就放在这里，下标是pid.通过pid快速定位task
    struct _task_t* tasks[TASK_LIMIT_CNT];
    list_t ready_list;
    list_t sleep_list;
    struct _task_t* init;
    struct _task_t *idle; //就绪队列没有任务了就让这个执行
}task_manager_t;

typedef struct _task_attr_t{
    size_t stack_size;
    size_t heap_size;

}task_attr_t;
typedef struct _task_t
{
    uint32_t esp;//存放栈顶地址
    enum 
    {
        TASK_STATE_NONE,
        TASK_STATE_CREATED,
        TASK_STATE_RUNNING,
        TASK_STATE_SLEEP,
        TASK_STATE_READY,
        TASK_STATE_WAITING,
        TASK_STATE_ZOMBIE,
    } state;
    enum {
        TASK_TYPE_TASKNONE,
        TASK_TYPE_KERNEL,
        TASK_TYPE_USR
    }type;
    int pid; //每个任务的pid都不同
    char name[16];
    char* stack_magic;
    uint32_t priority; //任务优先级，优先级高的会多分一些时间片
    int ticks; //每时间片-1

    page_entry_t *page_table;
    ph_addr_t heap_base; //堆的起始地址
    task_attr_t attr;
    list_node_t node;
    list_node_t pool_node; //用于快速分配释放task_t结构
}task_t;


typedef struct task_frame_t
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip;
} task_frame_t;

extern task_manager_t task_manager;
void task_manager_init(void);
task_t *create_task(ph_addr_t entry, const char *name, uint32_t priority, task_attr_t *attr);
bool is_stack_magic(task_t* task);
void task_activate(task_t* task);
#endif