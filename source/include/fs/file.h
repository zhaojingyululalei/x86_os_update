#ifndef __FILE_H
#define __FILE_H
#include "types.h"
#include "minix_fs.h"
#include "cpu_cfg.h"
typedef struct {
    int           fd;        // 文件描述符，标识目录流
    int         offset;    // 偏移量，指示读取目录条目的位置
    struct dirent *entry;    // 当前目录条目的指针
} DIR;
typedef struct _dirent {
    int          d_ino;        // 文件的 inode 编号
    uint16_t d_size;     // 目录条目的长度
    uint8_t  d_type;       // 文件类型（例如普通文件、目录、符号链接等）
    char           d_name[MINIX1_NAME_LEN];  // 文件名
}dirent_t;
typedef struct file_t
{
    inode_t *inode; // 文件 inode
    uint32_t ref;
    uint32_t pos;
} file_t;

file_t * file_alloc (void);
void file_free (file_t * file) ;
void file_inc_ref (file_t * file) ;


#endif