#ifndef __PAGE_H
#define __PAGE_H

#include "types.h"
#include "tools/list.h"
#include "pdt.h"
#include "task/task.h"
#include "tools/rb_tree.h"
typedef enum _page_type_t{
    PAGE_TYPE_NONE,
    PAGE_TYPE_FILE, //文件页  读文件
    PAGE_TYPE_ANON, //匿名页  brk 栈等
    PAGE_TYPE_CHACHE,//缓存页  近期数据缓存，防止频繁访问磁盘
    PAGE_TYPE_KERNEL//内核页  内核独享
}page_type_t;
/**
 * @brief 物理页管理
 */
typedef struct {
    vm_addr_t vmaddr;
    task_t* task;
    list_node_t node;
}page_owner_t;
typedef struct _page_t{
    page_type_t type;
    ph_addr_t phaddr;
    list_t onwers_list;
    int refs; //引用计数，拥有该页的进程数
    rb_node_t rbnode; //插入红黑树用的
    list_node_t lnode; //用于分配page_t
}page_t;
void page_manager_init(void);
int record_one_page(task_t *task, ph_addr_t phaddr, vm_addr_t vmaddr, page_type_t type);
int record_continue_pages(task_t* task,vm_addr_t vmstart,size_t size,page_type_t type);
int remove_one_page(task_t* task,vm_addr_t vm_addr);
int remove_pages(task_t* task,vm_addr_t vmstart,size_t size);
#endif