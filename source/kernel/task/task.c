
#include "task/task.h"
#include "mem/bitmap.h"
#include "string.h"
#include "printk.h"
#include "cpu_cfg.h"
#include "cpu_instr.h"
#include "task/sche.h"
#include "algrithm.h"
#include "irq/irq.h"
static task_t task_buf[TASK_LIMIT_CNT];
static list_t task_pool;
/**
 * @brief 分配task_t 内存池
 */
static void task_pool_init(void)
{
    list_init(&task_pool);
    for (int i = 0; i < TASK_LIMIT_CNT; i++)
    {
        list_insert_last(&task_pool, &task_buf[i].pool_node);
    }
}
/**
 * @brief 分配一个空的task_t结构体
 */
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
/**
 * @brief 释放一个task_t结构
 */
static void task_pool_free(task_t *task)
{
    memset(task, 0, sizeof(task_t));
    list_insert_last(&task_pool, &task->pool_node);
}
/**
 * @brief 空闲任务
 */
static void idle_task(void)
{
    irq_enable_global();
    while (1)
    {
        int x = 1;
        x++;
    }
}
extern void init_task_entry(void);
task_manager_t task_manager;
/**
 * @brief 0号进程
 */
extern void first_task(void);
extern char s_init_task_ph;
extern char s_init_task_vm;
extern char e_init_task_ph;
/**
 * @brief 创建init进程  并且映射init进程的代码和数据
 */
static task_t *create_init_task(void)
{
    ph_addr_t start_ph = (ph_addr_t)&s_init_task_ph;
    ph_addr_t end_ph = (ph_addr_t)&e_init_task_ph;
    vm_addr_t start_vm = (vm_addr_t)&s_init_task_vm;
    size_t init_text_size = end_ph - start_ph;
    ASSERT(start_vm == USR_ENTRY_BASE);
    task_t *init = create_task(start_vm, "init", TASK_PRIORITY_DEFAULT, NULL);
    if (!init)
    {
        dbg_error("init process create fail\r\n");
    }
    // 然后给init进程的代码段，数据段等做映射
    pdt_set_entry(init->page_table, start_vm, start_ph, align_up(init_text_size, MEM_PAGE_SIZE), PDE_P | PDE_U | PDE_W);

    return init;
}

/**
 * @brief 跳转到用户态去执行，内核态里的东西没用了。 只能从中断或者系统调用再次返回内核态
 */
void jmp_to_usr_mode(void)
{
    int ret;
    irq_state_t state = irq_enter_protection();
    // 这是个运行在内核态的进程
    // 堆栈都分配好了
    task_t *task = cur_task();
    // 之前是堆栈都是恒等映射，现在进入用户态，重新映射

    // 给用户进程分配用户栈空间
    vm_addr_t stack_utop = USR_STACK_TOP - 1;
    vm_addr_t stack_ubase = stack_utop + 1 - task->attr.stack_size;
    ph_addr_t ph_stack_ubase = task->stack_base; // 之前的stack_base肯定是物理地址
    ph_addr_t ph_stack_utop = ph_stack_ubase + task->attr.stack_size - 1;
    ret = pdt_set_entry(task->page_table, stack_ubase, ph_stack_ubase, task->attr.stack_size, PDE_U | PDE_W | PDE_P);
    ASSERT(ret >= 0);
    task->stack_base = stack_ubase;
    // 写入栈结束标记
    task->stack_magic = stack_ubase;
    // 页表还是内核页表，写不进去。因此用映射的物理地址写
    strncpy(ph_stack_ubase, STACK_MAGIC, STACK_MAGIC_LEN);
    task->esp = stack_utop;
    /*不用写栈帧，直接iret到init进程执行即可*/

    // 给用户进程分配内核栈空间
    //(内核栈空间就是中断或者系统调用压入一些上下文，需要的空间不多，就只分配一页)
    ph_addr_t stack_kbase = mm_bitmap_alloc_pages(1);
    task->esp0 = stack_kbase + MEM_PAGE_SIZE;

    // 给用户进程重新分配堆空间
    vm_addr_t head_ubase = USR_HEAP_BASE;
    ph_addr_t head_kbase = task->heap_base;
    ret = pdt_set_entry(task->page_table, head_ubase, head_kbase, task->attr.heap_size, PDE_P | PDE_U | PDE_W);
    ASSERT(ret >= 0);
    task->heap_base = head_ubase;

    task_activate(task);
    task->state = TASK_STATE_RUNNING;

    tss_t *tss = &task_manager.tss;
    tss->ss = SELECTOR_USR_DATA_SEG;
    tss->esp = task->esp;
    tss->eflags = (0 << 12 | 0b10 | 1 << 9);
    tss->cs = SELECTOR_USR_CODE_SEG;
    tss->eip = init_task_entry;

    irq_leave_protection(state);

    // 模拟中断返回
    asm volatile(
        // 模拟中断返回，切换入第1个可运行应用进程
        // 不过这里并不直接进入到进程的入口，而是先设置好段寄存器，再跳过去
        "push %[ss]\n\t"     // SS
        "push %[esp]\n\t"    // ESP
        "push %[eflags]\n\t" // EFLAGS
        "push %[cs]\n\t"     // CS
        "push %[eip]\n\t"    // ip
        "iret\n\t" ::[ss] "r"(tss->ss),
        [esp] "r"(tss->esp), [eflags] "r"(tss->eflags),
        [cs] "r"(tss->cs), [eip] "r"(tss->eip));
}

