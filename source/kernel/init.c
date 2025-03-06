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

boot_info_t* os_info;
static void test(void)
{
    int ret = 0;
    
    
    APCI_test();
    while (1)
    {
        ;
    }
    
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
    


    irq_enable_global();
    test();
}