#include "mem/page.h"

#include "mem/bitmap.h"
#include "algrithm.h"
#include "cpu_cfg.h"
#include "printk.h"
#include "irq/irq.h"
static page_t* page_buffer;
static uint32_t page_arr_len;
static list_t page_free_list; //page_t空闲链表，用于分配page_t
void page_manager_init(void){
    uint32_t page_cnt = calc_bitmap_pages(); //管理多少页
    //page_tbuffer占多大内存
    uint32_t page_buffer_len_byte = align_up(page_cnt* sizeof(page_t),MEM_PAGE_SIZE);
    //给page_t buffer分配内存
    page_buffer =  (page_t*)mm_bitmap_alloc_pages(page_buffer_len_byte/MEM_PAGE_SIZE);
    //page_t数组长度
    page_arr_len = page_cnt;

    list_init(&page_free_list);
    //全部放入空闲链表等待分配
    for (uint32_t i = 0; i < page_cnt; i++)
    {
        list_insert_last(&page_free_list,&page_buffer[i].lnode);
    }
    
    dbg_info("page_T free list count %d\r\n",list_count(&page_free_list));
}

page_t* page_t_alloc(){
    list_node_t* node = list_remove_first(&page_free_list);
    page_t* ret = list_node_parent(node,page_t,lnode);
    return ret;
}

void page_t_free(page_t* page){
    memset(page,0,sizeof(page_t));
    list_insert_last(&page_free_list,&page->lnode);
}

/**
 * @brief 创建一个页，谁创建的，创建的什么页
 */
int create_one_page(task_t* task,ph_addr_t phaddr,vm_addr_t vmaddr,page_type_t type){
    page_t* page = page_t_alloc();
    page->type = type;
    page->phaddr = phaddr;

    //留着，先写kmalloc吧，要不然总得建立内存池太麻烦
    //kmalloc弄好后，在这里把kmalloc管理的区域记为内核页
    //page->vmaddr = vmaddr;
    list_insert_last(&page->task_list,&task->page_node);
    page->refs++;
}