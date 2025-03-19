#include "cpu_cfg.h"
#include "mem/pdt.h"
#include "printk.h"
#include "algrithm.h"
#include "mem/bitmap.h"
#include "string.h"
#include "task/task.h"
#include "task/sche.h"
#include "mem/page.h"
// 页目录  32位系统，每个页表表项32位 4字节  总共有4096/(32/8) = 1024项

page_entry_t g_page_table[PAGE_TABLE_ENTRY_CNT] __attribute__((aligned(4096)));

/**
 * @brief 传入虚拟地址，获取某级页表的索引
 * @param addr 虚拟地址
 * @param level 几级页表 只有1和2
 */
 int get_pdt_index(vm_addr_t addr, int level)
{
    switch (level)
    {
    case 1:
        return addr >> 22;
    case 2:
        return (addr >> 12) & 0x3FF;
    default:
        dbg_error("page dir table level %d is not exsist!\r\n");
        return -1;
    }
}

/**
 * @brief 传入页表首地址 和 idx 获取对应的页表项
 * @note  可传入页目录首地址或者页表首地址
 */
 page_entry_t *get_page_entry(ph_addr_t pdt_base, int idx)
{

    ASSERT(idx >= 0 && idx < PAGE_TABLE_ENTRY_CNT);
    page_entry_t *pdt = (page_entry_t *)pdt_base;
    return pdt + idx;
}

/**
 * @brief 就只安装一页
 */
static int pdt_set_one_entry(page_entry_t* page_table, vm_addr_t vm_addr, ph_addr_t ph_addr, uint16_t attr)
{
    ASSERT(vm_addr % MEM_PAGE_SIZE == 0);
    // 根据虚拟地址，获取一级页表索引和二级页表索引值
    int pdt_idx = get_pdt_index(vm_addr, 1);
    int ptt_idx = get_pdt_index(vm_addr, 2);
    // 获取pde
    page_entry_t *pde = get_page_entry(page_table, pdt_idx);
    ph_addr_t ptt_base;
    if (!pde->present)
    { // 如果该pde没有被设置过
        // 分配一个物理页，作为ptt(二级页表)
        ptt_base = mm_bitmap_alloc_page();
        pde->val |= PDE_ATTR_DEFAULT; // 所有pde的属性都设置为默认值
        pde->phaddr = ptt_base >> 12;
    }
    else
    {
        // 如果pde是存在的，那么二级页表肯定就分配过了
        ptt_base = pde->phaddr << 12;
    }

    page_entry_t *pte = get_page_entry(ptt_base, ptt_idx); // 获取二级页表对应表项
    if (pte->present)
    {
        dbg_warning("the map between vmaddr:0x%x and phaddr:0x%x was built before\r\n", vm_addr, ph_addr);
        return -1;
    }
    pte->val |= attr;
    pte->phaddr = ph_addr >> 12;
    return 0;
}
/**
 * @brief 传入虚拟地址和物理地址，建立映射关系
 */
int pdt_set_entry(page_entry_t* page_table,vm_addr_t vm_addr, ph_addr_t ph_addr, uint32_t size, uint16_t attr)
{
    if (size == 0 && vm_addr == 0 && ph_addr == 0 && attr == 0)
    {
        return 0; // 用不着建立
    }
    ASSERT(vm_addr % MEM_PAGE_SIZE == 0 && ph_addr % MEM_PAGE_SIZE == 0);
    int ret;
    size = align_up(size, MEM_PAGE_SIZE);
    int loop = size / MEM_PAGE_SIZE;
    for (int i = 0; i < loop; i++)
    {
        ret = pdt_set_one_entry(page_table,vm_addr + i * MEM_PAGE_SIZE, ph_addr + i * MEM_PAGE_SIZE, attr);
        if (ret < 0)
        {
            dbg_error("pdt mapping between vm:0x%x and ph:0x%x fail\r\n", vm_addr + i * MEM_PAGE_SIZE, ph_addr + i * MEM_PAGE_SIZE);
            return -1;
        }
    }
    return 0;
}

ph_addr_t page_table_init(void)
{
    // 清0
    for (int i = 0; i < PAGE_TABLE_ENTRY_CNT; i++)
    {
        g_page_table[i].val = 0;
    }
    return (ph_addr_t)g_page_table;
}
/**
 * @brief 获取内核页表
 */
page_entry_t* get_global_page_table(void){
    return g_page_table;
}
/**
 * @brief 打印页表内容
 * @param pdt_base 一级页表（页目录表）的基地址
 */
