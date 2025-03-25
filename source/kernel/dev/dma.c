#include "dev/dma.h"
#include "mem/bitmap.h"
#include "mem/page.h"
#include "cpu_cfg.h"
#include "mem/kmalloc.h"
#include "task/task.h"
#include "errno.h"
#include "algrithm.h"
#include "dev/dma.h"
#include "printk.h"
/**
 * 映射虚拟内存区域到分散的物理内存列表（自动合并连续物理页）
 * @param scatterlist 已初始化的分散列表结构体
 * @param vmbase 虚拟内存基地址
 * @param len 要映射的长度(字节)
 * @return 成功返回0，失败返回错误码
 */
int scatter_list_map(scatter_list_t *scatterlist, vm_addr_t vmbase, int len)
{
    // 参数检查
    if (!scatterlist || len <= 0)
    {
        return -EINVAL;
    }

    vm_addr_t current_vm = vmbase;
    vm_addr_t end_vm = vmbase + len;
    task_t *task = cur_task();
    page_entry_t *page_table = task->page_table;

    

    while (current_vm < end_vm)
    {
        // 计算当前页的剩余空间
        vm_addr_t page_end = (current_vm & (~0xfff))+MEM_PAGE_SIZE;
        if (page_end > end_vm)
        {
            page_end = end_vm;
        }
        size_t seg_len = page_end - current_vm;

        // 转换虚拟地址到物理地址
        ph_addr_t ph_addr = vm_to_ph(page_table, current_vm);
        if (!ph_addr)
        {
            return -EFAULT;
        }
        scatter_node_t* last_node = list_node_parent(scatterlist->list.last,scatter_node_t,node);
        // 检查是否可以合并到上一个节点
        if (last_node && (last_node->phstart + last_node->len == ph_addr))
        {
            // 物理地址连续，扩展上一个节点
            last_node->len += seg_len;
        }
        else
        {
            // 需要创建新节点
            scatter_node_t *node = kmalloc(sizeof(scatter_node_t));
            if (!node)
            {
                return -ENOMEM;
            }

            node->vstart = current_vm;
            node->phstart = ph_addr;
            node->len = seg_len;

            // 添加到链表尾部
            list_insert_last(&scatterlist->list, &node->node);
        }

        current_vm = page_end;
    }

    return 0;
}
void scatter_list_destory(scatter_list_t* scatterlist){
    list_node_t* cur = scatterlist->list.first;
    while (cur)
    {
        list_node_t* next = cur->next;
        scatter_node_t* scatter = list_node_parent(cur,scatter_node_t,node);
        list_remove(&scatterlist->list,cur);
        kfree(scatter);
        cur = cur->next;
    }
    
}
/**
 * @brief 1.分配连续的物理页 2.首地址4KB对齐
 */
ph_addr_t dma_alloc_coherent(size_t size){
    size = align_up(size,MEM_PAGE_SIZE);
    task_t* cur = cur_task();
    //分配
    ph_addr_t phaddr = mm_bitmap_alloc_pages(size/MEM_PAGE_SIZE);
    //不用映射，物理地址全覆盖
    //记录
    record_continue_pages(cur,phaddr,size,PAGE_TYPE_KERNEL);
    return phaddr;

}

void dma_free_coherent(ph_addr_t ptr,size_t size){
    size = align_up(size,MEM_PAGE_SIZE);
    task_t* cur = cur_task();
    mm_bitmap_free_pages(ptr,size/MEM_PAGE_SIZE);
    remove_pages(cur,ptr,size);
}
void scatter_list_debug_print(scatter_list_t *scatterlist)
{
    list_node_t* cur = scatterlist->list.first;
    while (cur)
    {
        scatter_node_t* sn = list_node_parent(cur,scatter_node_t,node);
        dbg_info("vmaddr:0x%x,phaddr:0x%x,size:0x%x\r\n",sn->vstart,sn->phstart,sn->len);
        cur = cur->next;
    }
    
}