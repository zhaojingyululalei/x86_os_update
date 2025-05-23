#ifndef __TASK_H
#define __TASK_H

#include "types.h"
#include "tools/list.h"
#include "tools/id.h"
#include "mem/pdt.h"
#include "cpu.h"
#include "irq/traps.h"
#include "signal.h"
#include "mem/buddy_system.h"
#include "fs/inode.h"
#include "fs/file.h"
#define TASK_LIMIT_CNT 512
#define TASK_PRIORITY_DEFAULT 1
#define TASK_PID_START 0
#define TASK_PID_END (TASK_PID_START + TASK_LIMIT_CNT - 1)
#define STACK_MAGIC "magic"
#define STACK_MAGIC_LEN 5
#define TASK_OFILE_NR				128			// 最多支持打开的文件数量
struct _task_t;
typedef struct _task_manager_t
{
    id_pool_t pid_pool;       // 给任务分配pid用的id池
    struct _task_t *cur_task; // 指示当前任务
    // 只要创建了任务就放在这里，下标是pid.通过pid快速定位task
    struct _task_t *tasks[TASK_LIMIT_CNT];
    list_t ready_list;
    list_t sleep_list;
    list_t wait_list; // 父进程等待回收子进程，暂时不能去就绪队列中运行
    list_t stop_list; // 收到stop信号，就来这里
    struct _task_t *init;
    struct _task_t *idle; // 就绪队列没有任务了就让这个执行
    /**tss */
    tss_t tss;
} task_manager_t;

typedef struct _task_attr_t
{
    size_t stack_size;
    size_t heap_size;

} task_attr_t;
typedef struct _task_t
{
    uint32_t esp; // 存放栈顶地址
    // 中断发生，系统调用，系统都会自动把一些参数压入内核栈
    uint32_t esp0;
    uint32_t entry;
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
    exception_frame_t frame; // 用于恢复signal的上下文
    list_t *list;            // 任务当前所在队列
    list_t child_list;       // 存放所有子进程
    int err_num;             // 错误号，每个线程独有
    int uid;
    int gid;
    int pid;                 // 每个任务的pid都不同
    int ppid;                // 父进程pid
    int status;              // 高8位存放信号信息，低8位存退出码
    char name[16];
    char *stack_magic;
    uint32_t priority;   // 任务优先级，优先级高的会多分一些时间片
    int ticks;           // 每时间片-1
    int sleep_ticks;     // 睡眠时间片
    uint32_t alarm_time; // 闹钟时间
    int argc, envc;
    char **argv, **env;
    page_entry_t *page_table;
    addr_t heap_base;  // 堆的起始地址 没进入用户态，存放物理地址，进入用户态，存放虚拟地址
    addr_t stack_base; // 栈的起始地址 没进入用户态，存放物理地址，进入用户态，存放虚拟地址
    addr_t signal_stack_base;
    task_attr_t attr; // 任务属性
    buddy_mmpool_dyn_t m_pool;
    signal_handler s_handler[SIGNAL_MAX_CNT];
    sigset_t s_mask;
    sigset_t s_pending;
    list_node_t node;       // 就绪队列，睡眠队列 等待队列
    list_node_t child_node; // 子进程退出，放入父进程的子进程队列中记录
    list_node_t time_node;  // 全局超时等待队列
    list_node_t page_node;  // 记录每个页被哪些进程拥有
    uint32_t wake_time;     // 任务等待超时时间

    list_node_t pool_node; // 用于快速分配释放task_t结构
    bool paused;
    bool stop;
    inode_t* ipwd;  //当前所在目录
    inode_t* iroot; //当前所在根目录
    uint16_t umask;
    file_t * file_table[TASK_OFILE_NR];	// 任务最多打开的文件数量
} task_t;

typedef struct task_frame_t
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip;
} task_frame_t; // 调用约定栈帧，被调用者保存的
typedef struct _sysenter_frame_t
{
    uint32_t esp;
    uint32_t edx; // 存着调用返回的ip
    uint32_t ecx; // 存着调用返回的esp
    uint32_t esi;
    uint32_t ebp;
} sysenter_frame_t;
//c运行时栈帧
typedef struct _cr0_frame_t
{
    int argc;
    char **argv;
    int envc;
    char **env;
    uint32_t eip;
} cr0_frame_t;

extern task_manager_t task_manager;
void task_manager_init(void);
task_t *create_task(addr_t entry, const char *name, uint32_t priority, task_attr_t *attr);
bool is_stack_magic(task_t *task);
void task_activate(task_t *task);
void task_switch(task_t *next);
void jmp_to_usr_mode(void);
task_t *create_kernel_task(addr_t entry, const char *name, uint32_t priority, task_attr_t *attr);
void task_list_debug(void);
int task_get_errno(void);
int task_collect(task_t *task);

int task_alloc_fd (file_t * file);
file_t * task_file (int fd);
void task_remove_fd (int fd);


void sys_sleep(uint32_t ms);
void sys_yield(void);
int sys_fork(void);
int sys_getpid(void);
int sys_getppid(void);
int sys_wait(int *status);
void sys_exit(int status);
int sys_execve(const char *path, char *const *argv, char *const *env);
uint16_t sys_umask(uint16_t mask);
#endif