void debug_print_page_table()
{
    ph_addr_t pdt_base = (ph_addr_t)g_page_table;
    dbg_info("Printing page table at base: 0x%x\r\n", pdt_base);

    // 获取一级页表（页目录表）
    page_entry_t *pde_table = (page_entry_t *)pdt_base;

    // 遍历一级页表
    for (int pde_idx = 0; pde_idx < PAGE_TABLE_ENTRY_CNT; pde_idx++)
    {
        if (pde_table[pde_idx].present)
        {
            dbg_info("PDE Entry Idx %d:\r\n", pde_idx);
            dbg_info("PDE_VAL:0x%x\r\n", pde_table[pde_idx].val);

            // 获取二级页表基地址
            ph_addr_t pte_base = pde_table[pde_idx].phaddr << 12;
            page_entry_t *pte_table = (page_entry_t *)pte_base;

            // 遍历二级页表
            for (int pte_idx = 0; pte_idx < PAGE_TABLE_ENTRY_CNT; pte_idx++)
            {
                if (pte_table[pte_idx].present)
                {
                    dbg_info("  PTE Entry %d:\r\n", pte_idx);
                    dbg_info("  PTE VAL: 0x%x\r\n", pte_table[pte_idx].val);
                }
            }
        }
    }
}

/**
 * @brief 拷贝内核页表
 */
int copy_kernel_pdt(page_entry_t *task_pdt)
{
    //地址权限都相同，直接拷贝即可
    page_entry_t *kernel_pdt = g_page_table;
    for (int i = 0; i < 1024; i++)
    {
        if (kernel_pdt[i].present)
        {

            task_pdt[i].val = kernel_pdt[i].val;
        }
    }
    return 0;
}

/**
 * @brief 传入虚拟地址，获取对应的物理地址
 */
ph_addr_t vm_to_ph(page_entry_t* page_table,vm_addr_t vmaddr)
{
    // 根据虚拟地址，获取一级页表索引和二级页表索引值
    int pdt_idx = get_pdt_index(vmaddr, 1);
    int ptt_idx = get_pdt_index(vmaddr, 2);

    page_entry_t *pde = get_page_entry(page_table, pdt_idx);
    ph_addr_t ptt_base;
    if(!pde->present){
        dbg_error("the vmaddr:0x%x not map to phaddr in page_table:0x%x\r\n",vmaddr,page_table);
        return NULL;
    }
    ptt_base = pde->phaddr<<12;

    page_entry_t* pte = get_page_entry(ptt_base,ptt_idx);
    if(pte->present){
        return (pte->phaddr << 12) | (vmaddr & 0xfff);
    }
    else{
        dbg_error("the vmaddr:0x%x not map to phaddr in page_table:0x%x\r\n",vmaddr,page_table);
        return NULL;
    }
}
/**
 * @brief 直接通过虚拟地址，获得对应的页目录表项
 */
page_entry_t* get_pde(page_entry_t* page_table, vm_addr_t vm){
    int pde_idx = get_pdt_index(vm,1);
    if(pde_idx < 0){
        dbg_error("get pde err\r\n");
    }
    return page_table+pde_idx;
}
/**
 * @brief 直接通过虚拟地址，获取对应的页表表项
 */
page_entry_t* get_pte(page_entry_t* page_table,vm_addr_t vm){
    page_entry_t* pde = get_pde(page_table,vm);
    ph_addr_t ptt = pde->phaddr <<12;

    int pte_idx = get_pdt_index(vm,2);
    page_entry_t* pte = get_page_entry(ptt,pte_idx);
    if(pte){
        return pte;
    }
    dbg_error("get pte fail\r\n");
    return NULL;
}

/**
 * @brief 页异常处理写时复制
 */
int page_fault_cow(vm_addr_t PF_vm){
    task_t* cur = cur_task();
    //获取错误页表表项
    page_entry_t* fault_pte = get_pte(cur->page_table,PF_vm);
    if(fault_pte->ignored == PDE_COW && fault_pte->write ==0 && fault_pte->user==1){
        ph_addr_t src = fault_pte->phaddr<<12; //父进程的数据代码或者堆空间
        ph_addr_t dest = mm_bitmap_alloc_page(); //子进程分配空间，重新拷贝，并替换源页表记录的物理地址

        remove_one_page(cur,align_down(PF_vm,MEM_PAGE_SIZE));
        record_one_page(cur,dest,align_down(PF_vm,MEM_PAGE_SIZE),PAGE_TYPE_ANON);

        fault_pte->phaddr = dest>>12; //替换位新地址
        fault_pte->write = 1;
        fault_pte->ignored = 0;

        //开始拷贝父空间的内容
        memcpy(dest,src,MEM_PAGE_SIZE);
        return 0;
    }

    return -1;
    
}

