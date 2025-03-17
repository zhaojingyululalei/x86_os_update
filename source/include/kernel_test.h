#ifndef __KERNEL_TEST_H
#define __KERNEL_TEST_H
extern void string_test(void);
extern void stdarg_test(void);
extern void serial_test(void);
extern void printk_test(void);
extern void time_test(void);
extern void rtc_test(void);
extern void bitmap_test(void);
extern void link_script_test(void);
extern void APCI_test(void);
extern void id_pool_test(void);
extern void task_test(void);
extern int sys_calc_add(int a,int b,int c,int d,int e);
extern void ipc_test(void);
extern void buddy_system_test(void);
#endif