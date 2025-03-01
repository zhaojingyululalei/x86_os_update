#include "boot_info.h"
#include "string.h"
#include "stdarg.h"
#include "dev/serial.h"
/*测试函数*/
#include "kernel_test.h"
static void test(void)
{
    int ret = 0;

    
    printk_test();
}
void kernel_init(boot_info_t *boot_info)
{
    serial_init();
    test();
}