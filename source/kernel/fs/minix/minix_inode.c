
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
 * @brief 分配一个inode并加入树中
 */
static minix_inode_t *minix_inode_alloc(dev_t dev, int nr)
{
    minix_inode_t *inode = (minix_inode_t *)kmalloc(sizeof(minix_inode_t));
    inode->base.dev = dev;
    inode->base.nr = nr;
    rb_tree_t *inode_tree = get_inode_tree();
    rb_tree_insert(inode_tree, inode);
}
/**
 * @brief 从树中移除该inode并释放
 */
void minix_inode_free(minix_inode_t *inode)
{
    rb_tree_t *inode_tree = get_inode_tree();
    rb_tree_remove(inode_tree, inode);
    kfree(inode);
}
/**
 * @brief 去树中查找inode，找不到返回null
 */
minix_inode_t *minix_inode_find(dev_t dev, int nr)
{
    int key[2] = {dev, nr};
    rb_tree_t *inode_tree = get_inode_tree();
    rb_node_t *rbnode = rb_tree_find_by(inode_tree, key, inode_compare_key);
    if (rbnode == inode_tree->nil)
    {
        return NULL;
    }
    return (minix_inode_t *)rb_node_parent(rbnode, inode_t, rbnode);
}
/**
 * @brief 从磁盘获取inode信息
 */
static int minix_inode_getinfo(minix_inode_t *inode, dev_t dev, int nr)
{
    // 参数检查
    if (nr < 1)
    {
        dbg_error("param fault\r\r\n");
        return -1;
    }
    int major = MAJOR(dev);
    int minor = MINOR(dev);
    minix_super_t *super = minix_get_super(dev);

    if (!super)
    {
        dbg_error("get major=%d,minor=%d super desc failed\r\n", major, minor);
        return -2;
    }

    // 检查超级块是否有效
    if (super->data == NULL || super->zone_map == NULL)
    {
        dbg_error("param fault\r\n");
        return -3;
    }

    // 检查inode号是否有效
    if (nr > super->data->inodes)
    {
        dbg_error("param fault\r\r\n");
        return -4;
    }
    // 检查inode是否已分配
    uint16_t byte_offset = nr / 8;
    uint8_t bit_mask = 1 << (nr % 8);
    if (!(super->inode_map[byte_offset] & bit_mask))
    {
        dbg_error("param fault\r\r\n");
        return -1; // inode未分配
    }

    // 计算inode表起始块号
    uint32_t inode_table_start = 2 + super->data->imap_blocks + super->data->zmap_blocks;

    // 计算inode大小
    uint32_t inode_size = sizeof(minix_inode_desc_t);

    // 计算每个块能存放的inode数量
    uint32_t inodes_per_block = MINIX1_BLOCK_SIZE / inode_size; // 一块1024字节

    // 计算目标inode所在的块号
    uint32_t inode_block = inode_table_start + (nr - 1) / inodes_per_block;

    // 计算inode在块内的偏移
    uint32_t inode_offset = ((nr - 1) % inodes_per_block) * inode_size;

    // 读取包含目标inode的块
    buffer_t *inode_buf = fs_read_to_buffer(major, minor, inode_block, true);
    if (inode_buf == NULL)
    {

        dbg_error("read to buffer failed\r\n");
        return -5;
    }
    inode->data = (minix_inode_desc_t *)(inode_buf->data + inode_offset);
    return 0;
}
inode_opts_t minix_inode_opts;
minix_inode_t *minix_inode_open(dev_t dev, int nr)
{
    int ret;
    minix_inode_t *inode = minix_inode_find(dev, nr);
    if (inode)
    {
        ret = minix_inode_getinfo(inode, dev, nr);
        ASSERT(ret >= 0);
        inode->base.dev = dev;
        inode->base.nr = nr;
        inode->base.ops = &minix_inode_opts;
        inode->base.ref++;
        inode->base.type = FS_MINIX;
        return inode;
    }
    inode = minix_inode_alloc(dev, nr);
    ret = minix_inode_getinfo(inode, dev, nr);
    ASSERT(ret >= 0);
    inode->base.dev = dev;
    inode->base.nr = nr;
    inode->base.ops = &minix_inode_opts;
    inode->base.ref++;
    inode->base.type = FS_MINIX;
    return inode;
}
/**
 * @brief 从inode的数据块中读取内容
 * @param inode 文件inode指针
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param whence 读取起始偏移量
 * @param read_size 要读取的字节数
 * @return 成功返回实际读取的字节数，失败返回-1
 */
