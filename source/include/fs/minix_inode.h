#ifndef __MINIX_INODE_H__
#define __MINIX_INODE_H__


#include "types.h"
#include "time/time.h"
#include "fs/minix_cfg.h"
#include "tools/rb_tree.h"
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

typedef struct _minix_inode_desc_t
{
    minix_inode_t* data; //inode地址
    int major;
    int minor; //设备号
    int idx;//inode bitmap下标
    int ref;
    time_t atime; // 访问时间
    time_t ctime; // 创建时间
    struct _inode_t *parent;
    char name[MINIX1_NAME_LEN];
    rb_node_t rbnode;
}minix_inode_desc_t;

int minix_dblk_alloc(int major, int minor) ;
int minix_dblk_free(int major, int minor, int blk);
int minix_inode_alloc(int major, int minor);
int minix_inode_free(int major, int minor, int ino);

void minix_inode_tree_init(void);
int minix_inode_read(minix_inode_desc_t *inode, char *buf, int buf_size, int whence, int read_size);
int minix_inode_write(minix_inode_desc_t *inode, const char *buf, int buf_size, int whence, int write_size);
minix_inode_desc_t* minix_inode_find(int major,int minor,int ino);
minix_inode_desc_t *minix_inode_create(minix_inode_desc_t *parent, const char *name);
int minix_inode_truncate(minix_inode_desc_t *inode, uint32_t new_size);
minix_inode_desc_t *minix_namei(const char *path);
minix_inode_desc_t* minix_get_parent(minix_inode_desc_t* inode) ;
minix_inode_desc_t *minix_named(const char *path);
minix_inode_desc_t* minix_get_dev_root_inode(int major, int minor) ;


char* minix_inode_to_absolute_path(minix_inode_desc_t* inode);
void stat_inode(minix_inode_desc_t *inode);
void ls_inode(minix_inode_desc_t *inode);
void tree_inode(minix_inode_desc_t *inode, int level);


minix_inode_desc_t* get_root_inode(void);
#endif