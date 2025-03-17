#ifndef __MEM_ADDR_CFG_H
#define __MEM_ADDR_CFG_H


#define MEM_PAGE_SIZE 4096
#define MEM_TOTAL_SIZE  (128*1024*1024)
#define LOAD_KERNEL_SIZE    (2*1024*1024) //从磁盘上加载2MB大小的内核，以后不够了再加
#define VM_MEM_TOTAL_SIZE   (4*1024*1024*1024) //虚拟内存大小4GB
#define KM_MEM_TOTAL_SIZE   (8*1024*1024) //内核kmalloc区域大小，由bitmap分配

#define KERNEL_SIZE (32*1024*1024)

#define BOOTLOADER_SEG 0x0

/*物理地址设计*/
#define BOOT_START_ADDR_REL  0x7c00
#define LOAD_START_ADDR_REL  0x8000 
#define KERNEL_START_ADDR_REL   0x100000
#define STACK_KERNEL_TOP_ADDR   0x7000
#define STACK_SYSCALL_TOP_ADDR  0x2000

/*虚拟地址设计*/
#define USR_STACK_TOP   0xEFFF0000 //用户栈顶地址
#define USR_ENTRY_BASE  0x80000000  //用户态起始虚拟地址
#define USR_HEAP_BASE   0x90000000 //用户堆空间地址

#define VMALLOC_BASE    0xA0000000 //vmalloc管理的范围
#define VMALLOC_END     0xB0000000
#endif
