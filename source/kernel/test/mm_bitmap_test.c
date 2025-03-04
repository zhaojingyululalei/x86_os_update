#include "mem/bitmap.h"
#include "printk.h"
void bitmap_test(void){
    // 分配一页
    ph_addr_t page1 = mm_bitmap_alloc_page();
    dbg_info("Allocated page: 0x%x\r\n", page1);

    // 分配多页
    ph_addr_t pages = mm_bitmap_alloc_pages(4);
    dbg_info("Allocated 4 pages starting at: 0x%x\r\n", pages);

    // 打印位图状态
    //mm_bitmap_print();

    // 释放一页
    mm_bitmap_free_page(page1);
    dbg_info("Freed page: 0x%x\r\n", page1);

    // 释放多页
    mm_bitmap_free_pages(pages, 4);
    dbg_info("Freed 4 pages starting at: 0x%x\r\n", pages);

    // 打印位图状态
    //mm_bitmap_print();
}