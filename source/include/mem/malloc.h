#ifndef __MALLOC_H
#define __MALLOC_H

#include "types.h"
void* sys_malloc(size_t size);
void sys_free(void* ptr);
#endif