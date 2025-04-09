#ifndef __BLK_CFG_H
#define __BLK_CFG_H


#define DISK_SECTOR_SIZE    512
#define BLOCK_SIZE  1024
#define BLOCK_SECS  (BLOCK_SIZE/DISK_SECTOR_SIZE)
#define UNIT_SIZE   4096 //最小单元4KB
#define UNIT_SECTORS    (UNIT_SIZE/SECTOR_SIZE) //一个文件最小占8个扇区
#define FS_BUFFER_NR    64       //有多少个缓冲页
#define FS_BUFFER_SIZE  (MEM_PAGE_SIZE*FS_BUFFER_NR)
#define FS_PERIOD_SYNC_TIME_SECOND  60
#endif