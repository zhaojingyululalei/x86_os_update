#include "mem/memory.h"
#include "mem/pdt.h"
#include "cpu_instr.h"
#include "cpu_cfg.h"
#define MEM_KERNEL_MAP_CNT  5
extern uint8_t s_ro; // 只读区起始地址
extern uint8_t s_rw; // 读写区起始地址
extern uint8_t e_rw; // 读写区结束地址
#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)
static int create_kernel_pdt(ph_addr_t page_table)
{
    ph_addr_t ph_s_ro = &s_ro;
    ph_addr_t ph_s_rw = &s_rw;
    ph_addr_t ph_e_rw = &e_rw;
    struct {
        ph_addr_t start;
        ph_addr_t size;
        uint16_t attr;
    }map[MEM_KERNEL_MAP_CNT]={
        [0] = {.start = 0,.size = ph_s_ro,.attr = PDE_P|PDE_W},
        [1] = {.start = ph_s_ro,.size = ph_s_rw-ph_s_ro,.attr = PDE_P},
        [2] = {.start = ph_s_rw,.size = ph_e_rw-ph_s_rw,.attr = PDE_P|PDE_W},
        [3] = {.start = ph_e_rw,.size = MEM_TOTAL_SIZE- ph_e_rw,.attr = PDE_P|PDE_W}
    };
    for (int i = 0; i < MEM_KERNEL_MAP_CNT; i++)
    {
        pdt_set_entry(map[i].start,map[i].start,map[i].size,map[i].attr);
    }
    // 之前设置过PSE，现在清0
    // uint32_t cr4 = read_cr4();
    // write_cr4(cr4 &~ CR4_PSE); //PSE清0
    // 设置页表地址
    write_cr3((ph_addr_t)page_table);

    // 开启分页机制
    write_cr0(read_cr0() | CR0_PG);
    
}
void memory_init(void)
{
    ph_addr_t page_table = page_table_init();
    // 创建内核页表，恒等映射
    create_kernel_pdt(page_table);
    //debug_print_page_table();
}