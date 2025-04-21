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

#endif