#ifndef __FS_CFG_H
#define __FS_CFG_H

#include "types.h"
typedef uint16_t mode_t;
typedef enum whence_t
{
    SEEK_SET = 1, // 直接设置偏移
    SEEK_CUR,     // 当前位置偏移
    SEEK_END      // 结束位置偏移
} whence_t;
// 文件系统类型
typedef enum _fs_type_t {
    FS_NONE,
    FS_DEVFS,
    FS_MINIX = 0x83,
    FS_FAT32 = 0xC,
}fs_type_t;
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
//根文件系统所在设备
#define FS_ROOT_MAJOR   7
#define FS_ROOT_MINOR   16
#define FS_ROOT_TYPE    0x83 //minix

#endif