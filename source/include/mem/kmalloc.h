#ifndef __KMALLOC_H
#define __KMALLOC_H

void km_allocator_init(void);
ph_addr_t get_km_allocator_start(void);
size_t get_km_allocator_size(void);
#endif