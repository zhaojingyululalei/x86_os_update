#ifndef __MINIX_INODE_H
#define __MINIX_INODE_H
#include "types.h"
#include "fs/inode.h"
#include "fs/minix/minix_cfg.h"
// 文件目录项结构
typedef struct _minix_dentry_t
{
    uint16_t nr;                    // i 节点
    char name[MINIX1_NAME_LEN]; // 文件名
} minix_dentry_t;
typedef struct minix_inode_desc_t
{
    mode_t mode;    // 文件类型和属性(rwx 位)
    uint16_t uid;     // 用户id（文件拥有者标识符）
    uint32_t size;    // 文件大小（字节数）
    uint32_t mtime;   // 修改时间戳 这个时间戳应该用 UTC 时间，不然有瑕疵
    uint8_t gid;      // 组id(文件拥有者所在的组)
    uint8_t nlinks;   // 链接数（多少个文件目录项指向该i 节点）
    uint16_t zone[9]; // 直接 (0-6)、间接(7)或双重间接 (8) 逻辑块号
} minix_inode_desc_t;


typedef struct _minix_inode_t
{
    inode_t base; //扩展的一些信息
    minix_inode_desc_t *data;//磁盘中的固定信息
}minix_inode_t;
int minix_dblk_alloc(int major, int minor);
int minix_dblk_free(int major, int minor, int blk);
int inode_get_block(minix_inode_t *inode, int fblock, bool create);
#endif