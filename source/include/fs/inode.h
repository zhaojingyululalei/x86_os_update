#ifndef __INODE_H
#define __INODE_H
#include "types.h"
#include "time/time.h"
typedef struct minix_inode_t
{
    uint16_t mode;    // 文件类型和属性(rwx 位)
    uint16_t uid;     // 用户id（文件拥有者标识符）
    uint32_t size;    // 文件大小（字节数）
    uint32_t mtime;   // 修改时间戳 这个时间戳应该用 UTC 时间，不然有瑕疵
    uint8_t gid;      // 组id(文件拥有者所在的组)
    uint8_t nlinks;   // 链接数（多少个文件目录项指向该i 节点）
    uint16_t zone[9]; // 直接 (0-6)、间接(7)或双重间接 (8) 逻辑块号
} minix_inode_t;

typedef struct _inode_t
{
    minix_inode_t* data; //inode地址
    int major;
    int minor; //设备号
    int idx;//inode bitmap下标
    time_t atime; // 访问时间
    time_t mtime; // 修改时间
    time_t ctime; // 创建时间

}inode_t;
void stat_inode(inode_t* inode);
void ls_inode(inode_t* inode);
void tree_inode(inode_t* inode, int level);
#endif