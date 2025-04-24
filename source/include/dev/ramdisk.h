#ifndef __RAMDISK_H
#define __RAMDISK_H
#include "types.h"
#include "fs/fs_cfg.h"
#define RAMDISK_NR  4
typedef struct _ram_disk_t {
    char name[16];           // 设备名称
    uint32_t total_sectors;  // 总扇区数
    uint32_t sector_size;    // 扇区大小
    uint8_t *buffer;         // 内存缓冲区
    fs_type_t type;          // 文件系统类型
} ram_disk_t;

typedef enum _ramdisk_cmd_t{
    RAMDISK_CMD_SECTOR_COUNT = 1,
    RAMDISK_CMD_SECTOR_SIZE,
    RAMDISK_CMD_GET_FS_TYPE,
    RAMDISK_CMD_SET_FS_TYPE,
}ramdisk_cmd_t;
#endif