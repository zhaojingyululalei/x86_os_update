
#include "task/task.h"
#include "task/sche.h"
#include "printk.h"
#include "irq/irq.h"
#define TASK_INTERVAL_TIME 0xFFFFFF
void task2(void){
    irq_enable_global();
    int b = 0;
    while (1)
    {
        b++;
        //sys_sleep(1000);
        dbg_info("i am task2,b=%d\r\n",b);
        //sys_yield();
    }
    
    
}
void task1(void){
    irq_enable_global();
    int a = 0;
    while (1)
    {
        a++;
        //sys_sleep(1000);
        dbg_info("i am task1,a=%d\r\n",a);
        //sys_yield();
        
    }
    
    
}
void task3(void){
    irq_enable_global();
    int a = 0;
    while (1)
    {
        a++;
        //sys_sleep(1000);
        dbg_info("i am task3,a=%d\r\n",a);
        //sys_yield();
        
    }
    
    
}
void task4(void){
    irq_enable_global();
    int a = 0;
    while (1)
    {
        a++;
        //sys_sleep(1000);
        dbg_info("i am task4,a=%d\r\n",a);
        //sys_yield();
    }
    
    
}
void task5(void){
    irq_enable_global();
    int a = 0;
    while (1)
    {
        a++;
        //sys_sleep(1000);
        dbg_info("i am task5,a=%d\r\n",a);
        //sys_yield();
    }
    
    
}
void task_test(void){
    create_kernel_task((ph_addr_t)task1,"task one",TASK_PRIORITY_DEFAULT,NULL);
    create_kernel_task((ph_addr_t)task2,"task two",TASK_PRIORITY_DEFAULT,NULL);
    create_kernel_task((ph_addr_t)task3,"task three",TASK_PRIORITY_DEFAULT,NULL);
    create_kernel_task((ph_addr_t)task4,"task four",TASK_PRIORITY_DEFAULT,NULL);
    create_kernel_task((ph_addr_t)task5,"task five",TASK_PRIORITY_DEFAULT,NULL);
}