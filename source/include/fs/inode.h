#ifndef __INODE_H
#define __INODE_H
#include "fs/devfs/devfs.h"
#include "tools/rb_tree.h"
#include "fs/fs_cfg.h"
typedef struct _inode_opts_t
{

}inode_opts_t;
typedef struct _inode_t
{
    struct _inode_t* i_parent;//inode父节点
    dev_t dev; //inode所在设备
    fs_type_t type;//所在设备挂载的文件系统
    inode_opts_t* ops;//不同类型的inode操作不同
    int nr; //inode的序号
    int ref;//inode被打开几次
    rb_node_t rbnode; //inode存放在树中
}inode_t;
void register_inode_operations(inode_opts_t* opts,fs_type_t type);
void inode_tree_init(void);
int inode_compare_key(const void *key, const void *data)int inode_compare_key(const void *key, const void *data);
rb_tree_t* get_inode_tree(void);
#endif