int minix_inode_read(minix_inode_t *inode, char *buf, int buf_size, int whence, int read_size)
{
    if (!inode || !buf || buf_size <= 0 || whence < 0 || read_size < 0)
    {
        return -1;
    }

    minix_inode_desc_t *minix_inode = inode->data;
    dev_t dev = inode->base.dev;
    int major = MAJOR(dev);
    int minor = MINOR(dev);

    /* 检查偏移是否超出文件范围 */
    if (whence >= minix_inode->size)
    {
        return 0;
    }

    /* 调整读取大小防止越界 */
    if (read_size > buf_size)
    {
        read_size = buf_size;
    }
    if (whence + read_size > minix_inode->size)
    {
        read_size = minix_inode->size - whence;
    }
    if (read_size <= 0)
    {
        return 0;
    }

    /* 逐块读取 */
    int bytes_read = 0;
    char *block_data;
    int offset = whence;

    while (bytes_read < read_size)
    {
        /* 计算当前逻辑块和块内偏移 */
        uint32_t fblock = offset / BLOCK_SIZE;
        uint32_t blk_offset = offset % BLOCK_SIZE;

        /* 获取物理块号 */
        int phys_block = inode_get_block(inode, fblock, false);
        if (phys_block <= 0)
        {
            break; /* 块不存在或错误 */
        }

        /* 读取整个块 */
        buffer_t *blk_buf = fs_read_to_buffer(major, minor, phys_block, true);
        if (!blk_buf)
            break;
        block_data = blk_buf->data;

        /* 计算本次可读取的字节数 */
        int copy_size = BLOCK_SIZE - blk_offset;
        if (copy_size > (read_size - bytes_read))
        {
            copy_size = read_size - bytes_read;
        }

        /* 拷贝数据到用户缓冲区 */
        memcpy(buf + bytes_read, block_data + blk_offset, copy_size);

        /* 更新状态 */
        bytes_read += copy_size;
        offset += copy_size;
    }
    inode->data->mtime = sys_time(NULL);
    return bytes_read;
}
/**
 * @brief 向inode的数据块写入内容
 * @param inode 文件inode指针
 * @param buf 输入缓冲区
 * @param buf_size 缓冲区大小
 * @param whence 写入起始偏移量
 * @param write_size 要写入的字节数
 * @return 成功返回实际写入的字节数，失败返回-1
 */
int minix_inode_write(minix_inode_t *inode, const char *buf, int buf_size, int whence, int write_size)
{
    if (!inode || !buf || buf_size <= 0 || whence < 0 || write_size < 0)
    {
        return -1;
    }

    minix_inode_desc_t *minix_inode = inode->data;
    dev_t dev = inode->base.dev;
    int major = MAJOR(dev);
    int minor = MINOR(dev);

    /* 调整写入大小防止越界 */
    if (write_size > buf_size)
    {
        write_size = buf_size;
    }
    if (write_size <= 0)
    {
        return 0;
    }

    /* 逐块写入 */
    int bytes_written = 0;
    char *block_data;
    int offset = whence;

    while (bytes_written < write_size)
    {
        /* 计算当前逻辑块和块内偏移 */
        uint32_t fblock = offset / BLOCK_SIZE;
        uint32_t blk_offset = offset % BLOCK_SIZE;

        /* 获取物理块号（必要时创建新块） */
        int phys_block = inode_get_block(inode, fblock, true);
        if (phys_block <= 0)
        {
            break;
        }

        /* 如果是部分块写入，需要先读取原有数据 */

        buffer_t *blk_buf = fs_read_to_buffer(major, minor, phys_block, true);
        if (!blk_buf)
            break;
        block_data = blk_buf->data;

        /* 计算本次可写入的字节数 */
        int copy_size = BLOCK_SIZE - blk_offset;
        if (copy_size > (write_size - bytes_written))
        {
            copy_size = write_size - bytes_written;
        }

        /* 拷贝数据到块缓冲区 */
        memcpy(block_data + blk_offset, buf + bytes_written, copy_size);

        /* 更新状态 */
        bytes_written += copy_size;
        offset += copy_size;

        /* 扩展文件大小（如果需要） */
        if (offset > minix_inode->size)
        {
            minix_inode->size = offset;
            inode->data->mtime = sys_time(NULL); /* 更新修改时间 */
        }
    }

    inode->data->mtime = sys_time(NULL);
    return bytes_written;
}

/**
 * @brief 截断inode到指定大小
 * @param inode 要截断的inode
 * @param new_size 新的文件大小
 * @return 成功返回0，失败返回-1
 */
