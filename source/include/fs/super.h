#ifndef __SUPER_H
#define __SUPER_H
#include "types.h"
#include "tools/list.h"
#include "fs/devfs/devfs.h"
#include "fs/fs_cfg.h"

typedef struct _super_block_t
{
    dev_t dev; //该超级快所处设备
    fs_type_t type; //该设备文件系统类型
    list_node_t node;//链表存储所有超级快

}super_block_t;
void super_block_init(void);

list_t* get_super_list(void);

#endif