#ifndef __MINIX_SUPPER_H
#define __MINIX_SUPPER_H
#include "types.h"
#include "fs/devfs/devfs.h"
#include "fs/super.h"
typedef struct _minix_super_desc_t
{
    uint16_t inodes;        // 节点数
    uint16_t zones;         // 整个分区总逻辑块数
    uint16_t imap_blocks;   // i 节点位图所占用的数据块数
    uint16_t zmap_blocks;   // 逻辑块位图所占用的数据块数
    uint16_t firstdatazone; // 第一个数据逻辑块号
    uint16_t log_zone_size; // log2(每逻辑块数据块数)
    uint32_t max_size;      // 文件最大长度
    uint16_t magic;         // 文件系统魔数
} minix_super_desc_t;


typedef struct _minix_super_t
{
    super_block_t base;//内存中的扩展信息
    minix_super_desc_t* data;//超级快在磁盘中固定的信息
    //inode和zone的位图，磁盘中的固定信息
    uint8_t* inode_map;
    uint8_t* zone_map;
}minix_super_t;

minix_super_t *minix_get_super(dev_t dev);
void minix_super_free(minix_super_t *super);
#endif