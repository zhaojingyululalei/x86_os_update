
#include "task/task.h"
#include "mem/bitmap.h"
#include "string.h"
#include "printk.h"
#include "cpu_cfg.h"
#include "cpu_instr.h"
#include "task/sche.h"
#include "algrithm.h"
#include "irq/irq.h"
#include "errno.h"
#include "mem/page.h"
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
    task_attr_t attr;
    attr.stack_size = MEM_PAGE_SIZE;
    attr.heap_size = 4 * MEM_PAGE_SIZE;
    task_t *init = create_task(start_vm, "init", TASK_PRIORITY_DEFAULT, &attr);
    if (!init)
    {
        dbg_error("init process create fail\r\n");
    }
    // 然后给init进程的代码段，数据段等做映射
    pdt_set_entry(init->page_table, start_vm, start_ph, align_up(init_text_size, MEM_PAGE_SIZE), PDE_P | PDE_U | PDE_W);

    return init;
}
/**
 * @brief 创建或者fork或者clone一个任务后，记录其所有页
 */
static void record_all_pages_in_range(task_t *task)
{
    if (!task || !task->page_table) return;

    page_entry_t *pde_table = task->page_table;

    // 计算 PDE 范围（页目录索引）
    int start_pde_idx = (USR_ENTRY_BASE >> 22); // 0x80000000 >> 22
    int end_pde_idx = (USR_HEAP_BASE >> 22);    // 0x90000000 >> 22

    for (int pde_idx = start_pde_idx; pde_idx < end_pde_idx; pde_idx++)
    {
        if (!pde_table[pde_idx].present) continue;  // 跳过不存在的页目录项

        page_entry_t *pte_table = (page_entry_t *)(pde_table[pde_idx].phaddr << 12);
        if (!pte_table) continue;

        // 计算 PTE 范围（页表索引）
        int start_pte_idx = (pde_idx == start_pde_idx) ? ((USR_ENTRY_BASE >> 12) & 0x3FF) : 0;
        int end_pte_idx = (pde_idx == end_pde_idx - 1) ? ((USR_HEAP_BASE >> 12) & 0x3FF) : PAGE_TABLE_ENTRY_CNT;

        for (int pte_idx = start_pte_idx; pte_idx < end_pte_idx; pte_idx++)
        {
            if (!pte_table[pte_idx].present) continue;  // 跳过不存在的页表项

            vm_addr_t vmaddr = (pde_idx << 22) | (pte_idx << 12);
            ph_addr_t phaddr = pte_table[pte_idx].phaddr << 12;

            record_one_page(task, phaddr, vmaddr, PAGE_TYPE_ANON);
        }
    }
}
static void remove_all_pages_in_range(task_t *task)
{
    if (!task || !task->page_table) return;

    page_entry_t *pde_table = task->page_table;

    // 计算 PDE 范围（页目录索引）
    int start_pde_idx = (USR_ENTRY_BASE >> 22); // 0x80000000 >> 22
    int end_pde_idx = (USR_HEAP_BASE >> 22);    // 0x90000000 >> 22

    for (int pde_idx = start_pde_idx; pde_idx < end_pde_idx; pde_idx++)
    {
        if (!pde_table[pde_idx].present) continue;  // 跳过不存在的页目录项

        page_entry_t *pte_table = (page_entry_t *)(pde_table[pde_idx].phaddr << 12);
        if (!pte_table) continue;

        // 计算 PTE 范围（页表索引）
        int start_pte_idx = (pde_idx == start_pde_idx) ? ((USR_ENTRY_BASE >> 12) & 0x3FF) : 0;
        int end_pte_idx = (pde_idx == end_pde_idx - 1) ? ((USR_HEAP_BASE >> 12) & 0x3FF) : PAGE_TABLE_ENTRY_CNT;

        for (int pte_idx = start_pte_idx; pte_idx < end_pte_idx; pte_idx++)
        {
            if (!pte_table[pte_idx].present) continue;  // 跳过不存在的页表项

            vm_addr_t vmaddr = (pde_idx << 22) | (pte_idx << 12);
            ph_addr_t phaddr = pte_table[pte_idx].phaddr << 12;

            remove_one_page(task,  vmaddr);
        }
    }
}

