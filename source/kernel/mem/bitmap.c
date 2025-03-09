#include "mem/bitmap.h"
#include "boot_info.h"
#include "string.h"
#include "printk.h"
#include "cpu_cfg.h"
#include "cpu_instr.h"
#include "algrithm.h"
extern uint8_t s_ro; // 只读区起始地址
extern uint8_t s_rw; // 读写区起始地址
extern uint8_t e_rw; // 读写区结束地址
extern uint8_t e_init_task_ph;
// 全局位图对象
static mm_bitmap_t mm_bitmap;
static void caculate_mem_size(boot_info_t* boot_info){
    uint32_t sum = 0;
    for (int i = 0; i < boot_info->ram_region_count; i++)
    {
        sum += boot_info->ram_region_cfg[i].size;
    }
    boot_info->mem_size = sum;
}
/**
 * @brief 设置位图所要管理的内存区域
 */
static void mm_bitmap_set_region(int index, ph_addr_t start, uint32_t size) {
   
    ASSERT(start%MEM_PAGE_SIZE==0 && size %MEM_PAGE_SIZE == 0 && size > 0);
    if (index >= MM_REGION_CNT_MAX) {
        dbg_error("mm region out of boundry\r\n");
        dbg_error("you can modify the define MM_REGION_CNT_MAX more large\r\n");
        hlt();
    }
    mm_bitmap.region[index].start = start;
    mm_bitmap.region[index].size = size;
}
/**
 * @brief 将物理地址转换为位图中的索引
 */
static int addr_to_bit_index(ph_addr_t addr) {
    for (int i = 0; i < MM_REGION_CNT_MAX; i++) {
        if (addr >= mm_bitmap.region[i].start &&
            addr < mm_bitmap.region[i].start + mm_bitmap.region[i].size) {
            return (addr - mm_bitmap.region[i].start) / MEM_PAGE_SIZE;
        }
    }
    dbg_error("The address is not within the scope of bitmap management!\r\n");
    return -1;  // 地址不在任何区域中
}

/**
 * @brief 将位图中的索引转换为物理地址
 */
static ph_addr_t bit_index_to_addr(int bit_index) {
    for (int i = 0; i < MM_REGION_CNT_MAX; i++) {
        uint32_t page_count = mm_bitmap.region[i].size / MEM_PAGE_SIZE;
        if (bit_index < page_count) {
            return mm_bitmap.region[i].start + bit_index * MEM_PAGE_SIZE;
        }
        bit_index -= page_count;
    }
    dbg_warning("bitmap get addr,product an unvalid phaddr:0\r\n");
    return 0;  // 无效地址
}
/**
 * @brief 初始化位图
 */
void mm_bitmap_init(boot_info_t* boot_info) {
    
    ph_addr_t ph_s_ro = &s_ro;
    ph_addr_t ph_s_rw = &s_rw;
    ph_addr_t ph_e_rw = &e_rw;
    ph_addr_t ph_e_init = &e_init_task_ph;
    //位图所管理的内存，必须比实际内存要大
    if(MM_BITMAP_MAP_SIZE_BYTE*8*MEM_PAGE_SIZE < boot_info->mem_size){
        dbg_error("Bitmap is too small to manage the entire memory!\r\n");
        dbg_error("you can modify the define MM_BITMAP_MAP_SIZE_BYTE more large\r\n");
    }
    // 清零位图
    memset(mm_bitmap.map, 0, MM_BITMAP_MAP_SIZE_BYTE);  
    for (int i = 0; i < MM_REGION_CNT_MAX; i++) {
        mm_bitmap.region[i].start = 0;
        mm_bitmap.region[i].size = 0;
    }
    //设置位图所要管理的内存区域,从kenrel起始地址+kernel_size开始管理,而且4KB对齐
    int j = 0;//bitmap.region下标
    //内核末尾4KB对齐
    uint32_t mem_start = align_up(ph_e_init,MEM_PAGE_SIZE) ;
    //遍历boot_info每个区域，如果在bitmap管理范围内就加进去
    for (int i = 0; i < boot_info->ram_region_count; i++)
    {
        uint32_t region_start = boot_info->ram_region_cfg[i].start;
        uint32_t region_size = boot_info->ram_region_cfg[i].size;
        uint32_t region_end = region_start + region_size- 1;
        //由于对齐原因，region_end 绝不会等于 mem_start
        ASSERT(region_end != mem_start);
        if(region_end < mem_start){
            //不在bitmap管理范围内
            continue;
        }
        else{
            uint32_t manage_size = 0;
            //region_end > mem_start,肯定在mem_start管理范围内
            if(region_start < mem_start){
                //例如大小为2.5个页，那么就只管理2个页
                manage_size = align_down(region_size-(mem_start-region_start),MEM_PAGE_SIZE);
                if(manage_size <= 0){
                    //如果为0，就不要往bitmap.region里面装了
                    continue;
                }
                mm_bitmap_set_region(j,mem_start,manage_size);
            }else{
                manage_size = align_down(region_size,MEM_PAGE_SIZE);
                if(manage_size <= 0){
                    continue;
                }
                mm_bitmap_set_region(j,region_start,region_size);
            }
            j++;
        }
        
    }
    mm_bitmap_print();
}


