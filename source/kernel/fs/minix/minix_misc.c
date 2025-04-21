
#include "fs/devfs/devfs.h"
#include "fs/minix/minix_inode.h"
#include "fs/inode.h"
#include "mem/kmalloc.h"
#include "printk.h"
#include "fs/minix/minix_supper.h"
#include "fs/minix/minix_cfg.h"
#include "fs/buffer.h"
#include "time/time.h"
#include "string.h"
/**
 * @brief 分配一个空闲数据块，返回块号,已经考虑了first_datablk
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回分配的数据块号，失败返回-1
 */
int minix_dblk_alloc(int major, int minor)
{
    dev_t dev = MKDEV(major,minor);
    minix_super_t* sup_blk = minix_get_super(dev);
    
    if (!sup_blk)
    {
        dbg_error("get major=%d,minor=%d super desc failed\r\n", major, minor);
        return NULL;
    }

    // 检查超级块是否有效
    if (sup_blk->data == NULL || sup_blk->zone_map == NULL)
    {
        return -1;
    }

    // 计算位图总大小(以位为单位)
    uint32_t bits = sup_blk->data->zones - sup_blk->data->firstdatazone + 1;
    uint32_t bytes = (bits + 7) / 8; // 位图字节数

    // 在位图中查找第一个空闲块
    for (uint32_t i = 0; i < bytes; i++)
    {
        if (sup_blk->zone_map[i] != 0xFF)
        { // 不是全1，说明有空闲块
            // 查找第一个为0的位
            for (int j = 0; j < 8; j++)
            {
                if (!(sup_blk->zone_map[i] & (1 << j)))
                {
                    uint32_t bit_offset = i * 8 + j;
                    uint32_t blk = sup_blk->data->firstdatazone + bit_offset;

                    if (blk >= sup_blk->data->firstdatazone + sup_blk->data->zones)
                    {
                        return -2;
                    }
                    // 标记该块为已使用
                    sup_blk->zone_map[i] |= (1 << j);
                    buffer_t *buf = fs_read_to_buffer(major, minor, blk, true);
                    memset(buf->data, 0, BLOCK_SIZE);
                    return blk; // 返回分配的数据块号
                }
            }
        }
    }

    return -1; // 没有空闲块可用
}

/**
 * @brief 释放一个数据块,已经考虑了first_datablk
 * @param major 主设备号
 * @param minor 次设备号
 * @param blk 要释放的数据块号(就是zone里面的块号，是整个分区的相对块号，不是位图的)
 * @return 成功返回0，失败返回-1
 */
int minix_dblk_free(int major, int minor, int blk)
{
    dev_t dev = MKDEV(major,minor);
    minix_super_t* sup_blk = minix_get_super(dev);
    if (!sup_blk)
    {
        dbg_error("get major=%d,minor=%d super desc failed\r\n", major, minor);
        return NULL;
    }

    // 检查超级块是否有效
    if (sup_blk->data == NULL || sup_blk->zone_map == NULL)
    {
        return -1;
    }

    // 检查块号是否有效
    if (blk < sup_blk->data->firstdatazone || blk >= sup_blk->data->firstdatazone + sup_blk->data->zones)
    {
        return -1;
    }

    // 计算位图中的位置
    uint32_t bit_offset = blk - sup_blk->data->firstdatazone;
    uint32_t byte_offset = bit_offset / 8;
    uint8_t bit_mask = 1 << (bit_offset % 8);

    // 检查该块是否已经被释放
    if (!(sup_blk->zone_map[byte_offset] & bit_mask))
    {
        return -1; // 该块已经是空闲状态
    }

    // 清除位图中的位
    sup_blk->zone_map[byte_offset] &= ~bit_mask;
    return 0;
}
// 处理直接块
static int handle_direct_block(minix_inode_t *inode, int fblock, bool create) {

    minix_inode_desc_t *minix_inode = inode->data;
    dev_t dev = inode->base.dev;
    int major = MAJOR(dev);
    int minor = MINOR(dev);

    if (minix_inode->zone[fblock] != 0) {
        return minix_inode->zone[fblock]; // 已有物理块
    }

    if (create) {
        int new_blk = minix_dblk_alloc(major, minor);
        if (new_blk > 0) {
            minix_inode->zone[fblock] = new_blk;
            return new_blk;
        }
    }
    return -1;
}

