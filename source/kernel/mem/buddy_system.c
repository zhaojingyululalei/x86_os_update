#include "mem/buddy_system.h"
#include "mem/bitmap.h"
#include "cpu_cfg.h"
#include "printk.h"
#include "string.h"
#include "math.h"
static list_t bucket_free_list; // 用于分配桶的链表
static list_t zone_free_list;   // 用于分配buddy_zone的链表
/**初始化容量数组，存储2^0~2^31值 */
static size_t capacity_array[OS_32];
static void capacity_array_init(void){
    for (int i = 0; i < OS_32; i++)
    {
        capacity_array[i] = power(2,i);
    }
}
// 二分查找第一个 >= capacity 的值
static size_t find_greater_or_equal_capacity(size_t capacity) {
    int left = 0, right = OS_32 - 1;
    while (left < right) {
        int mid = left + (right - left) / 2;
        if (capacity_array[mid] >= capacity) {
            right = mid;  // 可能是解，收缩右边界
        } else {
            left = mid + 1;  // 继续向右搜索
        }
    }
    return capacity_array[left] >= capacity ? capacity_array[left] : 0; // 0 表示未找到
}
// 二分查找第一个 <= capacity 的值
static size_t find_less_or_equal_capacity(size_t capacity) {
    int left = 0, right = OS_32 - 1;
    while (left < right) {
        int mid = left + (right - left + 1) / 2; // 向上取整，防止死循环
        if (capacity_array[mid] <= capacity) {
            left = mid;  // 可能是解，继续向右搜索
        } else {
            right = mid - 1;  // mid 太大，收缩右边界
        }
    }
    return capacity_array[left] <= capacity ? capacity_array[left] : 0; // 0 表示未找到
}
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
static void zone_supplement(void)
{
    // 分配一页，全部用于bucket_t结构
    buddy_zone_t *zones = (buddy_zone_t *)mm_bitmap_alloc_page();
    int zone_cnt = MEM_PAGE_SIZE / sizeof(buddy_zone_t);
    for (int i = 0; i < zone_cnt; i++)
    {
        list_insert_last(&zone_free_list, &zones[i].lnode);
    }
}
/**
 * @brief 分配一个桶结构
 */
static bucket_t *bucket_alloc()
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
static buddy_zone_t *zone_alloc(void)
{
    while (zone_free_list.count <= 0)
    {
        zone_supplement();
        ASSERT(zone_free_list.count > 0);
    }
    list_node_t *node = list_remove_first(&zone_free_list);
    buddy_zone_t *zone = list_node_parent(node, buddy_zone_t, lnode);
    list_init(&zone->buckets_list);
    return zone;
}
/**
 * @brief 释放一个bucket_t结构
 */
static void bucket_free(bucket_t *bucket)
{
    memset(bucket, 0, sizeof(bucket_t));
    list_insert_last(&bucket_free_list, &bucket->lnode);
}
static void zone_free(buddy_zone_t *zone)
{
    memset(zone, 0, sizeof(buddy_zone_t));
    list_insert_last(&zone_free_list, &zone->lnode);
}
static void bucket_mempool_init(void)
{
    list_init(&bucket_free_list);
    
}
static void zone_mempool_init(void){
    list_init(&zone_free_list);
}

/******************buddy_system 相关***************** */

static int bucket_compare(const void *a, const void *b)
{
    int a_capacity = ((bucket_t *)a)->capacity;
    int b_capacity = ((bucket_t *)b)->capacity;

    return a_capacity - b_capacity;
}
static int zone_compare(const void *a, const void *b)
{
    int a_capacity = ((buddy_zone_t *)a)->capacity;
    int b_capacity = ((buddy_zone_t *)b)->capacity;

    return a_capacity - b_capacity;
}

static rb_node_t *bucket_get_node(const void *data)
{
    return &((bucket_t *)data)->rbnode;
}
static rb_node_t *zone_get_node(const void *data)
{
    return &((buddy_zone_t *)data)->rbnode;
}

static bucket_t *node_get_bucket(rb_node_t *node)
{
    bucket_t *s = rb_node_parent(node, bucket_t, rbnode);
    return s;
}
static buddy_zone_t *node_get_zone(rb_node_t *node)
{
    buddy_zone_t *s = rb_node_parent(node, buddy_zone_t, rbnode);
    return s;
}

