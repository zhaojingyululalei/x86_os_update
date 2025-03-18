#include "mem/buddy_system.h"
#include "mem/bitmap.h"
#include "mem/kmalloc.h"
#include "cpu_cfg.h"
static buddy_mmpool_fix_t km_pool;
void km_allocator_init(void){
    ph_addr_t start = mm_bitmap_alloc_pages(KM_MEM_TOTAL_SIZE/MEM_PAGE_SIZE);
    buddy_mmpool_fix_init(&km_pool,start,KM_MEM_TOTAL_SIZE);
    /******注：后续要让page_t记录信息为内核页 */
}

ph_addr_t get_km_allocator_start(void){
    return km_pool.sys.start;
}
size_t get_km_allocator_size(void){
    return km_pool.sys.size;
}

void* kmalloc(size_t size){
    return km_pool.alloc(&km_pool.sys,size);
}

void kfree(void* ptr){
    km_pool.free(&km_pool.sys,ptr);
}