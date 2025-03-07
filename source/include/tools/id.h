#ifndef __ID_H
#define __ID_H
#include "types.h"
#define ID_BITMAP_SIZE 1024  // 最大支持的 ID 数量（可调整）
// ID 池结构体
typedef struct _id_pool_t {
    uint32_t start;              // ID 范围开始
    uint32_t end;                // ID 范围结束
    uint8_t ids_bitmap[ID_BITMAP_SIZE];         // 位图数组，记录 ID 是否被占用
    uint32_t total_ids;          // 可用 ID 总数
} id_pool_t;

int id_pool_init(uint32_t start, uint32_t end,id_pool_t *pool) ;
int allocate_id(id_pool_t *pool) ;

void release_id(id_pool_t *pool, uint32_t id) ;

#endif