static void task_record_pages(task_t *task)
{
    // 在这里记录所有二级页表占用的空间
    for (int i = USR_ENTRY_BASE >> 22; i < 1024; i++)
    {
        if (task->page_table[i].present)
        {
            ph_addr_t phaddr = task->page_table[i].phaddr << 12;
            record_one_page(task, phaddr, phaddr, PAGE_TYPE_ANON);
        }
    }
    // 记录页目录
    record_one_page(task, task->page_table, task->page_table, PAGE_TYPE_ANON);

    //记录代码段，数据段，bss段等用到的页
    record_all_pages_in_range(task);
    // 记录栈
    record_continue_pages(task, task->stack_base, task->attr.stack_size, PAGE_TYPE_ANON);
    // 记录任务的内核栈
    record_one_page(task,task->esp0-MEM_PAGE_SIZE,task->esp0-MEM_PAGE_SIZE,PAGE_TYPE_ANON);
    //记录signal栈
    record_one_page(task,task->signal_stack_base,USR_SIGNAL_STACK_TOP-MEM_PAGE_SIZE,PAGE_TYPE_ANON);
    // 记录堆
    record_continue_pages(task, task->heap_base, task->attr.heap_size, PAGE_TYPE_ANON);
}
static void task_collect_pages(task_t* task){
    

    //回收代码段，数据段，bss段等用到的页
    remove_all_pages_in_range(task);
    // 回收栈
    remove_pages(task,task->stack_base,task->attr.stack_size);
    // 回收任务的内核栈
    remove_one_page(task,task->esp0-MEM_PAGE_SIZE);
    //回收signal栈
    remove_one_page(task,USR_SIGNAL_STACK_TOP-MEM_PAGE_SIZE);
    // 回收堆
    remove_pages(task, task->heap_base, task->attr.heap_size);
    //释放所有二级页表空间
    for (int i = USR_ENTRY_BASE >> 22; i < 1024; i++)
    {
        if (task->page_table[i].present)
        {
            ph_addr_t phaddr = task->page_table[i].phaddr << 12;
            remove_one_page(task, phaddr);
        }
    }
    //回收页目录空间
    remove_one_page(task, task->page_table);
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
    
    //用户进程分配信号处理栈空间
    ph_addr_t signal_stack_base = mm_bitmap_alloc_pages(1);
    vm_addr_t signal_stack_vbase = USR_SIGNAL_STACK_TOP-MEM_PAGE_SIZE;
    ret = pdt_set_entry(task->page_table, signal_stack_vbase, signal_stack_base, MEM_PAGE_SIZE, PDE_U | PDE_W | PDE_P);
    task->signal_stack_base = signal_stack_base;
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

    //记录所有page_t为匿名页
    task_record_pages(task);
    task->usr_flag = true;
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
    task->pid = -1;
    task->ppid = -1;
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
    // 创建空闲进程,永不释放
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

/**
 * @brief 获取任务的errno
 */
int task_get_errno(void)
{
    return cur_task()->err_num;
}
/**
 * @brief 打印链表地址
 */
void task_list_debug(void)
{
    dbg_info("ready list addr:%x\r\n", &task_manager.ready_list);
    dbg_info("sleep list addr:%x\r\n", &task_manager.sleep_list);
}
/**
 * @brief 进程主动让出cpu
 */
void sys_yield(void)
{
    task_t *task = cur_task();
    schedule();
}
/**
 * @brief 进程进入睡眠队列
 */
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

/**
 * @brief 浅拷贝父页表 0x80000000~0xefff0000
 * @note pde的ignore位标记为PDE_COW 所有页表表项标记为只读(页目录表项不用，就正常拷贝)，用于触发写时复制
 */
static void copy_parent_pdt(task_t *child, task_t *parent)
{
    page_entry_t *parent_pdt = parent->page_table;
    page_entry_t *child_pdt = child->page_table;

    ASSERT(child->stack_base == parent->stack_base);
    ASSERT(USR_STACK_TOP - child->attr.stack_size == child->stack_base);
    // ASSERT(child->heap_base == parent->heap_base == USR_HEAP_BASE);

    for (int i = get_pdt_index(USR_ENTRY_BASE, 1); i <= get_pdt_index(child->stack_base, 1); i++)
    {
        page_entry_t *parent_pde = &parent_pdt[i];
        page_entry_t *child_pde = &child_pdt[i];

        if (!parent_pde->present)
        {
            // PDE 不存在，跳过
            continue;
        }

        // **PDE 直接拷贝，不修改任何内容**
        *child_pde = *parent_pde;

        // **获取父进程的二级页表（PT）**
        ph_addr_t parent_pt_base = parent_pde->phaddr << 12;
        page_entry_t *parent_pt = (page_entry_t *)parent_pt_base;

        // **子进程分配新的二级页表**
        ph_addr_t child_pt_base = mm_bitmap_alloc_page();
        page_entry_t *child_pt = (page_entry_t *)child_pt_base;

        // **拷贝 PTE**
        for (int j = 0; j < PAGE_TABLE_ENTRY_CNT; j++)
        {
            if (!parent_pt[j].present)
                continue;

            // 计算 PTE 负责的虚拟地址
            vm_addr_t vm_addr = ((i << 22) | (j << 12));

            if (vm_addr >= USR_ENTRY_BASE && vm_addr < child->stack_base)
            {
                // **只对 0x80000000 ~ 0xEFFE0000 范围的 PTE 进行 COW 处理**
                child_pt[j] = parent_pt[j];     // 直接拷贝 PTE
                child_pt[j].write = 0;          // 标记 PTE 为只读
                child_pt[j].ignored |= PDE_COW; // **在 PTE 的 ignored 位中标记 COW**
            }
            else
            {
                // **超出范围的 PTE 不拷贝，不做 COW 处理**
                ;
            }
        }

        // **更新子 PDE 指向新的 PT**
        child_pde->phaddr = child_pt_base >> 12;
    }
}

/**
 * @brief 复制父进程,创建子进程
 */
extern void sys_handler_exit(void);
int sys_fork(void)
{
    int ret;
    irq_state_t state = irq_enter_protection();
    task_t *parent = cur_task();
    // 因为是复制父进程，子进程的入口地址不重要
    task_t *child = task_pool_alloc();
    if (!child)
    {
        irq_leave_protection(state);
        dbg_error("task pool err:create child task fail\r\n");
        return -1;
    }
    child->attr.heap_size = parent->attr.heap_size;
    child->attr.stack_size = parent->attr.stack_size;
    strcpy(child->name, "child");

    child->ticks = child->priority = parent->priority;
    // 创建子进程页表
    child->page_table = (page_entry_t *)mm_bitmap_alloc_page();
    copy_kernel_pdt(child->page_table);

    child->stack_base = parent->stack_base; // 虚拟地址
    child->heap_base = parent->heap_base;   // 虚拟地址
    // 栈空间另行处理，其余空间全部遵循写时复制 cow机制
    // 浅拷贝父页表，从0x80000000到child.stack_base  包含所有代码和数据包括堆空间
    copy_parent_pdt(child, parent);

    // 为子进程分配栈空间并拷贝父进程栈空间
    ph_addr_t c_stack_base = mm_bitmap_alloc_pages(child->attr.stack_size / MEM_PAGE_SIZE);
    ph_addr_t c_stack_top = c_stack_base + child->attr.stack_size;
    child->stack_magic = (char *)c_stack_base;
    strncpy(child->stack_magic, STACK_MAGIC, STACK_MAGIC_LEN);

    vm_addr_t p_stack_base = parent->stack_base;
    // 映射
    ret = pdt_set_entry(child->page_table, p_stack_base, c_stack_base, child->attr.stack_size, PDE_P | PDE_W | PDE_U);
    ASSERT(ret >= 0);
    // ph_addr_t test_addr = vm_to_ph(child->page_table,0xeffef000);
    // 拷贝
    ph_addr_t p_ph_stack_base = vm_to_ph(parent->page_table, p_stack_base); // 获取父进程栈空间对应的物理地址
    memcpy(c_stack_base, p_ph_stack_base, child->attr.stack_size);

    // 给子进程分配内核栈空间
    child->esp0 = mm_bitmap_alloc_page() + MEM_PAGE_SIZE;
    
    // 获取父进程内核 sysenter_frmae栈帧,获取返回到用户态的 esp和eip
    sysenter_frame_t *sysenter_frame = (sysenter_frame_t *)(parent->esp0 - sizeof(sysenter_frame_t));
    vm_addr_t ret_esp = sysenter_frame->esp;
    vm_addr_t ret_eip = sys_handler_exit;
    vm_addr_t ret_ebp = sysenter_frame->ebp;
    // 处理子进程栈帧 task_frame_t  （子进程是通过task_switch进行切换的，而不是直接jmp to usr mode,栈帧必须处理好）
    vm_addr_t c_vm_stack_top = ret_esp - sizeof(task_frame_t);
    ph_addr_t c_ph_frame = vm_to_ph(child->page_table, c_vm_stack_top);
    task_frame_t *c_frame = (task_frame_t *)c_ph_frame;
    c_frame->ebp = ret_ebp;
    c_frame->eip = ret_eip;

    child->esp = c_vm_stack_top;

    // test_addr = vm_to_ph(child->page_table,0xeffef000);
    child->pid = allocate_id(&task_manager.pid_pool);
    child->ppid = parent->pid;
    // 将子进程加入父进程的child_list中
    list_insert_last(&parent->child_list, &child->child_node);
    // 将子进程加入到taskmanger的tasks中
    task_manager.tasks[child->pid] = child;
    // 将子进程加入就绪队列
    task_set_ready(child);

    //记录子进程页信息
    task_record_pages(child);
    irq_leave_protection(state);
    sys_yield();
    return child->pid;
}

int sys_getpid(void)
{
    return cur_task()->pid;
}
int sys_getppid(void)
{
    return cur_task()->ppid;
}

void sys_exit(int status)
{
    task_t *cur = cur_task();
    task_t *parent = parent_task();
    ASSERT(cur->state == TASK_STATE_RUNNING);
    ASSERT(!cur->list);
    ASSERT(cur != task_manager.init); // init进程永不退出
    irq_state_t state = irq_enter_protection();
    cur->status = status;
    cur->state = TASK_STATE_ZOMBIE;
    // 查看该进程是否存在子进程，如果存在子进程，交给父进程处理，自己退出
    if (list_count(&cur->child_list))
    {
        list_join(&cur->child_list, &parent->child_list);
    }
    // 唤醒父进程，可以回收了
    if (parent->list == &task_manager.wait_list && parent->state == TASK_STATE_WAITING)
    {
        task_set_ready(parent);
    }
    sys_yield();
    irq_leave_protection(state);
}
/**
 * @brief 回收一个进程的所有资源
 */
int task_collect(task_t *task)
{
    task_collect_pages(task);

    // task_manager相关资源
    task_manager.tasks[task->pid] = NULL;
    release_id(&task_manager.pid_pool,task->pid);
    task_pool_free(task);
}

/**
 * @brief 阻塞回收所有子进程
 */
int sys_wait(int *status)
{
    int ret_pid = -1;
    task_t *parent = cur_task();

    while (true)
    {
        irq_state_t state = irq_enter_protection();
        if (!list_count(&parent->child_list))
        {
            // 一个子进程都没有，返回-1
            parent->err_num = ECHILD;
            return -1;
        }
        // 遍历child_list，看看有没有可以回收的子进程
        list_node_t *cur_node = parent->child_list.first;
        while (cur_node)
        {
            list_node_t *next_node = cur_node->next;
            task_t *cur_child = list_node_parent(cur_node, task_t, child_node);
            if (cur_child->state == TASK_STATE_ZOMBIE)
            {
                ASSERT(!cur_child->list); // 肯定不在任何状态队列中
                ret_pid = cur_child->pid;
                *status = cur_child->status;
                list_remove(&parent->child_list, cur_node); // 从子进程队列中移除
                task_collect(cur_child);                    // 释放该进程所有资源
                irq_leave_protection(state);
                return ret_pid; // 本函数唯一可以退出的出口
            }

            cur_node = next_node;
        }
        // 没有可以回收的子进程
        task_set_wait(parent); // 加入task_manager的等待队列
        sys_yield();           // 调度走，以后不要再执行了，等待某个子进程退出后唤醒它
        // 唤醒之后，接着从遍历child_list链表开始执行
        irq_leave_protection(state);
    }
}