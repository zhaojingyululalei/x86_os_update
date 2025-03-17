#include "mem/buddy_system.h"
#include "mem/bitmap.h"
#include "cpu_cfg.h"
#include "printk.h"
#include "string.h"
#include "math.h"
static list_t bucket_free_list; // 用于分配桶的链表

/*******************桶的相关操作*********** */
/**
 * @brief 补充bucket_t结构：bucket_T结构不够用了，再申请内存，往bucket_free_list中添加
 */
static void bucket_supplement(void)
{
    // 分配一页，全部用于bucket_t结构
    bucket_t *buckets = (bucket_t *)mm_bitmap_alloc_page();
    int bucket_cnt = MEM_PAGE_SIZE / sizeof(bucket_t);
    for (int i = 0; i < bucket_cnt; i++)
    {
        list_insert_last(&bucket_free_list, &buckets[i].lnode);
    }
}
/**
 * @brief 分配一个桶结构
 */
bucket_t *bucket_alloc()
{
    while (bucket_free_list.count <= 0)
    {
        bucket_supplement();
        ASSERT(bucket_free_list.count > 0);
    }
    list_node_t *node = list_remove_first(&bucket_free_list);
    bucket_t *bucket = list_node_parent(node, bucket_t, lnode);
    return bucket;
}
/**
 * @brief 释放一个bucket_t结构
 */
void bucket_free(bucket_t *bucket)
{
    memset(bucket, 0, sizeof(bucket_t));
    list_insert_last(&bucket_free_list, &bucket->lnode);
}
void bucket_mempool_init(void)
{
    list_init(&bucket_free_list);
}

/******************buddy_system 相关***************** */

static int bucket_compare(const void *a, const void *b)
{
    int a_capacity = ((bucket_t *)a)->capacity;
    int b_capacity = ((bucket_t *)b)->capacity;

    return a_capacity - b_capacity;
}

static rb_node_t *bucket_get_node(const void *data)
{
    return &((bucket_t *)data)->rbnode;
}

static bucket_t *node_get_bucket(rb_node_t *node)
{
    bucket_t *s = rb_node_parent(node, bucket_t, rbnode);
    return s;
}
/**capacity 作为key,查找对应的桶 */
static int find_by_capacity(const void *key, const void *data)
{
    int capacity = (int)key;
    bucket_t *s = (bucket_t *)data;

    return capacity - s->capacity;
}
static void bucket_print(bucket_t* bucket){
    dbg_info("bucket:0x%x,size=0x%x\r\n",bucket->addr,bucket->capacity);
}
void buddy_system_init(buddy_system_t *buddy, addr_t start, size_t size)
{
    if (!buddy)
        return; // 检查指针是否有效

    buddy->start = start;
    buddy->size = size;

    // 初始化红黑树
    rb_tree_init(&buddy->tree, bucket_compare, bucket_get_node, node_get_bucket);

    // 创建初始桶
    bucket_t *initial_bucket = bucket_alloc();
    initial_bucket->addr = start;
    initial_bucket->capacity = size;
    initial_bucket->state = BUCKET_STATE_FREE;

    // 将初始桶插入红黑树
    rb_tree_insert(&buddy->tree, initial_bucket);
}

/**
 * @brief 伙伴系统分配 size大小的内存
 */


void *buddy_system_alloc(buddy_system_t *buddy, size_t size) {
    if (!buddy || size == 0)
        return NULL;

    size += sizeof(buddy_alloc_head_t);  // 预留头部信息

    // 找到第一个大于等于 size 的桶
    rb_node_t *node = rb_tree_find_first_greater_or_equal(&buddy->tree, (void *)size, find_by_capacity);
    if (node == buddy->tree.nil)
        return NULL;  // 没有合适的桶

    bucket_t *bucket = node_get_bucket(node); //找到这个桶

    

    // **分裂逻辑**
    while (bucket->capacity / 2 >= size) {
        bucket_t *new_bucket = bucket_alloc();
        new_bucket->capacity = bucket->capacity / 2;
        new_bucket->addr = bucket->addr + new_bucket->capacity;
        new_bucket->state = BUCKET_STATE_FREE;

        bucket->capacity /= 2;
        
        // 伙伴关系
        bucket->buddy = new_bucket;
        new_bucket->buddy = bucket;

        // 插入新的 `new_bucket` 到红黑树
        rb_tree_insert(&buddy->tree, new_bucket);
        rb_tree_inorder(&buddy->tree,&buddy->tree.root,bucket_print);
    }
    // 分配该 `bucket`，并从红黑树中移除
    rb_tree_remove(&buddy->tree, bucket);
    bucket->state = BUCKET_STATE_USED;
    rb_tree_inorder(&buddy->tree,&buddy->tree.root,bucket_print);

    // 记录桶信息
    buddy_alloc_head_t *header = (buddy_alloc_head_t *)bucket->addr;
    header->bucket = bucket;

    return (void *)(bucket->addr + sizeof(buddy_alloc_head_t));
}


/**
 * @brief 伙伴系统释放addr的内存
 */
void buddy_system_free(buddy_system_t *buddy, void *addr) {
    if (!buddy || !addr)
        return;

    // 找到对应的 `bucket`
    buddy_alloc_head_t *header = (buddy_alloc_head_t *)((addr_t)addr - sizeof(buddy_alloc_head_t));
    bucket_t *bucket = header->bucket;
    bucket->state = BUCKET_STATE_FREE;

    // **合并逻辑**
    while (bucket->buddy && bucket->buddy->state == BUCKET_STATE_FREE) {
        bucket_t *buddy_bucket = bucket->buddy;

        // 释放 `buddy_bucket`
        rb_tree_remove(&buddy->tree, buddy_bucket);
        bucket_free(buddy_bucket);

        // **合并当前 `bucket`**
        if (bucket->addr < buddy_bucket->addr) {
            bucket->capacity *= 2;  // `bucket` 在前，合并后容量翻倍
        } else {
            buddy_bucket->capacity *= 2;
            bucket = buddy_bucket;  // 让 `bucket` 指向前面的块
        }

        bucket->buddy = NULL;  // 伙伴已经被合并
    }

    // **插入合并后的 `bucket` 回红黑树**
    rb_tree_insert(&buddy->tree, bucket);
}
