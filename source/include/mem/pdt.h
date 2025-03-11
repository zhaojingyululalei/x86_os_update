#ifndef __PDT_H
#define __PDT_H

#include "types.h"
#define PAGE_TABLE_ENTRY_CNT (MEM_PAGE_SIZE / (OS_32 / 8))
#define PDE_P (1 << 0)
#define PDE_W (1 << 1)
#define PDE_U (1 << 2)
#define PDE_PWT (1 << 3)
#define PDE_PCD (1 << 4)
#define PDE_A (1 << 6)
#define PDE_PS (1 << 7)
#define PDE_PAT PDE_PS
#define PDE_G (1 << 8)

#define PDE_COW 2   //该页做了写时复制标记，写在pde的ignore域


// PDE属性默认全部设置为 可读可写 且用户可访问   PTE属性根据系统设置进行配置
#define PDE_ATTR_DEFAULT (PDE_P | PDE_W | PDE_U)
// 页表表项
#pragma pack(1)
typedef union page_entry_t
{
    struct
    {
        uint32_t present : 1;  // 在内存中
        uint32_t write : 1;    // 0 只读 1 可读可写
        uint32_t user : 1;     // 1 所有人 0 超级用户 DPL < 3
        uint32_t pwt : 1;      // page write through 1 直写模式，0 回写模式
        uint32_t pcd : 1;      // page cache disable 禁止该页缓冲
        uint32_t accessed : 1; // 被访问过，用于统计使用频率
        uint32_t dirty : 1;    // 脏页，表示该页缓冲被写过
        uint32_t pat_ps : 1;   // pat or ps
        uint32_t global : 1;   // 全局，所有进程都用到了，该页不刷新缓冲
        uint32_t ignored : 3;  // 该安排的都安排了，送给操作系统吧
        uint32_t phaddr : 20;   // 页索引
    };
    uint32_t val;
} page_entry_t;
#pragma pack(0)


ph_addr_t page_table_init(void);
 int get_pdt_index(vm_addr_t addr, int level);
 page_entry_t *get_page_entry(ph_addr_t pdt_base, int idx);
 page_entry_t* get_pde(page_entry_t* page_table, vm_addr_t vm);
 page_entry_t* get_pte(page_entry_t* page_table,vm_addr_t vm);
ph_addr_t vm_to_ph(page_entry_t* page_table,vm_addr_t vmaddr);
int pdt_set_entry(page_entry_t* page_table, vm_addr_t vm_addr,ph_addr_t ph_addr,uint32_t size,uint16_t attr);
int copy_kernel_pdt(page_entry_t *task_pdt);
void debug_print_page_table();


int page_fault_cow(vm_addr_t PF_vm);
#endif