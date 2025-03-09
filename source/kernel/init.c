#include "boot_info.h"
#include "string.h"
#include "stdarg.h"
#include "dev/serial.h"
#include "cpu.h"
#include "time/clock.h"
#include "irq/irq.h"
#include "time/time.h"
#include "time/rtc.h"
/*测试函数*/
#include "kernel_test.h"
#include "mem/bitmap.h"
#include "mem/memory.h"
#include "task/task.h"
#include "task/sche.h"
#include "printk.h"
#include "cpu_cfg.h"
#define TASK_INTERVAL_TIME 0xFFFFFF
boot_info_t *os_info;
extern void task_switch(task_t *task);
static void test(void)
{
    int ret = 0;
    ret++;

    task_test();
}
// /**
//  * @brief 跳转到init进程，(将first task变成init进程)
//  */
// static void jmp_to_first_task(void)
// {
//     irq_state_t state = irq_enter_protection();
//     // task_manager保存了公共tss结构
//     tss_t *tss = &task_manager.tss;
//     tss->ss = SELECTOR_USR_DATA_SEG;
//     tss->esp = task_manager.init->esp;
//     tss->eflags = (0 << 12 | 0b10 | 1 << 9);
//     tss->cs = SELECTOR_USR_CODE_SEG;
//     tss->eip = task_manager.init->entry;
//     task_activate(task_manager.init);
//     //当前就绪队列正在执行的任务必须是first
//     ASSERT(task_manager.ready_list.first == &task_manager.first->node);
//     list_remove_first(&task_manager.ready_list);//将first任务移除，替换为init进程
//     list_insert_first(&task_manager.ready_list,&task_manager.init->node);
//     irq_leave_protection(state);
//     // 也可以使用类似boot跳loader中的函数指针跳转
//     // 这里用jmp是因为后续需要使用内联汇编添加其它代码
//     asm volatile(
//         // 模拟中断返回，切换入第1个可运行应用进程
//         // 不过这里并不直接进入到进程的入口，而是先设置好段寄存器，再跳过去
//         "push %[ss]\n\t"     // SS
//         "push %[esp]\n\t"    // ESP
//         "push %[eflags]\n\t" // EFLAGS
//         "push %[cs]\n\t"     // CS
//         "push %[eip]\n\t"    // ip
//         "iret\n\t" ::[ss] "r"(tss->ss),
//         [esp] "r"(tss->esp), [eflags] "r"(tss->eflags),
//         [cs] "r"(tss->cs), [eip] "r"(tss->eip));
// }
// /**
//  * @brief 系统first进程
//  */
// void first_task(void)
// {
//     // set_cur_task(task_manager.init);
//     task_manager.first->state = TASK_STATE_RUNNING;
//     task_activate(task_manager.first);
//     test();
//     irq_enable_global();

//     jmp_to_first_task();
//     while (1)
//     {
//         for (int i = 0; i < TASK_INTERVAL_TIME; i++)
//         {
//         }
//         dbg_info("i am first task\r\n");
//     }
// }
/**
 * @brief 内核初始化
 */
void kernel_init(boot_info_t *boot_info)
{
    os_info = boot_info;
    serial_init();
    cpu_init();
    clock_init();
    time_init();
    rtc_init();
    // 暂时用位图分配内存过度，分配页表，分配进程PCB都用位图分
    // 页和 进程模块都弄好了，再写伙伴系统
    mm_bitmap_init(boot_info);
    memory_init();
    task_manager_init();
    irq_enable_global();
    test();
    jmp_to_usr_mode();
    //task_switch(task_manager.first); // 汇编中会把当前任务设置为first，否则当前任务为空

    while (1)
    {
        ;
    }
    
    // 切换到init进程去执行
    ASSERT(1 == 2); // 再也执行不到这里了
}