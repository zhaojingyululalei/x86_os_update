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
#include "mem/page.h"
#include "dev/pci.h"


extern void ide_init(void);
extern void fs_init(void);
extern void kbd_init(void);
extern  void devfs_init(void);
extern void tty_init(void);
boot_info_t *os_info;
static void test(void)
{
    dev_show_all();
    inode_test();
    // devfs_test();
    //buffer_test();
    //tty_test();
    //path_test();
    
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
    kbd_init();
    // 暂时用位图分配内存过度，分配页表，分配进程PCB都用位图分
    // 页和 进程模块都弄好了，再写伙伴系统
    mm_bitmap_init(boot_info);
    memory_init();
    pci_init();
    task_manager_init();
    devfs_init();
    tty_init();
    irq_enable_global();
    ide_init(); //用到了中断
    
    fs_init();//用到了ide

    test();
    jmp_to_usr_mode();
    

    while (1)
    {
        ;
    }
    
    // 切换到init进程去执行
    ASSERT(1 == 2); // 再也执行不到这里了
}