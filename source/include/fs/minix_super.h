#ifndef __MINIX_SUPER_H__
#define __MINIX_SUPER_H__
#include "types.h"
#define MAX_MINIX_SUPER_BLOCKS 32
typedef struct _minix_super_t
{
    uint16_t inodes;        // 节点数
    uint16_t zones;         // 逻辑块数
    uint16_t imap_blocks;   // i 节点位图所占用的数据块数
    uint16_t zmap_blocks;   // 逻辑块位图所占用的数据块数
    uint16_t firstdatazone; // 第一个数据逻辑块号
    uint16_t log_zone_size; // log2(每逻辑块数据块数)
    uint32_t max_size;      // 文件最大长度
    uint16_t magic;         // 文件系统魔数
} minix_super_t;


typedef struct _minix_super_desc_t
{
    enum{
        MINIX_SUPER_FREE,
        MINIX_SUPER_USED,
    }state;
    minix_super_t* data;//超级快加载到内存中的地址
    uint8_t* inode_map;
    uint8_t* zone_map;
    int major; //记录这是哪个设备的超级快
    int minor;
}minix_super_desc_t;

minix_super_desc_t* minix_get_super_desc(int major,int minor);
int minix_collect_super_desc(int major,int minor);
#endif