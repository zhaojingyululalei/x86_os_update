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
#define TASK_INTERVAL_TIME 0xFFFFFF
boot_info_t* os_info;
extern void task_switch(task_t* task);
static void test(void)
{
    int ret = 0;
    ret ++;
    
    task_test();
    while (1)
    {
        for (int i = 0; i < TASK_INTERVAL_TIME; i++)
        {
            
        }
        dbg_info("i am init task\r\n");
    }
    
}
/**
 * @brief 系统init进程
 */
void first_task(void){
    //set_cur_task(task_manager.init);
    task_manager.init->state = TASK_STATE_RUNNING;
    task_activate(task_manager.init);
    irq_enable_global();
    test();
}
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
    //暂时用位图分配内存过度，分配页表，分配进程PCB都用位图分
    //页和 进程模块都弄好了，再写伙伴系统
    mm_bitmap_init(boot_info);
    memory_init();
    task_manager_init();
    task_switch(task_manager.init);
    
    //切换到init进程去执行
    ASSERT(1==2);//再也执行不到这里了
}