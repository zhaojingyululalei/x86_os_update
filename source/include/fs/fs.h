#ifndef __FS_H
#define __FS_H

#include "types.h"
#include "ipc/mutex.h"
#include "fs/inode.h"
#include "fs/file.h"
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
    FS_FAT16,
    FS_DEVFS,
}fs_type_t;



/**
 * @brief 文件系统操作接口
 */
typedef struct _fs_op_t {
	int (*mount) (struct _fs_t * fs,int major, int minor);
    void (*unmount) (struct _fs_t * fs);
    int (*open) (inode_t *dir, char *name, int flags, int mode);
    int (*read) (char * buf, int size, file_t * file);
    int (*write) (char * buf, int size, file_t * file);
    void (*close) (file_t * file);
    int (*lseek) (file_t * file, uint32_t offset, int dir);
    int (*stat)(file_t * file, struct stat *st);
    int (*ioctl) (file_t * file, int cmd, int arg0, int arg1);

    int (*opendir)(const char * name, DIR * dir);
    int (*readdir)(DIR* dir, struct dirent * dirent);
    int (*closedir)(DIR *dir);
    int (*unlink) (const char * path);
}fs_op_t;
typedef struct _fs_t {
    inode_t* mount_inode;       // 挂载点路径
    fs_type_t type;              // 文件系统类型

    fs_op_t * op;              // 文件系统操作接口
    void * data;                // 文件系统的操作数据
    int major;                 // 所属的设备
    int minor;
    list_node_t node;           // 下一结点
    mutex_t * mutex;              // 文件系统操作互斥信号量
}fs_t;
int sys_open(const char *path, int flags, uint32_t mode);
int sys_read(int fd, char *buf, int len);
int sys_write(int fd,const char* buf,size_t len);
int sys_lseek(int fd, int offset, int whence);
int sys_close(int fd);
int sys_fsync(int fd);
#endif