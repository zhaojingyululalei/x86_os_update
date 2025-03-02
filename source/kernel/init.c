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
static void test(void)
{
    int ret = 0;
    
    
    rtc_test();
}
/**
 * @brief 内核初始化
 */
void kernel_init(boot_info_t *boot_info)
{
    serial_init();
    cpu_init();
    clock_init();
    time_init();
    rtc_init();

    irq_enable_global();
    test();
}