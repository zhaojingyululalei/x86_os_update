#ifndef __FS_H
#define __FS_H

#include "fs/fs_cfg.h"
#include "fs/file.h"
#include "fs/stat.h"
#include "fs/inode.h"
#include "fs/dirent.h"
#include "tools/list.h"


/**
 * @brief 文件系统操作接口
 */
typedef struct _fs_op_t {
	
    int (*open) ( file_t * file,const char *path, int flags, int mode);
    int (*read) (file_t * file,char * buf, int size);
    int (*write) (file_t * file,char * buf, int size);
    void (*close) (file_t * file);
    int (*lseek) (file_t * file, int offset, int whence);
    int (*stat)(file_t * file, file_stat_t *st);
    int (*ioctl) (file_t * file, int cmd, int arg0, int arg1);
    void (*fsync)(file_t* file);
    int (*opendir)(const char * path, DIR * dir);
    int (*readdir)(DIR* dir, struct dirent * dirent);
    int (*closedir)(DIR *dir);
    int (*mkdir)(inode_t* pinode,const char* dir_name,mode_t mode);
    int (*link)(const char *old_path, const char *new_path);
    int (*unlink) (const char * path);
    int (*mount)(const char *path, dev_t dev, fs_type_t type);
    int (*umount)(const char* path);

}fs_op_t;
inode_t* sys_namei(const char *path);
int sys_open(const char *path, int flags, mode_t mode);
int sys_read(int fd, char *buf, int len);

int sys_write(int fd, const char *buf, size_t len);
int sys_close(int fd);
int sys_lseek(int fd, int offset, int whence);
int sys_stat(int fd);
int sys_ioctl(int fd, int cmd, int arg0, int arg1);
int sys_fsync(int fd);
int sys_opendir(const char *path, DIR *dir);
int sys_readdir(DIR *dir, struct dirent *dirent);
int sys_closedir(DIR *dir);
int sys_link(const char *old_path, const char *new_path);
int sys_unlink(const char *path);
int sys_mkdir(const char *path, uint16_t mode);
int sys_rmdir(const char *path);
int sys_mount(const char *path, int major, int minor, fs_type_t type);
int sys_unmount(const char *path);

void fs_init(void);
void fs_register(fs_type_t type, fs_op_t *opt);
inode_t* get_root_inode(void);

#define MAX_PATH_BUF 256

typedef struct _mount_point_t {
    char point_path[MAX_PATH_BUF]; //挂载点路径，必须是绝对路径
    dev_t dev; //挂载设备
    fs_type_t type; //挂载文件系统类型

    dev_t orign_dev; //原来的设备号
    fs_type_t orign_type;//原来的文件系统类型

    inode_t* ipoint;//挂载到哪个inode上了
    list_node_t node;
}mount_point_t;
mount_point_t* find_point_by_abspath(const char* abs_path);
mount_point_t* find_point_by_inode(inode_t* inode);
#endif