/**capacity 作为key,查找对应的桶 */
static int bucket_find_by_capacity(const void *key, const void *data)
{
    int capacity = (int)key;
    bucket_t *s = (bucket_t *)data;

    return capacity - s->capacity;
}
static int zone_find_by_capacity(const void *key, const void *data)
{
    int capacity = (int)key;
    buddy_zone_t *s = (buddy_zone_t *)data;

    return capacity - s->capacity;
}

static void bucket_print(bucket_t *bucket)
{
    dbg_info("------bucket:0x%x,size=0x%x\r\n", bucket->addr, bucket->capacity);
}
static void zone_print(buddy_zone_t *zone)
{
    dbg_info("---zone capacity:0x%x has %d buckets as like:\r\n", zone->capacity, zone->buckets_list.count);
    list_node_t *cur = zone->buckets_list.first;
    while (cur)
    {
        bucket_t *b = list_node_parent(cur, bucket_t, lnode);
        bucket_print(b);
        cur = cur->next;
    }
}
/**
 * @brief 获取某个桶的兄弟桶
 */
static bucket_t* fix_get_buddy(buddy_system_t* buddy,bucket_t* bucket){
    rb_node_t *node =  rb_tree_find_by(&buddy->tree,bucket->capacity,bucket_find_by_capacity);
    bucket_t* buddy_bucket;
    if(node==buddy->tree.nil){
        buddy_bucket = NULL;
    }else{
        buddy_bucket = rb_node_parent(node,bucket_t,rbnode);
        ASSERT(buddy_bucket->state == BUCKET_STATE_FREE); //存放在树上的桶，全部是空闲的
    }
    return buddy_bucket;
    
}
static bucket_t* dynamic_get_buddy(buddy_system_t* buddy,bucket_t* bucket){
    size_t capacity = bucket->capacity;
    rb_node_t *zone_node = rb_tree_find_by(&buddy->tree, capacity, zone_find_by_capacity);
    buddy_zone_t *zone;
    if (zone_node == buddy->tree.nil)
    {
        // 如果没找到zone,说明兄弟还在被使用，还没释放
        return NULL;
    }
    else
    {
        // 如果找到了zone，
        zone = rb_node_parent(zone_node, buddy_zone_t, rbnode);
        list_node_t* cur = zone->buckets_list.first;
        while (cur)
        {
            bucket_t* b = list_node_parent(cur,bucket_t,lnode);
            if(b->addr+b->capacity == bucket->addr || bucket->addr + bucket->capacity == b->addr){
                return b;
            }
            cur = cur->next;
        }
        
    }
    return NULL; //兄弟节点还在使用，
}
static void buddy_system_init(buddy_system_t *buddy, addr_t start, size_t size)
{
    if (!buddy)
        return; // 检查指针是否有效

    size = find_less_or_equal_capacity(size); //确保size是2^n
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

static void *buddy_system_alloc(buddy_system_t *buddy, size_t size)
{
    if (!buddy || size == 0)
        return NULL;

    size += sizeof(buddy_alloc_head_t); // 预留头部信息

    // 找到第一个大于等于 size 的桶
    rb_node_t *node = rb_tree_find_first_greater_or_equal(&buddy->tree, (void *)size, bucket_find_by_capacity);
    if (node == buddy->tree.nil)
        return NULL; // 没有合适的桶

    bucket_t *bucket = node_get_bucket(node); // 找到这个桶
    // 分配该 `bucket`，并从红黑树中移除
    rb_tree_remove(&buddy->tree, bucket);
    bucket->state = BUCKET_STATE_USED;
    // **分裂逻辑**  偶数分裂
    while (bucket->capacity / 2 >= size && (bucket->capacity / 2) % 2 == 0)
    {
        bucket_t *new_bucket = bucket_alloc();
        new_bucket->capacity = bucket->capacity / 2;
        new_bucket->addr = bucket->addr + new_bucket->capacity;
        new_bucket->state = BUCKET_STATE_FREE;

        bucket->capacity /= 2;

        

        // 插入新的 `new_bucket` 到红黑树
        rb_tree_insert(&buddy->tree, new_bucket);
    }
    
    rb_tree_inorder(&buddy->tree, &buddy->tree.root, bucket_print);

    // 记录桶信息
    buddy_alloc_head_t *header = (buddy_alloc_head_t *)bucket->addr;
    header->bucket = bucket;

    return (void *)(bucket->addr + sizeof(buddy_alloc_head_t));
}

/**
 * @brief 伙伴系统释放addr的内存
 */
static void buddy_system_free(buddy_system_t *buddy, void *addr)
{
    if (!buddy || !addr)
        return;

    // 找到对应的 `bucket`
    buddy_alloc_head_t *header = (buddy_alloc_head_t *)((addr_t)addr - sizeof(buddy_alloc_head_t));
    bucket_t *bucket = header->bucket;
    bucket->state = BUCKET_STATE_FREE;

    bucket_t *buddy_bucket = fix_get_buddy(buddy,bucket);
    // **合并逻辑**
    while (buddy_bucket && buddy_bucket->state == BUCKET_STATE_FREE)
    {
        

        // 释放 `buddy_bucket`
        rb_tree_remove(&buddy->tree, buddy_bucket);
        

        // **合并当前 `bucket`**
        if (bucket->addr < buddy_bucket->addr)
        {
            bucket->capacity *= 2; // `bucket` 在前，合并后容量翻倍
        }
        else
        {
            bucket->capacity *= 2;
            bucket->addr = buddy_bucket->addr; // 让 `bucket` 指向前面的块
        }
        bucket_free(buddy_bucket);
        buddy_bucket = fix_get_buddy(buddy,bucket);
    }

    // **插入合并后的 `bucket` 回红黑树**
    rb_tree_insert(&buddy->tree, bucket);
    rb_tree_inorder(&buddy->tree, &buddy->tree.root, bucket_print);
}

static void buddy_dynamic_system_init(buddy_system_t *buddy, addr_t start, size_t size)
{
    if (!buddy)
        return; // 检查指针是否有效

    size = find_less_or_equal_capacity(size);//确保size是2^n
    buddy->start = start;
    buddy->size = size;

    // 初始化红黑树
    rb_tree_init(&buddy->tree, zone_compare, zone_get_node, node_get_zone);

    //创建初始zone
    buddy_zone_t* initial_zone  = zone_alloc();
    initial_zone->capacity = size;

    // 创建初始桶
    bucket_t *initial_bucket = bucket_alloc();
    initial_bucket->addr = start;
    initial_bucket->capacity = size;
    initial_bucket->state = BUCKET_STATE_FREE;

    //将初始桶加入初始zone
    list_insert_last(&initial_zone->buckets_list,&initial_bucket->lnode);
    // 将初始zone插入红黑树
    rb_tree_insert(&buddy->tree, initial_zone);
}
/**
 * @brief 找一个zone，有就返回，没有就创建一个，加入红黑树并返回
 */
static buddy_zone_t *find_zone_by_capacity(buddy_system_t *buddy, size_t capacity)
{
    rb_node_t *zone_node = rb_tree_find_by(&buddy->tree, capacity, zone_find_by_capacity);
    buddy_zone_t *new_zone;
    if (zone_node == buddy->tree.nil)
    {
        // 如果没找到zone,就创建一个zone
        new_zone = zone_alloc(); // 创建
        new_zone->capacity = capacity;
        rb_tree_insert(&buddy->tree, new_zone); // 插入红黑树
    }
    else
    {
        // 如果找到了zone，
        new_zone = rb_node_parent(zone_node, buddy_zone_t, rbnode);
    }
    return new_zone;
}
/**支持sbrk的分配 */
static void *buddy_system_dynamic_alloc(buddy_system_t *buddy, size_t size)
{
    if (!buddy || size == 0)
        return NULL;

    size += sizeof(buddy_alloc_head_t); // 预留头部信息

    // 找到第一个大于等于 size 的zone
    rb_node_t *rbnode = rb_tree_find_first_greater_or_equal(&buddy->tree, (void *)size, zone_find_by_capacity);
    if (rbnode == buddy->tree.nil)
        return NULL; // 没有合适的桶

    buddy_zone_t *zone = node_get_zone(rbnode); // 找到这个zone
    // 取出一个桶
    list_node_t *node = list_remove_first(&zone->buckets_list);
    bucket_t *bucket = list_node_parent(node, bucket_t, lnode);
    bucket->state = BUCKET_STATE_USED;
    ASSERT(zone->capacity == bucket->capacity);
    // **分裂桶**
    while (bucket->capacity / 2 >= size && (bucket->capacity / 2) % 2 == 0)
    {
        // 如果可以分裂，创建一个新桶
        bucket_t *new_bucket = bucket_alloc();
        new_bucket->capacity = bucket->capacity / 2;
        new_bucket->addr = bucket->addr + new_bucket->capacity;
        new_bucket->state = BUCKET_STATE_FREE;

        bucket->capacity /= 2;

       

        // 插入新的 `new_bucket` 到红黑树
        // 先找和new_bucket大小相同的zone
        buddy_zone_t* new_zone = find_zone_by_capacity(buddy,new_bucket->capacity);
        list_insert_last(&new_zone->buckets_list, &new_bucket->lnode);

        //rb_tree_inorder(&buddy->tree, &buddy->tree.root, zone_print);
    }

    
    // 如果zone上没有bucket了，直接把zone从红黑树上移除
    if (zone->buckets_list.count <= 0)
    {
        rb_tree_remove(&buddy->tree, zone);
        zone_free(zone);
    }
    rb_tree_inorder(&buddy->tree, &buddy->tree.root, zone_print);

    
    // 记录桶信息
    buddy_alloc_head_t *header = (buddy_alloc_head_t *)bucket->addr;
    header->bucket = bucket;

    return (void *)(bucket->addr + sizeof(buddy_alloc_head_t));
}

static void buddy_system_dynamic_free(buddy_system_t *buddy, void *addr)
{
    if (!buddy || !addr)
        return;

    // 找到对应的 `bucket`
    buddy_alloc_head_t *header = (buddy_alloc_head_t *)((addr_t)addr - sizeof(buddy_alloc_head_t));
    bucket_t *bucket = header->bucket;
    bucket->state = BUCKET_STATE_FREE;

    bucket_t *buddy_bucket = dynamic_get_buddy(buddy,bucket);
    // **合并逻辑**
    while (buddy_bucket && buddy_bucket->state == BUCKET_STATE_FREE)
    {
        

        // 释放 `buddy_bucket`
        //先找兄弟节点所在的zone
        buddy_zone_t* buddy_zone = find_zone_by_capacity(buddy,buddy_bucket->capacity);
        ASSERT(buddy_zone->buckets_list.count > 0);//兄弟节点肯定在里面，要不然就是哪里出错了
        //取出buddy_bucket
        list_remove(&buddy_zone->buckets_list,&buddy_bucket->lnode);
        if(buddy_zone->buckets_list.count <=0){ 
            //如果buddyzone没有任何bucket了，直接从红黑树中删除
            rb_tree_remove(&buddy->tree,buddy_zone);
            zone_free(buddy_zone);
        }
        

        // **合并当前 `bucket`**
        if (bucket->addr < buddy_bucket->addr)
        {
            bucket->capacity *= 2; // `bucket` 在前，合并后容量翻倍
        }
        else
        {
            bucket->capacity *= 2;
            bucket->addr = buddy_bucket->addr; // 让 `bucket` 指向前面的块
        }

        
        bucket_free(buddy_bucket);

        buddy_bucket = dynamic_get_buddy(buddy,bucket);
    }

    // **插入合并后的 `bucket` 回红黑树**
    // 先找zone
    buddy_zone_t* new_zone = find_zone_by_capacity(buddy,bucket->capacity);
    // 找到zone，把桶放入链表
    list_insert_last(&new_zone->buckets_list, &bucket->lnode);
    rb_tree_inorder(&buddy->tree, &buddy->tree.root, zone_print);
}

/**
 * @brief 伙伴系统动态扩容
 *        把首地址为addr，容量为capacity的内存加入伙伴系统进行管理
 */
static void buddy_system_dynamic_expand(buddy_system_t* buddy,void* addr,size_t capacity){
    //找一个zone，有就返回，没有就创建一个，加入红黑树并返回
    buddy_zone_t* new_zone = find_zone_by_capacity(buddy,capacity);

    //创建一个桶
    bucket_t* new_bucket = bucket_alloc();
    new_bucket->addr = addr;
    new_bucket->capacity = capacity;
    new_bucket->state = BUCKET_STATE_FREE;

    //将桶加入zone
    list_insert_last(&new_zone->buckets_list,&new_bucket->lnode);

}

void buddy_system_enable(void){
    bucket_mempool_init();
    zone_mempool_init();
    capacity_array_init();
}

void buddy_mmpool_fix_init(buddy_mmpool_fix_t* mmpool,void* base,size_t capacity){
    buddy_system_init(&mmpool->sys,base,capacity);
    mmpool->alloc = buddy_system_alloc;
    mmpool->free = buddy_system_free;

}
void buddy_mmpool_dyn_init(buddy_mmpool_dyn_t* mmpool,void* base,size_t capacity){
    buddy_dynamic_system_init(&mmpool->sys,base,capacity);
    mmpool->alloc = buddy_system_dynamic_alloc;
    mmpool->free = buddy_system_dynamic_free;
    mmpool->sbrk = buddy_system_dynamic_expand;

}

