#ifndef __FS_H
#define __FS_H

#include "types.h"
#include "ipc/mutex.h"
#include "fs/file.h"
#include "fs/stat.h"
enum file_flag
{
    O_RDONLY = 00,      // 只读方式
    O_WRONLY = 01,      // 只写方式
    O_RDWR = 02,        // 读写方式
    O_ACCMODE = 03,     // 文件访问模式屏蔽码
    O_CREAT = 00100,    // 如果文件不存在就创建
    O_EXCL = 00200,     // 独占使用文件标志
    O_NOCTTY = 00400,   // 不分配控制终端
    O_TRUNC = 01000,    // 若文件已存在且是写操作，则长度截为 0
    O_APPEND = 02000,   // 以添加方式打开，文件指针置为文件尾
    O_NONBLOCK = 04000, // 非阻塞方式打开和操作文件
};
// 文件系统类型
typedef enum _fs_type_t {
    FS_NONE,
    FS_MINIX,
    FS_FAT16,
    FS_DEVFS,
}fs_type_t;
struct _fs_op_t;
typedef struct file_t
{
    //如果是 普通文件就是 minix_inode_desc_t*
    //如果是socket就是 socket_t
    //如果是dev那就是 dev_t
    void * metadata; //文件元数据
    uint32_t ref;
    int pos;
    fs_type_t fs_type;
    fs_op_t* fs_opts;
} file_t;

file_t * file_alloc (void);
void file_free (file_t * file) ;
void file_inc_ref (file_t * file) ;
typedef enum whence_t
{
    SEEK_SET = 1, // 直接设置偏移
    SEEK_CUR,     // 当前位置偏移
    SEEK_END      // 结束位置偏移
} whence_t;
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
    int (*mkdir)(minix_inode_desc_t* pinode,const char* dir_name,uint16_t mode);
    int (*link)(const char *old_path, const char *new_path);
    int (*unlink) (const char * path);
}fs_op_t;


int sys_open(const char *path, int flags, uint32_t mode);
int sys_read(int fd, char *buf, int len);
int sys_write(int fd,const char* buf,size_t len);
int sys_lseek(int fd, int offset, int whence);
int sys_close(int fd);
int sys_fsync(int fd);
int sys_mount(const char *path, int major, int minor, fs_type_t type);
int sys_unmount(const char *path) ;
void fs_register(fs_type_t type,fs_op_t* opt);

int sys_mkdir(const char* path, uint16_t mode) ;
int sys_rmdir(const char* path);
#endif