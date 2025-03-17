#ifndef __DS_COMM_H
#define __DS_COMM_H

#include "types.h"

#define offset_in_parent(parent_type, node_name)    \
    ((uintptr_t)&(((parent_type*)0)->node_name))

// 2.求node所在的结构体首址：node的地址 - node的偏移
// 即已知a->node的地址，求a的地址
#define offset_to_parent(node, parent_type, node_name)   \
    ((uintptr_t)node - offset_in_parent(parent_type, node_name))
#endif