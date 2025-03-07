#include "tools/id.h"

#include "printk.h"


/**
 * @brief 初始化 ID 池
 * */ 
int id_pool_init(uint32_t start, uint32_t end,id_pool_t *pool) {
    if (start > end) {
        return NULL;
    }

    
    if (!pool) {
        return NULL;
    }

    pool->start = start;
    pool->end = end;
    pool->total_ids = end - start + 1;
    
    // 分配位图存储 ID 使用状态
    uint32_t bitmap_bytes = (pool->total_ids + 7) / 8;  // 计算字节数
    if(bitmap_bytes > ID_BITMAP_SIZE)
    {
        dbg_error("id pool out of boundry,please modify the difine ID_BITMAP_SIZE\r\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief 分配 ID
 */
int allocate_id(id_pool_t *pool) {
    if (!pool || !pool->ids_bitmap) {
        return -1;
    }

    for (uint32_t i = 0; i < pool->total_ids; i++) {
        uint32_t byte_index = i / 8;  // 找到对应的字节
        uint32_t bit_index = i % 8;   // 找到字节内的具体位
        if ((pool->ids_bitmap[byte_index] & (1 << bit_index)) == 0) {
            pool->ids_bitmap[byte_index] |= (1 << bit_index);
            return pool->start + i;  // 返回分配的 ID
        }
    }

    return -1;  // 无可用 ID
}

/**
 * @brief 释放 ID
 */
void release_id(id_pool_t *pool, uint32_t id) {
    if (!pool || !pool->ids_bitmap || id < pool->start || id > pool->end) {
        return;
    }

    uint32_t index = id - pool->start;
    uint32_t byte_index = index / 8;
    uint32_t bit_index = index % 8;

    pool->ids_bitmap[byte_index] &= ~(1 << bit_index);  // 清除位
}