int minix_inode_truncate(minix_inode_t *inode, uint32_t new_size)
{
    if (!inode || !inode->data)
    {
        dbg_error("Invalid inode\r\r\n");
        return -1;
    }
    minix_inode_getinfo(inode, inode->base.dev, inode->base.nr);
    minix_inode_desc_t *minix_inode = inode->data;
    uint32_t old_size = minix_inode->size;
    dev_t dev = inode->base.dev;
    int major = MAJOR(dev);
    int minor = MINOR(dev);

    buffer_t *tmp_buf;
    // 如果新大小等于当前大小，直接返回
    if (new_size == old_size)
    {
        return 0;
    }

    // 如果是扩大的情况
    if (new_size > old_size)
    {
        // 计算需要增加的块数
        uint32_t old_blocks = (old_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        uint32_t new_blocks = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

        // 分配新的数据块
        for (uint32_t i = old_blocks; i < new_blocks; i++)
        {
            int blk = inode_get_block(inode, i, true);
            if (blk <= 0)
            {
                dbg_error("Failed to allocate block %d\r\r\n", i);
                // 回滚已分配的块？
                return -2;
            }
        }

        minix_inode->size = new_size;
        minix_inode->mtime = sys_time(NULL);
        return 0;
    }

    uint32_t old_blocks = (old_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint32_t new_blocks = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // 释放不再需要的块
    for (uint32_t i = new_blocks; i < old_blocks; i++)
    {
        uint32_t fblock = i;
        uint16_t *zone = minix_inode->zone;

        // 处理直接块 (0-6)
        if (fblock < 7)
        {
            if (zone[fblock] != 0)
            {
                minix_dblk_free(major, minor, zone[fblock]);
                zone[fblock] = 0;
            }
            continue;
        }

        // 处理一级间接块 (7)
        if (fblock < 7 + BLOCK_SIZE / sizeof(uint16_t))
        {
            if (zone[7] == 0)
                continue;

            uint16_t *indirect;
            buffer_t *indirect_buf = fs_read_to_buffer(major, minor, zone[7], true);
            ASSERT(indirect != NULL);
            indirect = indirect_buf->data;

            uint32_t idx = fblock - 7;
            if (indirect[idx] != 0)
            {
                minix_dblk_free(major, minor, indirect[idx]);
                indirect[idx] = 0;
            }
            continue;
        }
        // 处理二级间接块
        if (fblock >= 7 + BLOCK_SIZE / sizeof(uint16_t))
        {
            if (zone[8] == 0)
                continue;

            // 读取二级间接块
            buffer_t *double_indirect_buf = fs_read_to_buffer(major, minor, zone[8], true);
            if (!double_indirect_buf)
            {
                dbg_error("Failed to read double indirect block\r\r\n");
                continue;
            }
            uint16_t *double_indirect = double_indirect_buf->data;

            // 计算一级间接块的索引
            int idx1 = (fblock - (7 + BLOCK_SIZE / sizeof(uint16_t))) / (BLOCK_SIZE / sizeof(uint16_t));
            int idx2 = (fblock - (7 + BLOCK_SIZE / sizeof(uint16_t))) % (BLOCK_SIZE / sizeof(uint16_t));

            // 检查一级间接块是否存在
            if (double_indirect[idx1] == 0)
                continue;

            // 读取一级间接块
            buffer_t *indirect_buf = fs_read_to_buffer(major, minor, double_indirect[idx1], true);
            if (!indirect_buf)
            {
                dbg_error("Failed to read indirect block\r\r\n");
                continue;
            }
            uint16_t *indirect = indirect_buf->data;

            // 释放数据块
            if (indirect[idx2] != 0)
            {
                minix_dblk_free(major, minor, indirect[idx2]);
                indirect[idx2] = 0;
            }

            // 检查一级间接块是否为空
            bool is_indirect_empty = true;
            for (int i = 0; i < BLOCK_SIZE / sizeof(uint16_t); i++)
            {
                if (indirect[i] != 0)
                {
                    is_indirect_empty = false;
                    break;
                }
            }

            // 如果一级间接块为空，释放它
            if (is_indirect_empty)
            {
                minix_dblk_free(major, minor, double_indirect[idx1]);
                double_indirect[idx1] = 0;
            }

            // 检查二级间接块是否为空
            bool is_double_indirect_empty = true;
            for (int i = 0; i < BLOCK_SIZE / sizeof(uint16_t); i++)
            {
                if (double_indirect[i] != 0)
                {
                    is_double_indirect_empty = false;
                    break;
                }
            }

            // 如果二级间接块为空，释放它
            if (is_double_indirect_empty)
            {
                minix_dblk_free(major, minor, zone[8]);
                zone[8] = 0;
            }
        }
    }

    // 更新inode信息
    minix_inode->size = new_size;
    minix_inode->mtime = sys_time(NULL);
    return 0;
}
inode_opts_t minix_inode_opts = {

};