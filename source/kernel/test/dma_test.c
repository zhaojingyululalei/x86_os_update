#include "dev/dma.h"
#include "cpu_cfg.h"
#include "printk.h"
scatter_list_t scatterlist;
void scatter_list_test(void){
    int ret;
    ret = scatter_list_map(&scatterlist,0x1200,MEM_PAGE_SIZE*4);
    if(ret<0){
        dbg_error("scatterlist test fail\r\n");
    }
    scatter_list_debug_print(&scatterlist);
}
void dma_test(void){
    scatter_list_test();
    size_t size = MEM_PAGE_SIZE*2+512;
    ph_addr_t base = dma_alloc_coherent(size);
    dma_free_coherent(base,size);
}