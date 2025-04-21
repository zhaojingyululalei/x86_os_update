
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
#include "fs/stat.h"
#include "fs/dirent.h"
/**
 * @brief 分配一个inode并加入树中
 */
static minix_inode_t *minix_inode_create(dev_t dev, int nr)
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
static void minix_inode_destory(minix_inode_t *inode)
{
    rb_tree_t *inode_tree = get_inode_tree();
    rb_tree_remove(inode_tree, inode);
    kfree(inode);
}

/**
 * @brief 去树中查找inode，找不到返回null
 */
static minix_inode_t *minix_inode_find(dev_t dev, int nr)
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

/**
 * @brief 在该分区，找一个空闲的inode节点
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回分配的i节点号，失败返回-1
 */
static int minix_inode_alloc(dev_t dev)
{
    int major = MAJOR(dev);
    int minor = MINOR(dev);
    minix_super_t *sup_blk = minix_get_super(dev);
    if (!sup_blk)
    {
        dbg_error("get major=%d,minor=%d super desc failed\r\n", major, minor);
        return -1;
    }

    // 检查超级块是否有效
    if (sup_blk->data == NULL || sup_blk->inode_map == NULL)
    {
        return -1;
    }

    // i节点总数
    uint16_t inodes = sup_blk->data->inodes;
    // i节点位图字节数
    uint16_t imap_bytes = (inodes + 7) / 8;

    // 在位图中查找第一个空闲i节点(从1开始，0是无效节点号)
    for (uint16_t i = 0; i < imap_bytes; i++)
    {
        if (sup_blk->inode_map[i] != 0xFF)
        { // 不是全1，说明有空闲i节点
            // 查找第一个为0的位
            for (int j = 0; j < 8; j++)
            {
                if (!(sup_blk->inode_map[i] & (1 << j)))
                {
                    uint16_t ino = i * 8 + j; // i节点号从1开始

                    // 检查i节点号是否有效
                    if (ino > inodes)
                    {
                        return -1; // 无效的i节点号
                    }

                    // 标记该i节点为已使用
                    sup_blk->inode_map[i] |= (1 << j);
                    if (ino == 0)
                    {
                        continue;
                    }
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
static int minix_inode_free(dev_t dev, int ino)
{
    int major = MAJOR(dev);
    int minor = MINOR(dev);
    minix_super_t *sup_blk = minix_get_super(dev);
    if (!sup_blk)
    {
        dbg_error("get major=%d,minor=%d super desc failed\r\n", major, minor);
        return -1;
    }

    // 检查超级块是否有效
    if (sup_blk->data == NULL || sup_blk->inode_map == NULL)
    {
        return -1;
    }

    // 检查i节点号是否有效
    if (ino < 1 || ino > sup_blk->data->inodes)
    {
        return -1;
    }

    // 计算位图中的位置
    uint16_t bit_offset = ino; // i节点号从1开始，位图从0开始
    uint16_t byte_offset = bit_offset / 8;
    uint8_t bit_mask = 1 << (bit_offset % 8);

    // 检查该i节点是否已经被释放
    if (!(sup_blk->inode_map[byte_offset] & bit_mask))
    {
        return -1; // 该i节点已经是空闲状态
    }

    // 清除位图中的位
    sup_blk->inode_map[byte_offset] &= ~bit_mask;
    return 0;
}
static minix_inode_t *minix_inode_open(dev_t dev, int nr)
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
        inode->base.type = FS_MINIX;
        return inode;
    }
    inode = minix_inode_create(dev, nr);
    ret = minix_inode_getinfo(inode, dev, nr);
    if (ret < 0)
    {
        dbg_error("inode map not alloc\r\n");
        return NULL;
    }
    inode->base.dev = dev;
    inode->base.nr = nr;
    inode->base.ops = &minix_inode_opts;
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
static int minix_inode_read(minix_inode_t *inode, char *buf, int buf_size, int whence, int read_size)
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
static int minix_inode_write(minix_inode_t *inode, const char *buf, int buf_size, int whence, int write_size)
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
static int minix_inode_truncate(minix_inode_t *inode, uint32_t new_size)
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

/**
 * @brief 给一个目录类型的inode，找到目录下的name
 * @param name为null就按照nr找，nr为0就按照name找，全部为0报错
 * @return 找到返回0  没找到返回<0
 */
static int find_entry(minix_inode_t *inode, const char *name, struct dirent *entry)
{
    if (!inode || !entry)
    {
        return -1; // 参数错误
    }
    int ret;
    if (!name)
    {
        return -2; // 参数错误，name和nr不能同时为0
    }
    mode_t mode = inode->data->mode;
    // 如果不是目录，找不了
    if (!ISDIR(mode))
    {
        dbg_error("current inode not dir\r\n");
        return -1;
    }
    // 读取目录内容
    int entry_nr = inode->data->size / sizeof(minix_dentry_t);
    minix_dentry_t *start = (minix_dentry_t *)kmalloc(inode->data->size);

    minix_inode_read(inode, start, inode->data->size, 0, inode->data->size);
    if (ret <= 0)
    {
        return -3; // 读取失败
    }

    // 遍历目录项
    for (int i = 0; i < entry_nr; i++, start++)
    {
        if (strncmp(start->name, name, strlen(name)) == 0)
        {
            strcpy(entry->name, start->name);
            entry->nr = start->nr;
            kfree(start);
            return i;
        }
    }
    kfree(start);
    return -4; // 未找到
}
/**
 * @brief 向inode节点中添加一个条目
 */
static int add_entry(minix_inode_t *inode, struct dirent *dentry)
{
    uint16_t mode = inode->data->mode;
    // 如果不是目录，找不了
    if (!ISDIR(mode))
    {
        dbg_error("current inode not dir\r\n");
        return -1;
    }
    const char *name = dentry->name;
    int ino = dentry->nr;
    minix_dentry_t *entry = (minix_dentry_t *)kmalloc(sizeof(minix_dentry_t));
    int entry_nr = inode->data->size / sizeof(minix_dentry_t);
    int whence = entry_nr * sizeof(minix_dentry_t);
    strncpy(entry->name, name, strlen(name));
    entry->nr = ino;
    // 添加目录项
    int ret = minix_inode_write(inode, entry, sizeof(minix_dentry_t), whence, sizeof(minix_dentry_t));
    if (ret < 0)
    {
        dbg_error("write fail\r\n");
        kfree(entry);
        return -2;
    }
    kfree(entry);
    return 0;
}

static int delete_entry(minix_inode_t *inode, struct dirent *entry)
{
    int ret = find_entry(inode, entry->name, NULL);
    if (ret < 0)
    {
        dbg_error("Entry not found\r\n");
        return -1;
    }
    // 从目录中删除条目
    int entry_size = sizeof(minix_dentry_t);
    int entry_count = inode->data->size / entry_size;
    if (ret == entry_count - 1)
    {
        // 如果是最后一个条目，直接截断
        inode->data->size -= entry_size;
    }
    // 否则用最后一个条目覆盖
    else
    {
        minix_dentry_t last_entry;
        if (minix_inode_read(inode, (char *)&last_entry, entry_size,
                             (entry_count - 1) * entry_size, entry_size) < 0)
        {
            return -7;
        }

        if (minix_inode_write(inode, (char *)&last_entry, entry_size,
                              ret * entry_size, entry_size) < 0)
        {
            return -8;
        }

        inode->data->size -= entry_size;
    }
    return 0;
}
/**
 * @brief 获取一个分区的根目录inode
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回根目录的inode指针，失败返回NULL
 */
minix_inode_t *minix_get_dev_root_inode(dev_t dev)
{
    int ret;
    // 获取超级块
    int major = MAJOR(dev);
    int minor = MINOR(dev);
    minix_super_t *sup_blk = minix_get_super(dev);
    if (!sup_blk || !sup_blk->data || !sup_blk->inode_map)
    {
        dbg_error("Failed to get super block or inode map\r\n");
        return NULL;
    }

    // 根目录的 inode 号通常是 1
    uint16_t root_ino = 1;

    // 检查根目录 inode 是否已分配
    uint16_t byte_offset = root_ino / 8;
    uint8_t bit_mask = 1 << (root_ino % 8);
    if (!(sup_blk->inode_map[byte_offset] & bit_mask))
    {
        dbg_error("Root inode not allocated\r\n");
        return NULL;
    }

    // 创建并返回根目录 inode 描述符
    minix_inode_t *root_inode = minix_inode_open(dev, root_ino);

    if (!root_inode)
    {
        dbg_error("Failed to find root inode\r\n");
        return NULL;
    }
    ret = minix_inode_getinfo(root_inode, dev, root_ino);
    if (ret < 0)
    {
        dbg_error("get info failz\r\n");
        return NULL;
    }
    if (strlen(root_inode->base.name) == 0)
    {
        strcpy(root_inode->base.name, "/");
    }
    return root_inode;
}
inode_opts_t minix_inode_opts = {
    .find_entry = find_entry,                       // 查找目录项
    .add_entry = add_entry,                         // 添加目录项
    .delete_entry = delete_entry,                   // 删除目录项
    .read = minix_inode_read,                       // 读取文件内容
    .write = minix_inode_write,                     // 写入文件内容
    .truncate = minix_inode_truncate,               // 截断文件
    .getinfo = minix_inode_getinfo,                 // 获取inode磁盘缓冲区中的信息
    .create = minix_inode_create,                   // 创建inode并入树
    .destroy = minix_inode_destory,                 // 从树中移除并释放inode_t结构体
    .free = minix_inode_free,                       // 位图释放inode
    .alloc = minix_inode_alloc,                     // 位图分配inode
    .open = minix_inode_open,                       // 打开并返回inode
    .get_dev_root_inode = minix_get_dev_root_inode, // 获取minix文件系统根目录inode的方式
};
