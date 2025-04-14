#ifndef __MINIX_ENTRY_H
#define __MINIX_ENTRY_H
#include "fs/minix_cfg.h"
#include "types.h"
#include "fs/minix_inode.h"
// 文件目录项结构
typedef struct minix_dentry_t
{
    uint16_t nr;                    // i 节点
    char name[MINIX1_NAME_LEN]; // 文件名
} minix_dentry_t;

int find_entry(minix_inode_desc_t *inode, const char *name, minix_dentry_t *entry);
int add_entry(minix_inode_desc_t *inode, const char *name,int ino);
int update_entry(minix_inode_desc_t *inode, minix_dentry_t *dentry);
int delete_entry(minix_inode_desc_t *inode, const char *name);
int delete_entry_not_dir(minix_inode_desc_t* inode, const char* name);
int delete_entry_dir(minix_inode_desc_t* inode, const char* name, bool recursion) ;
void print_entrys(minix_inode_desc_t* inode) ;
#endif