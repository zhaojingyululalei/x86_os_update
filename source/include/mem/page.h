#ifndef __PAGE_H
#define __PAGE_H

#include "types.h"
#include "tools/list.h"
#include "task/task.h"
#include "pdt.h"
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
typedef struct _page_t{
    page_type_t type;
    ph_addr_t addr;
    task_t* tasks[TASK_LIMIT_CNT];//拥有该页的进程
    int refs; //引用计数，拥有该页的进程数
    
}page_t;
#endif