/**
 * @brief 判断stack magic是否被覆盖 (栈溢出判断)
 */
bool is_stack_magic(task_t *task)
{
    irq_state_t state = irq_enter_protection();
    bool ret = strncmp(STACK_MAGIC, task->stack_magic, STACK_MAGIC_LEN) == 0;
    irq_leave_protection(state);
    return ret;
}

/**
 * @brief 创建一个进程
 *          1.分配堆栈空间
 *          2.创建进程页表并完成映射
 *          3.并没有加入就绪队列
 */
task_t *create_task(addr_t entry, const char *name, uint32_t priority, task_attr_t *attr)
{
    irq_state_t state = irq_enter_protection();
    task_t *task = task_pool_alloc();
    if (!task)
    {
        irq_leave_protection(state);
        return NULL;
    }
    task->entry = entry;
    if (attr)
    {
        task->attr.stack_size = align_up(attr->stack_size, MEM_PAGE_SIZE);
        task->attr.heap_size = align_up(attr->heap_size, MEM_PAGE_SIZE);
    }
    else
    {
        task->attr.heap_size = MEM_PAGE_SIZE;
        task->attr.stack_size = MEM_PAGE_SIZE;
    }
    strcpy(task->name, name);
    // 给进程分配栈空间
    ph_addr_t stack_base = mm_bitmap_alloc_pages(task->attr.stack_size / MEM_PAGE_SIZE);
    task->stack_base = stack_base;
    ph_addr_t stack_top = stack_base + task->attr.stack_size; // 栈顶地址
    // 写入栈结束标记
    task->stack_magic = stack_top - task->attr.stack_size;
    strncpy(task->stack_magic, STACK_MAGIC, STACK_MAGIC_LEN);
    // 初始化栈帧
    stack_top -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack_top;
    frame->esi = frame->edi = frame->ebx = frame->ebp = 0;
    frame->eip = entry;

    // 分配堆空间
    task->heap_base = mm_bitmap_alloc_pages((task->attr.heap_size + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE);

    task->esp = stack_top;
    task->ticks = task->priority = priority;
    task->pid = allocate_id(&task_manager.pid_pool);

    // 创建进程页表，并拷贝内核页表(记得写cr3，任务才能用自己的页表)
    task->page_table = (page_entry_t *)mm_bitmap_alloc_page();
    copy_kernel_pdt(task->page_table);

    // 分配pid
    task_manager.tasks[task->pid] = task;
    task->state = TASK_STATE_CREATED;
    task->list = NULL;
    irq_leave_protection(state);
    return task;
}

/**
 * @brief 创建一个内核进程，并且直接加入就绪队列
 */
task_t *create_kernel_task(addr_t entry, const char *name, uint32_t priority, task_attr_t *attr)
{
    task_t *task = create_task(entry, name, priority, attr);
    task_set_ready(task);
}
/**
 * @brief 回收任务
 */
void task_collect(task_t *task)
{
    irq_state_t state = irq_enter_protection();
    // 回收pid
    release_id(&task_manager.pid_pool, task->pid);
    irq_leave_protection(state);
}
extern void syscall_init(ph_addr_t);
/**
 * @brief 激活一个任务的必备环境
 */
void task_activate(task_t *task)
{
    irq_state_t state = irq_enter_protection();
    // 先切换页表再做检查
    if ((ph_addr_t)task->page_table != (ph_addr_t)read_cr3())
    {
        write_cr3((ph_addr_t)task->page_table);
    }
    if (!is_stack_magic(task))
    {
        dbg_error("stack magic err\r\n");
        irq_leave_protection(state);
        return;
    }

    // 如果是用户进程，激活还需要写入用户的内核栈
    // 因为从用户态，进入中断或者系统调用都需要压栈到该进程的内核栈
    task_manager.tss.esp0 = task->esp0;

    // 内核栈要挂在到 msr寄存器中，用于系统调用栈,经过考虑，和中断不冲突
    asm volatile(
        "movl $0x175,%%ecx\n\t"
        "movl %0,%%eax\n\t"
        "xor %%edx,%%edx\n\t"
        "wrmsr\n\t" ::"r"(task->esp0)
        : "eax", "ecx", "edx");
    irq_leave_protection(state);
    // syscall_init(task->esp0); //将当前要调度的任务 的 esp0写入到msr寄存器中，系统调用自动加载esp和ip
}

/**
 * @brief 任务管理器初始化
 */
void task_manager_init(void)
{
    task_pool_init();
    id_pool_init(TASK_PID_START, TASK_PID_END, &task_manager.pid_pool);
    list_init(&task_manager.ready_list);
    list_init(&task_manager.sleep_list);
    // 创建空闲进程
    task_manager.idle = create_task((ph_addr_t)idle_task, "idle", TASK_PRIORITY_DEFAULT, NULL);
    task_manager.idle->state = TASK_STATE_READY;
    // 创建first进程
    task_manager.init = create_init_task();

    set_cur_task(task_manager.init);
    task_manager.init->state = TASK_STATE_RUNNING;
    // 打印所有链表的地址，便于调试
    
}
extern void task_switch_to(task_t *next);
/**
 * @brief 进程切换，切换上下文
 */
void task_switch(task_t *next)
{
    irq_state_t state = irq_enter_protection();
    task_switch_to(next);
    irq_leave_protection(state);
}

void sys_sleep(uint32_t ms)
{
    task_t *task = cur_task();
    irq_state_t state = irq_enter_protection();
    task->sleep_ticks = (ms + OS_TICK_MS - 1) / OS_TICK_MS;
    if (task->sleep_ticks <= 0)
    {

        irq_leave_protection(state);
        return; // 不用睡
    }
    ASSERT(task->list == NULL);
    ASSERT(task->state == TASK_STATE_RUNNING);
    // 加入睡眠队列

    task_set_sleep(task);
    schedule();
    irq_leave_protection(state);
    return;
}
void sys_yield(void){
    task_t *task = cur_task();
     ASSERT(task->list == NULL);
    ASSERT(task->state == TASK_STATE_RUNNING);
    schedule();
}
/**
 * @brief 打印链表地址
 */
void task_list_debug(void){
     dbg_info("ready list addr:%x\r\n",&task_manager.ready_list);
     dbg_info("sleep list addr:%x\r\n",&task_manager.sleep_list);
}