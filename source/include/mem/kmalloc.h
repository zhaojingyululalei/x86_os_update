#ifndef __KMALLOC_H
#define __KMALLOC_H
#include "types.h"
void km_allocator_init(void);
ph_addr_t get_km_allocator_start(void);
size_t get_km_allocator_size(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
#endif