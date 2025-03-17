#include "mem/buddy_system.h"
#include "mem/bitmap.h"
#include "mem/kmalloc.h"
#include "cpu_cfg.h"
static buddy_system_t km_allocator;
void km_allocator_init(void){
    ph_addr_t start = mm_bitmap_alloc_pages(KM_MEM_TOTAL_SIZE/MEM_PAGE_SIZE);
    buddy_system_init(&km_allocator,start,KM_MEM_TOTAL_SIZE);
    /******注：后续要让page_t记录信息为内核页 */
}

ph_addr_t get_km_allocator_start(void){
    return km_allocator.start;
}
size_t get_km_allocator_size(void){
    return km_allocator.size;
}

void* kmalloc(size_t size){
    return buddy_system_alloc(&km_allocator,size);
}

void kfree(void* ptr){
    buddy_system_free(&km_allocator,ptr);
}