// 处理一级间接块
static int handle_indirect_block(minix_inode_t *inode, int fblock, bool create) {
    minix_inode_desc_t *minix_inode = inode->data;
    dev_t dev = inode->base.dev;
    int major = MAJOR(dev);
    int minor = MINOR(dev);

    // 检查间接块是否存在
    if (minix_inode->zone[7] == 0) {
        if (!create) return -1;

        // 分配间接块
        minix_inode->zone[7] = minix_dblk_alloc(major, minor);
        if (minix_inode->zone[7] == 0) return -1;
    }

    // 读取间接块
    buffer_t *indirect_buf = fs_read_to_buffer(major, minor, minix_inode->zone[7], true);
    if (!indirect_buf) {
        dbg_error("read buffer fail\r\r\n");
        return -2;
    }
    uint16_t *indirect_block = indirect_buf->data;

    int idx = fblock - 7;
    if (indirect_block[idx] != 0) {
        return indirect_block[idx]; // 已有物理块
    }

    if (create) {
        int new_blk = minix_dblk_alloc(major, minor);
        if (new_blk > 0) {
            indirect_block[idx] = new_blk;
            return new_blk;
        }
    }
    return -1;
}

// 处理二级间接块
static int handle_double_indirect_block(minix_inode_t *inode, int fblock, bool create) {
    minix_inode_desc_t *minix_inode = inode->data;
    dev_t dev = inode->base.dev;
    int major = MAJOR(dev);
    int minor = MINOR(dev);

    // 检查二级间接块是否存在
    if (minix_inode->zone[8] == 0) {
        if (!create) return -1;

        // 分配二级间接块
        minix_inode->zone[8] = minix_dblk_alloc(major, minor);
        if (minix_inode->zone[8] == 0) return -1;
    }

    // 读取二级间接块
    buffer_t *double_indirect_buf = fs_read_to_buffer(major, minor, minix_inode->zone[8], true);
    if (!double_indirect_buf) {
        dbg_error("read double indirect buffer fail\r\r\n");
        return -2;
    }
    uint16_t *double_indirect_block = double_indirect_buf->data;

    // 计算一级间接块的索引
    int idx1 = (fblock - (7 + BLOCK_SIZE / sizeof(uint16_t))) / (BLOCK_SIZE / sizeof(uint16_t));
    int idx2 = (fblock - (7 + BLOCK_SIZE / sizeof(uint16_t))) % (BLOCK_SIZE / sizeof(uint16_t));

    // 检查一级间接块是否存在
    if (double_indirect_block[idx1] == 0) {
        if (!create) return -1;

        // 分配一级间接块
        double_indirect_block[idx1] = minix_dblk_alloc(major, minor);
        if (double_indirect_block[idx1] == 0) return -1;
    }

    // 读取一级间接块
    buffer_t *indirect_buf = fs_read_to_buffer(major, minor, double_indirect_block[idx1], true);
    if (!indirect_buf) {
        dbg_error("read indirect buffer fail\r\r\n");
        return -3;
    }
    uint16_t *indirect_block = indirect_buf->data;

    // 检查数据块是否存在
    if (indirect_block[idx2] != 0) {
        return indirect_block[idx2]; // 已有物理块
    }

    if (create) {
        // 分配数据块
        int new_blk = minix_dblk_alloc(major, minor);
        if (new_blk > 0) {
            indirect_block[idx2] = new_blk;
            return new_blk;
        }
    }
    return -1;
}
/**
 * @brief 获取文件块在分区中的块号
 * @param fblock:该文件块块号。例如文件大小4KB，总共4块，每块2个扇区，每个扇区512字节。要获取它第二块所在分区的块号
 * @param create:如果这个fblock超过了该文件大小，也就是不存在，是否创建一个数据块。
 * @return 返回该文件块在分区中的块号
 */
int inode_get_block(minix_inode_t *inode, int fblock, bool create) {
    // 参数检查
    if (!inode || !inode->data || fblock < 0) {
        return -1;
    }

    // 1. 直接块处理 (0-6)
    if (fblock < 7) {
        return handle_direct_block(inode, fblock, create);
    }

    // 2. 一级间接块处理
    int indirect_limit = 7 + BLOCK_SIZE / sizeof(uint16_t);
    if (fblock < indirect_limit) {
        return handle_indirect_block(inode, fblock, create);
    }

    // 3. 二级间接块处理
    return handle_double_indirect_block(inode, fblock, create);
}