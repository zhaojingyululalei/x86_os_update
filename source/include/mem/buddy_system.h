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


typedef struct {
    addr_t start; //管理的起始地址
    size_t size; //管理的大小
    
    rb_tree_t tree; //红黑树，存放所有空闲可供分配的桶
}buddy_system_t;




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


//静态的伙伴系统内存池
typedef struct {
    buddy_system_t sys;

    
    void* (*alloc)(buddy_system_t* sys,size_t size);
    void (*free)(buddy_system_t* sys,void* ptr);
}buddy_mmpool_fix_t; 

//动态的伙伴系统内存池
typedef struct {
    buddy_system_t sys;

    
    void* (*alloc)(buddy_system_t* sys,size_t size);
    void (*free)(buddy_system_t* sys,void* ptr);
    void (*sbrk)(buddy_system_t* sys,void* base,size_t capacity);
}buddy_mmpool_dyn_t; 

void buddy_system_enable(void);
void buddy_mmpool_fix_init(buddy_mmpool_fix_t* mmpool,void* base,size_t capacity);
void buddy_mmpool_dyn_init(buddy_mmpool_dyn_t* mmpool,void* base,size_t capacity);

#endif