#include "mem/malloc.h"
#include "task/task.h"
#include "mem/page.h"
#include "mem/bitmap.h"
#include "irq/irq.h"
void sbrk(void){
    task_t * cur = cur_task();
    //分配
    ph_addr_t phaddr = mm_bitmap_alloc_page();
    vm_addr_t vmaddr = USR_HEAP_BASE+cur->attr.heap_size;
    //映射
    pdt_set_entry(cur->page_table,vmaddr,phaddr,MEM_PAGE_SIZE,PDE_U | PDE_W | PDE_P);
    //记录
    record_one_page(cur,phaddr,vmaddr,PAGE_TYPE_ANON);
    //修改
    cur->attr.heap_size +=MEM_PAGE_SIZE;//动态扩容

    //加入伙伴系统进行管理
    cur->m_pool.sbrk(&cur->m_pool.sys,vmaddr,MEM_PAGE_SIZE);
}
void* sys_malloc(size_t size){
    task_t * cur = cur_task();
    void* ret;
    irq_state_t state = irq_enter_protection();
   
    ret = cur->m_pool.alloc(&cur->m_pool.sys,size);
    if(ret == NULL){
        sbrk();
        ret = cur->m_pool.alloc(&cur->m_pool.sys,size);
    }
     irq_leave_protection(state);
    return ret;
}

void sys_free(void* ptr){
    task_t * cur = cur_task();
    cur->m_pool.free(&cur->m_pool.sys,ptr);
}

