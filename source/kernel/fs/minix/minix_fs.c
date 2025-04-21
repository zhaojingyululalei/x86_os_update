
#include "fs/fs_cfg.h"
#include "fs/devfs/devfs.h"
#include "fs/inode.h"
extern inode_opts_t minix_inode_opts;
void minixfs_init(void)
{
    register_inode_operations(&minix_inode_opts,FS_MINIX);
}