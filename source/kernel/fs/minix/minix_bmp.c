
#include "fs/minix_fs.h"
#include "printk.h"
#include "fs/buffer.h"
#include "string.h"
/**
 * @brief 分配一个空闲数据块，返回块号,已经考虑了first_datablk
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回分配的数据块号，失败返回-1
 */
int minix_dblk_alloc(int major, int minor) {
    super_block_t sup_blk;
    if (get_dev_super_block(&sup_blk, major, minor) != 0) {
        return -1; // 获取超级块失败
    }

    // 检查超级块是否有效
    if (sup_blk.data == NULL || sup_blk.zone_map == NULL) {
        return -1;
    }

    // 计算位图总大小(以位为单位)
    uint32_t bits = sup_blk.data->zones - sup_blk.data->firstdatazone + 1;
    uint32_t bytes = (bits + 7) / 8; // 位图字节数

    // 在位图中查找第一个空闲块
    for (uint32_t i = 0; i < bytes; i++) {
        if (sup_blk.zone_map[i] != 0xFF) { // 不是全1，说明有空闲块
            // 查找第一个为0的位
            for (int j = 0; j < 8; j++) {
                if (!(sup_blk.zone_map[i] & (1 << j))) {
                    uint32_t bit_offset = i * 8 + j;
                    uint32_t blk = sup_blk.data->firstdatazone + bit_offset;
                    
                    if(blk >= sup_blk.data->firstdatazone+sup_blk.data->zones ){
                        return -2;
                    }
                    // 标记该块为已使用
                    sup_blk.zone_map[i] |= (1 << j);
                    buffer_t* buf = fs_read_to_buffer(major,minor,blk,true);
                    memset(buf->data,0,BLOCK_SIZE);
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
 * @param blk 要释放的数据块号
 * @return 成功返回0，失败返回-1
 */
int minix_dblk_free(int major, int minor, int blk) {
    super_block_t sup_blk;
    if (get_dev_super_block(&sup_blk, major, minor) != 0) {
        return -1; // 获取超级块失败
    }

    // 检查超级块是否有效
    if (sup_blk.data == NULL || sup_blk.zone_map == NULL) {
        return -1;
    }

    // 检查块号是否有效
    if (blk < sup_blk.data->firstdatazone || blk >= sup_blk.data->firstdatazone+sup_blk.data->zones) {
        return -1;
    }

    // 计算位图中的位置
    uint32_t bit_offset = blk - sup_blk.data->firstdatazone;
    uint32_t byte_offset = bit_offset / 8;
    uint8_t bit_mask = 1 << (bit_offset % 8);

    // 检查该块是否已经被释放
    if (!(sup_blk.zone_map[byte_offset] & bit_mask)) {
        return -1; // 该块已经是空闲状态
    }

    // 清除位图中的位
    sup_blk.zone_map[byte_offset] &= ~bit_mask;
    return 0;
}

/**
 * @brief 分配一个空闲i节点，返回i节点号
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回分配的i节点号，失败返回-1
 */
int minix_inode_alloc(int major, int minor) {
    super_block_t sup_blk;
    if (get_dev_super_block(&sup_blk, major, minor) != 0) {
        return -1; // 获取超级块失败
    }

    // 检查超级块是否有效
    if (sup_blk.data == NULL || sup_blk.inode_map == NULL) {
        return -1;
    }

    // i节点总数
    uint16_t inodes = sup_blk.data->inodes;
    // i节点位图字节数
    uint16_t imap_bytes = (inodes + 7) / 8;

    // 在位图中查找第一个空闲i节点(从1开始，0是无效节点号)
    for (uint16_t i = 0; i < imap_bytes; i++) {
        if (sup_blk.inode_map[i] != 0xFF) { // 不是全1，说明有空闲i节点
            // 查找第一个为0的位
            for (int j = 0; j < 8; j++) {
                if (!(sup_blk.inode_map[i] & (1 << j))) {
                    uint16_t ino = i * 8 + j ; // i节点号从1开始
                    
                    // 检查i节点号是否有效
                    if (ino > inodes) {
                        return -1; // 无效的i节点号
                    }

                    // 标记该i节点为已使用
                    sup_blk.inode_map[i] |= (1 << j);
                    inode_t t;
                    get_dev_inode(&t,major,minor,ino);
                    memset(t.data,0,sizeof(minix_inode_t));
                    return ino; // 返回分配的i节点号
                }
            }
        }
    }

    return -1; // 没有空闲i节点可用
}

/**
 * @brief 释放一个i节点
 * @param major 主设备号
 * @param minor 次设备号
 * @param ino 要释放的i节点号(从1开始)
 * @return 成功返回0，失败返回-1
 */
int minix_inode_free(int major, int minor, int ino) {
    super_block_t sup_blk;
    if (get_dev_super_block(&sup_blk, major, minor) != 0) {
        return -1; // 获取超级块失败
    }

    // 检查超级块是否有效
    if (sup_blk.data == NULL || sup_blk.inode_map == NULL) {
        return -1;
    }

    // 检查i节点号是否有效
    if (ino < 1 || ino > sup_blk.data->inodes) {
        return -1;
    }

    // 计算位图中的位置
    uint16_t bit_offset = ino ; // i节点号从1开始，位图从0开始
    uint16_t byte_offset = bit_offset / 8;
    uint8_t bit_mask = 1 << (bit_offset % 8);

    // 检查该i节点是否已经被释放
    if (!(sup_blk.inode_map[byte_offset] & bit_mask)) {
        return -1; // 该i节点已经是空闲状态
    }

    // 清除位图中的位
    sup_blk.inode_map[byte_offset] &= ~bit_mask;
    return 0;
}