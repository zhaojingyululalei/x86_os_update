#ifndef __BUDDY_SYSTEM_H
#define __BUDDY_SYSTEM_H
#include "tools/rb_tree.h"
#include "tools/list.h"
#include "types.h"

typedef struct _bucket_t{
    enum{
        BUCKET_STATE_USED, //该桶有东西，不能合并
        BUCKET_STATE_FREE, //该桶空闲，可以合并
    }state;
    addr_t addr; //该桶代表的内存首地址
    size_t capacity; //代表的内存块大小

    list_node_t lnode;//用于空闲链表分配bucket_t结构
    rb_node_t rbnode; //用于插入红黑树，按照capacity排序插入
}bucket_t;




bucket_t* bucket_alloc();
void bucket_free(bucket_t* bucket);
void bucket_mempool_init(void);


typedef struct {
    addr_t start; //管理的起始地址
    size_t size; //管理的大小
    
    rb_tree_t tree; //红黑树，存放所有空闲可供分配的桶
}buddy_system_t;

void buddy_system_init(buddy_system_t* buddy,addr_t start,size_t size);
void *buddy_system_alloc(buddy_system_t *buddy, size_t size);
void buddy_system_free(buddy_system_t *buddy, void *addr) ;



/**
 * @brief 例如在0xA0000000分配了10字节，在他前面会有一块空间记录该指针所对应的桶信息
 * 因此在用户分配空间时，例如用户想分配10字节的空间，实际上底层会分配10+sizeof(buddy_alloc_head_t)空间，返回时跳过buddy_alloc_head_t，返回指针
 */
#pragma pack(1)
typedef struct _buddy_alloc_head_t
{
    bucket_t* bucket; //这个内存属于哪个节点
}buddy_alloc_head_t;
#pragma pack()


//例如这是个4KB的区域，那么链表上全部都是4KB大小的空闲桶(可供分配)
typedef struct _buddy_zone_t{
    size_t capacity;
    list_t buckets_list; //所有属于该区域的桶，大小全部相同
    list_node_t lnode; //用于分配zone结构
    rb_node_t rbnode; //往树上挂
}buddy_zone_t;

buddy_zone_t *zone_alloc(void);
void zone_free(buddy_zone_t* zone);

void buddy_dynamic_system_init(buddy_system_t *buddy, addr_t start, size_t size);
void *buddy_system_dynamic_alloc(buddy_system_t *buddy, size_t size);
void buddy_system_dynamic_free(buddy_system_t *buddy, void *addr);
#endif