/**
 * @brief 分配一页
 */
ph_addr_t mm_bitmap_alloc_page() {
    ph_addr_t ret;
    for (int i = 0; i < MM_BITMAP_MAP_SIZE_BYTE * 8; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;
        if (!(mm_bitmap.map[byte_index] & (1 << bit_index))) {
            mm_bitmap.map[byte_index] |= (1 << bit_index);  // 标记为已占用
            ret = bit_index_to_addr(i);
            memset((void*)ret,0,MEM_PAGE_SIZE);
            return ret;
        }
    }
    dbg_warning("out of memory:bit map can`t alloc one page\r\n");
    return 0;  // 没有可用页
}

/**
 * @brief 释放一页
 */
void mm_bitmap_free_page(ph_addr_t addr) {
    int bit_index = addr_to_bit_index(addr);
    if (bit_index == -1) return;  // 无效地址

    int byte_index = bit_index / 8;
    int bit_offset = bit_index % 8;
    mm_bitmap.map[byte_index] &= ~(1 << bit_offset);  // 标记为未占用
    memset((void*)addr,0,MEM_PAGE_SIZE);
}

/**
 * @brief 分配多页
 */
ph_addr_t mm_bitmap_alloc_pages(uint32_t num_pages) {
    ph_addr_t ret;
    int start_bit_index = -1;
    int consecutive_free = 0;

    for (int i = 0; i < MM_BITMAP_MAP_SIZE_BYTE * 8; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;
        if (!(mm_bitmap.map[byte_index] & (1 << bit_index))) {
            if (start_bit_index == -1) {
                start_bit_index = i;
            }
            consecutive_free++;
            if (consecutive_free == num_pages) {
                // 标记这些页为已占用
                for (int j = start_bit_index; j < start_bit_index + num_pages; j++) {
                    int byte_idx = j / 8;
                    int bit_offset = j % 8;
                    mm_bitmap.map[byte_idx] |= (1 << bit_offset);
                }
                ret = bit_index_to_addr(start_bit_index);
                memset((void*)ret,0,MEM_PAGE_SIZE*num_pages);
                return ret;
            }
        } else {
            start_bit_index = -1;
            consecutive_free = 0;
        }
    }
    dbg_warning("There are no continue %d pages\r\n",num_pages);
    return 0;  // 没有足够的连续页
}

/**
 * @brief 释放多页
 */
void mm_bitmap_free_pages(ph_addr_t start_addr, uint32_t num_pages) {
    int start_bit_index = addr_to_bit_index(start_addr);
    if (start_bit_index == -1) {
        dbg_warning("What you want to release is an invalid page:0x%x\r\n",start_addr);
    }

    for (int i = start_bit_index; i < start_bit_index + num_pages; i++) {
        int byte_index = i / 8;
        int bit_offset = i % 8;
        mm_bitmap.map[byte_index] &= ~(1 << bit_offset);  // 标记为未占用
    }
    memset((void*)start_addr,0,MEM_PAGE_SIZE*num_pages);
}

 /**
  * @brief 打印位图状态（调试用）
  */
void mm_bitmap_print() {
    dbg_info("bitmap manage range:\r\n");
    dbg_info("..................\r\n");
    for (int i = 0; i < MM_REGION_CNT_MAX; i++)
    {
        mm_region_t *region =& mm_bitmap.region[i];
        if(region->size == 0){
            continue;
        }
        else{
            
            dbg_info("region[%d]: start = 0x%x  size = 0x%x \r\n",i,region->start,region->size);
        }
    }
    dbg_info("..................\r\n");
    
}