#ifndef __INODE_H
#define __INODE_H
#include "fs/devfs/devfs.h"
#include "tools/rb_tree.h"
#include "fs/fs_cfg.h"
#include "fs/dirent.h"
#include "fs/stat.h"
struct _inode_opts_t;
typedef struct _inode_t
{
    struct _inode_t *i_parent; // inode父节点
    dev_t dev;                 // inode所在设备
    fs_type_t type;            // 所在设备挂载的文件系统
    struct _inode_opts_t *ops; // 不同类型的inode操作不同
    int nr;                    // inode的序号
    rb_node_t rbnode;          // inode存放在树中
    char name[128];            // 文件名
    bool mount;                // 该inode是否被挂载
} inode_t;
typedef struct _inode_opts_t
{
    inode_t *(*create)(dev_t dev, int nr);                                                   // 创建inode并入树
    void (*destroy)(inode_t *inode);                                                         // 从树中移除并释放inode_t结构体
    int (*free)(dev_t dev, int nr);                                                          // 位图释放inode
    int (*alloc)(dev_t dev);                                                                 // 位图分配inode
    inode_t *(*get)(dev_t dev, int nr);                                                      // 打开已分配的inode并返回inode
    int (*read)(inode_t *inode, char *buf, int buf_size, int whence, int read_size);         // 读取文件内容
    int (*write)(inode_t *inode, const char *buf, int buf_size, int whence, int write_size); // 写入文件内容
    int (*truncate)(inode_t *inode, uint32_t new_size);                                      // 截断文件
    int (*getinfo)(inode_t *inode, dev_t dev, int nr);                                       // 获取inode磁盘缓冲区中的信息
    int (*find_entry)(inode_t *inode, const char *name, struct dirent *entry);               // 查找目录项
    int (*add_entry)(inode_t *inode, struct dirent *entry);                                  // 添加目录项
    int (*delete_entry)(inode_t *inode, struct dirent *entry);                               // 删除目录项
    inode_t *(*get_dev_root_inode)(dev_t dev);                                               // 获取特定文件系统根目录inode的方式
    int (*mkdir)(inode_t *parent_inode, const char *dir_name, mode_t mode);                  // 创建目录
    inode_t *(*open)(inode_t *inode, const char *file_name, uint32_t flag, mode_t mode);     // 打开文件
    void (*fsync)(inode_t *inode);
    void (*stat)(inode_t *inode, file_stat_t *state);
    int (*rmdir)(inode_t *inode, const char *dir_name);
    int (*root_nr)(void);
    mode_t (*acquire_mode)(inode_t *inode);
    uint16_t (*acquire_uid)(inode_t *inode);
    uint32_t (*acquire_size)(inode_t *inode);
    uint32_t (*acquire_mtime)(inode_t *inode);
    uint32_t (*acquire_atime)(inode_t *inode);
    uint32_t (*acquire_ctime)(inode_t *inode);
    uint8_t (*acquire_gid)(inode_t *inode);
    uint8_t (*acquire_nlinks)(inode_t *inode);

} inode_opts_t;
void register_inode_operations(inode_opts_t *opts, fs_type_t type);
void inode_tree_init(void);
int inode_compare_key(const void *key, const void *data);
rb_tree_t *get_inode_tree(void);

inode_t *inode_get(dev_t dev, int nr);
inode_t *inode_create(dev_t dev, int nr);
void inode_destroy(inode_t *inode);
int inode_free(dev_t dev, int nr);
int inode_alloc(dev_t dev);
int inode_read(inode_t *inode, char *buf, int buf_size, int whence, int read_size);
int inode_write(inode_t *inode, const char *buf, int buf_size, int whence, int write_size);
int inode_truncate(inode_t *inode, uint32_t new_size);
int inode_getinfo(inode_t *inode, dev_t dev, int nr);
int inode_find_entry(inode_t *inode, const char *name, struct dirent *entry);
int inode_add_entry(inode_t *inode, struct dirent *entry);
int inode_delete_entry(inode_t *inode, struct dirent *entry);
inode_t *get_dev_root_inode(dev_t dev);
int inode_mkdir(inode_t *inode, const char *dir_name, mode_t mode);
inode_t *inode_open(inode_t *inode, const char *file_name, uint32_t flag, mode_t mode);
void inode_fsync(inode_t *inode);
void inode_stat(inode_t *inode, file_stat_t *state);
int inode_rmdir(inode_t *inode, const char *name);
int inode_root_nr(fs_type_t type);
mode_t inode_acquire_mode(inode_t *inode);
uint16_t inode_acquire_uid(inode_t *inode);
uint32_t inode_acquire_size(inode_t *inode);
uint32_t inode_acquire_mtime(inode_t *inode);
uint32_t inode_acquire_atime(inode_t *inode);
uint32_t inode_acquire_ctime(inode_t *inode);
uint8_t inode_acquire_gid(inode_t *inode);
uint8_t inode_acquire_nlinks(inode_t *inode);
#endif