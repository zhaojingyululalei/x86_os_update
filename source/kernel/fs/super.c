#include "fs/super.h"

static list_t super_block_list;

void super_block_init(void)
{
    list_init(&super_block_list);
}

list_t* get_super_list(void)
{
    return &super_block_list;
}

