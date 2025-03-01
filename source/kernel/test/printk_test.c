#include "printk.h"

void printk_test(void){
    dbg_info("hello i am printk %04d,%c\r\n",26,'a');
    dbg_error("what is wrong\r\n");
    ASSERT(1==2);
}