#include "mem/memory.h"
#include "mem/pdt.h"
#include "cpu_instr.h"
#include "cpu_cfg.h"
#include "apci.h"
#include "boot_info.h"
#include "mem/buddy_system.h"
#include "mem/kmalloc.h"
#define MEM_KERNEL_MAP_CNT  5
extern boot_info_t* os_info;
extern uint8_t s_ro; // 只读区起始地址
extern uint8_t s_rw; // 读写区起始地址
extern uint8_t e_rw; // 读写区结束地址
#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)
/**
 * @brief 创建内核页表，为整个内存做恒等映射
 */
static int create_kernel_pdt(ph_addr_t page_table)
{
    ph_addr_t ph_s_ro = &s_ro;
    ph_addr_t ph_s_rw = &s_rw;
    ph_addr_t ph_e_rw = &e_rw;
    //做虚拟物理地址映射
    struct {
        ph_addr_t start;
        ph_addr_t size;
        uint16_t attr;
    }map[MEM_KERNEL_MAP_CNT]={
        //0~kernel代码段
        [0] = {.start = 0,.size = ph_s_ro,.attr = PDE_P|PDE_W},
        //整个kernel代码段
        [1] = {.start = ph_s_ro,.size = ph_s_rw-ph_s_ro,.attr = PDE_P},
        //整个kernel数据段
        [2] = {.start = ph_s_rw,.size = ph_e_rw-ph_s_rw,.attr = PDE_P|PDE_W},
        //剩余物理内存映射
        [3] = {.start = ph_e_rw,.size = MEM_TOTAL_SIZE- ph_e_rw,.attr = PDE_P|PDE_W},
        //映射apic 包括local_apic 和 io_apic
        [4] = {.start = os_info->cpu_info.io_apic_addr,.size = VM_MEM_TOTAL_SIZE-os_info->cpu_info.io_apic_addr,.attr=PDE_P|PDE_W}
    };
    for (int i = 0; i < MEM_KERNEL_MAP_CNT; i++)
    {
        pdt_set_entry(page_table,map[i].start,map[i].start,map[i].size,map[i].attr);
    }
    // 之前设置过PSE，现在清0
    // uint32_t cr4 = read_cr4();
    // write_cr4(cr4 &~ CR4_PSE); //PSE清0
    // 设置页表地址
    write_cr3((ph_addr_t)page_table);

    // 开启分页机制
    write_cr0(read_cr0() | CR0_PG);
    //debug_print_page_table();
    
}

void memory_init(void)
{
    ph_addr_t page_table = page_table_init();
    // 创建内核页表，恒等映射
    create_kernel_pdt(page_table);
    bucket_mempool_init();//bucket_t内存池初始化,不管是内核kmalloc还是用户的malloc，只要获取桶结构，就依靠这个初始化
    //为kmalloc开辟空间，用于内核动态分配内存,并且初始化km_allocator
    km_allocator_init();

    page_manager_init();//用page_t结构记录物理页信息
    //vmalloc空间管理初始化
}

