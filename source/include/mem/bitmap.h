#ifndef __BITMAP_H
#define __BITMAP_H

#include "types.h"
#include "boot_info.h"
#define MM_BITMAP_MAP_SIZE_BYTE 4096
#define MM_REGION_CNT_MAX   4
typedef struct _mm_region_t{
    ph_addr_t start;
    uint32_t size;
}mm_region_t;
typedef struct _mm_bitmap_t{
    uint8_t map[MM_BITMAP_MAP_SIZE_BYTE];
    mm_region_t region[MM_REGION_CNT_MAX];
}mm_bitmap_t;

void mm_bitmap_init(boot_info_t* boot_info) ;
ph_addr_t mm_bitmap_alloc_page();
void mm_bitmap_free_page(ph_addr_t addr);
ph_addr_t mm_bitmap_alloc_pages(uint32_t num_pages);
void mm_bitmap_free_pages(ph_addr_t start_addr, uint32_t num_pages);
void mm_bitmap_print();
#endif