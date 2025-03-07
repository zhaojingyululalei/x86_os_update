
#include "task/task.h"
#include "mem/bitmap.h"
#include "string.h"
#include "printk.h"
#include "cpu_cfg.h"
#include "cpu_instr.h"
#include "task/sche.h"
#include "algrithm.h"
static task_t task_buf[TASK_LIMIT_CNT];
static list_t task_pool;
task_manager_t task_manager;

static void task_pool_init(void)
{
    list_init(&task_pool);
    for (int i = 0; i < TASK_LIMIT_CNT; i++)
    {
        list_insert_last(&task_pool, &task_buf[i].pool_node);
    }
}
static task_t *task_pool_alloc(void)
{
    if (task_pool.count == 0)
    {
        dbg_error("task_pool err:task_t struct is not enough\r\n");
        return NULL;
    }
    list_node_t *node = list_remove_first(&task_pool);
    return list_node_parent(node, task_t, pool_node);
}
static void task_pool_free(task_t *task)
{
    list_insert_last(&task_pool, &task->pool_node);
}

static void idle_task(void)
{
    while (1)
    {
        int x = 1;
        x++;
    }
}
extern void first_task(void);
void task_manager_init(void)
{
    task_pool_init();
    id_pool_init(TASK_PID_START, TASK_PID_END, &task_manager.pid_pool);
    list_init(&task_manager.ready_list);
    list_init(&task_manager.sleep_list);
    //创建init进程
    task_manager.init = create_task((ph_addr_t)first_task,"init",TASK_PRIORITY_DEFAULT,NULL);
    //创建空闲进程
    task_manager.idle = create_task((ph_addr_t)idle_task, "idle", TASK_PRIORITY_DEFAULT, NULL);
    //注：空闲进程得从就绪队列中取出 仍为就绪态
    list_remove(&task_manager.ready_list,&task_manager.idle->node);

    set_cur_task(NULL);
}

task_t *create_task(ph_addr_t entry, const char *name, uint32_t priority, task_attr_t *attr)
{
    task_t *task = task_pool_alloc();
    if (!task)
        return NULL;

    if (attr)
    {
        task->attr.stack_size = align_up(attr->stack_size,MEM_PAGE_SIZE);
        task->attr.heap_size = align_up(attr->heap_size,MEM_PAGE_SIZE);
    }
    else{
        task->attr.heap_size = MEM_PAGE_SIZE;
        task->attr.stack_size = MEM_PAGE_SIZE;
    }
    strcpy(task->name, name);

    // 给进程分配栈空间
    ph_addr_t stack_base = mm_bitmap_alloc_pages(task->attr.stack_size / MEM_PAGE_SIZE);
    stack_base += task->attr.stack_size;//栈顶地址
    // 写入栈结束标记
    task->stack_magic = stack_base - task->attr.stack_size;
    strncpy(task->stack_magic , STACK_MAGIC, STACK_MAGIC_LEN);
    // 初始化栈帧
    stack_base -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack_base;
    frame->esi = frame->edi = frame->ebx = frame->ebp = 0;
    frame->eip = entry;

    task->esp = stack_base;
    //分配堆空间
    task->heap_base = mm_bitmap_alloc_pages((task->attr.heap_size + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE);
    task->esp = stack_base;
    task->ticks = task->priority = priority;
    task->pid = allocate_id(&task_manager.pid_pool);

    // 创建进程页表，并拷贝内核页表(记得写cr3，任务才能用自己的页表)
    task->page_table = (page_entry_t *)mm_bitmap_alloc_page();
    copy_kernel_pdt(task->page_table);

    // task加入就绪队列
    task_set_ready(task);
    task_manager.tasks[task->pid] = task;
    return task;
}
bool is_stack_magic(task_t* task){
     return  strncmp(STACK_MAGIC,task->stack_magic,STACK_MAGIC_LEN)==0;
}
void task_activate(task_t* task){
    if(!is_stack_magic(task)){
        dbg_error("stack magic err\r\n");
        return;
    }
    if((ph_addr_t)task->page_table !=(ph_addr_t)read_cr3()){
        write_cr3((ph_addr_t)task->page_table);
    }

}