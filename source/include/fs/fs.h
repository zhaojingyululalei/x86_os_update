#ifndef __FS_H
#define __FS_H

#include "fs/fs_cfg.h"
#include "fs/file.h"
#include "fs/stat.h"
#include "fs/inode.h"
#include "fs/dirent.h"

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
    int (*readdir)(DIR* dir, dirent_t * dirent);
    int (*closedir)(DIR *dir);
    int (*mkdir)(inode_t* pinode,const char* dir_name,mode_t mode);
    int (*link)(const char *old_path, const char *new_path);
    int (*unlink) (const char * path);
}fs_op_t;
int sys_open(const char *path, int flags, mode_t mode);
int sys_read(int fd, char *buf, int len);

int sys_write(int fd, const char *buf, size_t len);
int sys_close(int fd);
int sys_lseek(int fd, int offset, int whence);
int sys_stat(int fd);
int sys_ioctl(int fd, int cmd, int arg0, int arg1);
int sys_fsync(int fd);
int sys_opendir(const char *path, DIR *dir);
int sys_readdir(DIR *dir, dirent_t *dirent);
int sys_closedir(DIR *dir);
int sys_link(const char *old_path, const char *new_path);
int sys_unlink(const char *path);
int sys_mkdir(const char *path, uint16_t mode);
int sys_rmdir(const char *path);
//extern void file_table_init(void);
void fs_init(void);
void fs_register(fs_type_t type, fs_op_t *opt);
#endif
