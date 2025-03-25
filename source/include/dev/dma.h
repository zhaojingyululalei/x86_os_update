#ifndef __DMA_H
#define __DMA_H

#include "types.h"
#include "tools/list.h"
typedef struct _scatter_node_t{
    vm_addr_t vstart;
    ph_addr_t phstart;
    size_t len;
    list_node_t node;
}scatter_node_t;
typedef struct _scatter_list_t{
    list_t list;
}scatter_list_t;

int scatter_list_map(scatter_list_t *scatterlist, vm_addr_t vmbase, int len);
void scatter_list_destory(scatter_list_t* scatterlist);
ph_addr_t dma_alloc_coherent(size_t size);
void dma_free_coherent(ph_addr_t ptr,size_t size);

void scatter_list_debug_print(scatter_list_t *scatterlist);

#endif