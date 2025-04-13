#include "task/signal.h"
#include "irq/irq.h"
#include "string.h"
#include "task/task.h"
#include "time/time.h"
#include "errno.h"
#include "task/sche.h"
extern void exception_handler_signal(void);
void do_handler_signal(exception_frame_t *frame)
{
    task_t *cur = cur_task();
    memcpy(frame, &cur->frame, sizeof(exception_frame_t));
}

void signal_init(void)
{
    // 注册0x40中断

    uint32_t handler_addr = (irq_handler_t)exception_handler_signal;
    gate_desc_t *desc = idt_get_entry(IRQ_SIGNAL);
    desc->offset_15_0 = handler_addr & 0xFFFF;
    desc->offset_31_16 = (handler_addr >> 16) & 0xFFFF;
    desc->DPL = GATE_DPL3;
    desc->p = 1;
    desc->param = 0;
    desc->selector = SELECTOR_KERNEL_CODE_SEG;
    desc->type = GATE_INT_TYPE_IDT;
    return;
}
/**
 * @brief 注册信号处理函数
 */
int sys_signal(int signum, signal_handler handler)
{
    if (signum <= 0 || signum >= SIGNAL_MAX_CNT)
        return -1;

    if (signum == SIGKILL || signum == SIGSTOP || signum == SIGCONT)
    {
        return -1; // 不允许覆盖
    }
    task_t *cur = cur_task();
    cur->s_handler[signum] = handler;
    return 0;
}
/**
 * @brief 获取旧的mask，保存新的mask
 */
int sys_sigpromask(int how, const sigset_t *set, sigset_t *old)
{
    task_t *cur = cur_task();

    if (old)
    {
        old->bitmap = cur->s_mask.bitmap;
    }

    if (set)
    {
        switch (how)
        {
        case SIG_BLOCK:
            cur->s_mask.bitmap |= set->bitmap;
            break;
        case SIG_UNBLOCK:
            cur->s_mask.bitmap &= ~set->bitmap;
            break;
        case SIG_SETMASK:
            cur->s_mask.bitmap = set->bitmap;
        default:
            return -1;
        }
    }

    return 0;
}
/**
 * @brief 向当前进程发送信号
 */
int sys_raise(int signum)
{
    if (signum <= 0 || signum >= SIGNAL_MAX_CNT)
        return -1;

    task_t *cur = cur_task();
    cur->s_pending.bitmap |= (1 << signum);
    return 0;
}
/**
 * @brief 向其他进程发送信号
 */
int sys_kill(int pid, int signum)
{
    if (signum <= 0 || signum >= SIGNAL_MAX_CNT)
        return -1;
    if(signum == SIGCONT){
        task_manager.tasks[pid]->stop = false;
    }
    task_t *target = task_manager.tasks[pid];
    if (!target)
        return -1;

    target->s_pending.bitmap |= (1 << signum);
    return 0;
}

/**
 * @brief 获取未决信号集
 */

int sys_sigpending(sigset_t *set)
{
    if (!set)
        return -1;

    task_t *cur = cur_task();
    set->bitmap = cur->s_pending.bitmap;
    return 0;
}

/**
 * @brief 暂停程序，直到收到一个信号，并且处理完
 */
int sys_pause(void)
{
    task_t *cur = cur_task();
    cur->paused = true; // 设置进程为暂停状态

    // 进入等待状态，直到有信号被处理
    while (cur->paused)
    {

        sys_yield();
    }
    return 0; // 当信号处理完毕，返回
}
/**
 * @brief 闹钟
 */
uint32_t sys_alarm(uint32_t seconds)
{
    task_t *cur = cur_task();

    // 如果 seconds 为0，表示取消定时器
    if (seconds == 0)
    {
        cur->alarm_time = 0;
        return 0;
    }

    uint32_t prev_time = cur->alarm_time;
    cur->alarm_time = sys_time(NULL) + seconds; // 设置定时器，单位为秒
    return prev_time;
}
void check_alarm_and_send_signal(task_t *cur)
{
    if (cur->alarm_time && cur->alarm_time <= sys_time(NULL))
    {
        // 定时器到期，发送 SIGALRM 信号
        sys_raise(SIGALRM);
        cur->alarm_time = 0; // 清除定时器
    }
}
void check_stop_and_wakeup(void){
    //遍历stop_list，看看哪些任务收到了continu信号
    list_node_t* cur = task_manager.stop_list.first;
    while (cur)
    {
        list_node_t* next= cur->next;
        task_t* task = list_node_parent(cur,task_t,node);
        if(task->stop==false){
            list_remove(&task_manager.stop_list,cur);
            task_set_ready(task);
        }
        cur = next;
    }
    
}
static void jmp_to_usr(exception_frame_t *frame, signal_handler handler, int signum)
{

    // 保存异常栈帧于task结构，以后恢复
    task_t *cur = cur_task();
    memcpy(&cur->frame, frame, sizeof(exception_frame_t));

    tss_t *tss = &task_manager.tss;
    tss->ss = SELECTOR_USR_DATA_SEG;
    // 找到用户栈栈顶
    tss->esp = frame->esp3 - 8;       // 留个参数和返回值的空隙
    int *arg = (int *)(tss->esp + 4); // 设置个参数
    *arg = signum;
    tss->eflags = (0 << 12 | 0b10 | 1 << 9);
    tss->cs = SELECTOR_USR_CODE_SEG;
    tss->eip = handler;

    // 模拟中断返回
    asm volatile(
        // 模拟中断返回，切换入第1个可运行应用进程
        // 不过这里并不直接进入到进程的入口，而是先设置好段寄存器，再跳过去
        "mov %[ss],%%ds\n\t"
        "mov %[ss],%%es\n\t"
        "mov %[ss],%%fs\n\t"
        "mov %[ss],%%gs\n\t"
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
 * @brief 时钟中断处检查是否有可以执行的信号并执行处理函数
 */

void check_and_handle_signal(task_t *cur, exception_frame_t *frame)
{

    task_t *task = cur;

    for (int i = 1; i < SIGNAL_MAX_CNT; ++i)
    {
        uint32_t mask = (1 << i);
        // 如果pending位置1，并且还没有屏蔽
        if ((task->s_pending.bitmap & mask) && !(task->s_mask.bitmap & mask))
        {
            cur->paused = false; // 只要收到信号，就解除暂停
            //stop和continue信号，没有处理函数，改个标志位直接return
            if (i == SIGSTOP)
            {
                task->stop = true; // 设置为暂停状态
                task_set_stop(cur);
                return;
            }
            else if (i == SIGCONT)
            {
                task->stop = false; // 恢复执行
                return;
            }
            signal_handler handler = task->s_handler[i];
            if (handler)
            {
                // 调用已有的 handle_signal()

                if (frame->cs != SELECTOR_USR_CODE_SEG)
                {
                    return;
                }
                // 清除 pending
                task->s_pending.bitmap &= ~mask;
                jmp_to_usr(frame, handler, i);
                return;
            }
            else
            {
                // 默认处理：终止
                sys_exit(-1); // 进程退出
            }
        }
    }
}
