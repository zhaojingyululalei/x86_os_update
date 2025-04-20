#ifndef __FILE_H
#define __FILE_H
#include "types.h"
#include "minix_fs.h"
#include "cpu_cfg.h"
#include "fs/minix_inode.h"
typedef struct {
    int           fd;        // 文件描述符，标识目录流
    int         offset;    // 偏移量，指示读取目录条目的位置
    struct dirent *entry;    // 当前目录条目的指针
} DIR;
#define DT_UNKNOWN   0   // 未知类型
#define DT_FIFO      1   // 命名管道（FIFO）
#define DT_CHR       2   // 字符设备
#define DT_DIR       4   // 目录
#define DT_BLK       6   // 块设备
#define DT_REG       8   // 普通文件
#define DT_LNK      10   // 符号链接
#define DT_SOCK     12   // 套接字
#define DT_WHT      14   // 保留类型
typedef struct dirent {
    int          d_ino;        // 文件的 inode 编号
    uint16_t d_size;     // 目录条目的长度
    uint8_t  d_type;       // 文件类型（例如普通文件、目录、符号链接等）
    char           d_name[MINIX1_NAME_LEN];  // 文件名
}dirent_t;



#endif