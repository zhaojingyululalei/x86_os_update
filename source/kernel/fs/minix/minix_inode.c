#include "fs/minix_inode.h"
#include "fs/minix_super.h"
#include "printk.h"
#include "mem/kmalloc.h"
#include "string.h"
#include "cpu_cfg.h"
#include "fs/buffer.h"
#include "time/time.h"
#include "fs/minix_entry.h"
#include "task/sche.h"
#include "fs/stat.h"
#include "fs/fs.h"
#include "task/task.h"
#include "fs/mount.h"
static rb_tree_t inode_tree;

static int minix_inode_compare(void *a, void *b)
{
    minix_inode_desc_t *ia = a;
    minix_inode_desc_t *ib = b;
    if (ia->idx != ib->idx)
    {
        return ia->idx - ib->idx;
    }
    if (ia->major != ib->major)
    {
        return ia->major - ib->major;
    }
    if (ia->minor != ib->minor)
    {
        return ia->minor - ib->minor;
    }
    return 0;
}
static rb_node_t *inode_get_rbnode(const void *data)
{
    minix_inode_desc_t *inode = data;
    return &inode->rbnode;
}

static minix_inode_desc_t *rbnode_get_inode(rb_node_t *node)
{
    return rb_node_parent(node, minix_inode_desc_t, rbnode);
}
/**
 * @brief 分配一个空闲数据块，返回块号,已经考虑了first_datablk
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回分配的数据块号，失败返回-1
 */
int minix_dblk_alloc(int major, int minor)
{
    minix_super_desc_t *sup_blk = minix_get_super_desc(major, minor);
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
    minix_super_desc_t *sup_blk = minix_get_super_desc(major, minor);
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
void minix_inode_tree_init(void)
{
    rb_tree_init(&inode_tree, minix_inode_compare, inode_get_rbnode, rbnode_get_inode, NULL);
}
/**
 * @brief 在该分区，找一个空闲的inode节点
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回分配的i节点号，失败返回-1
 */
int minix_inode_alloc(int major, int minor)
{
    minix_super_desc_t *sup_blk = minix_get_super_desc(major, minor);
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
int minix_inode_free(int major, int minor, int ino)
{
    minix_super_desc_t *sup_blk = minix_get_super_desc(major, minor);
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
/**
 * @brief 获取磁盘中的inode数据
 */
minix_inode_t *get_dev_inode_data(int major, int minor, int ino)
{
    // 参数检查
    if (ino < 1)
    {
        dbg_error("param fault\r\r\n");
        return -1;
    }

    minix_super_desc_t *sup_blk = minix_get_super_desc(major, minor);
    if (!sup_blk)
    {
        dbg_error("get major=%d,minor=%d super desc failed\r\n", major, minor);
        return NULL;
    }

    // 检查超级块是否有效
    if (sup_blk->data == NULL || sup_blk->zone_map == NULL)
    {
        dbg_error("param fault\r\n");
        return -1;
    }

    // 检查inode号是否有效
    if (ino > sup_blk->data->inodes)
    {
        dbg_error("param fault\r\r\n");
        return -1;
    }
    // 检查inode是否已分配
    uint16_t byte_offset = ino / 8;
    uint8_t bit_mask = 1 << (ino % 8);
    if (!(sup_blk->inode_map[byte_offset] & bit_mask))
    {
        dbg_error("param fault\r\r\n");
        return -1; // inode未分配
    }

    // 计算inode表起始块号
    uint32_t inode_table_start = 2 + sup_blk->data->imap_blocks + sup_blk->data->zmap_blocks;

    // 计算inode大小
    uint32_t inode_size = sizeof(minix_inode_t);

    // 计算每个块能存放的inode数量
    uint32_t inodes_per_block = BLOCK_SIZE / inode_size; // 需要定义BLOCK_SIZE

    // 计算目标inode所在的块号
    uint32_t inode_block = inode_table_start + (ino - 1) / inodes_per_block;

    // 计算inode在块内的偏移
    uint32_t inode_offset = ((ino - 1) % inodes_per_block) * inode_size;

    // 读取包含目标inode的块
    buffer_t *inode_buf = fs_read_to_buffer(major, minor, inode_block, true);
    if (inode_buf == NULL)
    {

        dbg_error("read to buffer failed\r\n");
        return NULL;
    }
    minix_inode_t *inode_data = (minix_inode_t *)(inode_buf->data + inode_offset);
    return inode_data;
}
/**
 * @brief 在父目录下创建一个inode节点
 * @param parent 父目录的inode节点
 * @param name 要创建的inode节点的名称
 */
minix_inode_desc_t *minix_inode_create(minix_inode_desc_t *parent, const char *name)
{
    if (find_entry(parent, name, NULL) >= 0)
    {
        dbg_warning("parent=%p,name=%s already exist\r\n", parent, name);
        return NULL;
    }
    int major = parent->major;
    int minor = parent->minor;
    minix_inode_desc_t *inode = (minix_inode_desc_t *)kmalloc(sizeof(minix_inode_desc_t));
    int ino = minix_inode_alloc(parent->major, parent->minor);
    if (ino < 1)
    {
        dbg_error("alloc inode failed\r\n");
        kfree(inode);
        return NULL;
    }
    if (add_entry(parent, name, ino) < 0)
    {
        dbg_error("add entry failed\r\n");
        kfree(inode);
        return NULL;
    }

    minix_inode_t *inode_data = get_dev_inode_data(major, minor, ino);

    inode->data = inode_data;

    // 填充其他信息,并不对data区域修改
    inode->major = major;
    inode->minor = minor;
    inode->idx = ino;

    // 初始化时间(使用inode中的mtime作为基准)
    time_t now = sys_time(NULL);
    inode->atime = now;
    inode->data->mtime = inode->atime;

    inode->ctime = inode->data->mtime;
    inode->parent = parent;
    strcpy(inode->name, name);
    rb_tree_insert(&inode_tree, inode);
    return inode;
}
/**
 * @brief 从树中移除并释放inode
 */
void minix_inode_delete(minix_inode_desc_t *inode)
{
    rb_tree_remove(&inode_tree, inode);
    kfree(inode);
}
static int inode_compare_key(const void *key, const void *data)
{
    int *arr = (int *)key;
    minix_inode_desc_t *ib = (minix_inode_desc_t *)data;
    int major = arr[0];
    int minor = arr[1];
    int ino = arr[2];
    if (ino != ib->idx)
    {
        return ino - ib->idx;
    }
    if (major != ib->major)
    {
        return major - ib->major;
    }
    if (minor != ib->minor)
    {
        return minor - ib->minor;
    }
    return 0;
}
bool minix_inode_is_free(int major,int minor,int ino)
{
    minix_super_desc_t *sup_blk = minix_get_super_desc(major, minor);
    if (!sup_blk)
    {
        dbg_error("get major=%d,minor=%d super desc failed\r\n", major, minor);
        return false;
    }

    // 检查超级块是否有效
    if (sup_blk->data == NULL || sup_blk->inode_map == NULL)
    {
        return false;
    }

    // 检查i节点号是否有效
    if (ino < 1 || ino > sup_blk->data->inodes)
    {
        return false;
    }

    // 计算位图中的位置
    uint16_t bit_offset = ino; // i节点号从1开始，位图从0开始
    uint16_t byte_offset = bit_offset / 8;
    uint8_t bit_mask = 1 << (bit_offset % 8);

    // 检查该i节点是否已经被释放
    if (!(sup_blk->inode_map[byte_offset] & bit_mask))
    {
        return true; // 该i节点已经是空闲状态
    }

   
    return false;
}
/**
 * @brief 查找inode，没找到是否创建inode
 */
minix_inode_desc_t *minix_inode_find(int major, int minor, int ino)
{
    //位图中不存在，肯定是没有
    if(minix_inode_is_free(major,minor,ino))
    {
        return NULL;
    }
    //为图中存在，肯定是有，没有inode_desc就创建，有就直接返回
    int arr[3] = {major, minor, ino};
    minix_inode_desc_t *inode = NULL;
    rb_node_t *rbnode = rb_tree_find_by(&inode_tree, arr, inode_compare_key);
    if (rbnode == inode_tree.nil)
    {
        // dbg_warning("inode not found\r\n");
        // 没找到就创建一个并加入树中
        inode = (minix_inode_desc_t *)kmalloc(sizeof(minix_inode_desc_t));
        inode->major = major;
        inode->minor = minor;
        inode->idx = ino;
        rb_tree_insert(&inode_tree, inode);
    }
    else
    {
        inode = rbnode_get_inode(rbnode);
    }
    // data区域重新读，缓存一致性
    if (inode)
    {

        minix_inode_t *inode_data = get_dev_inode_data(major, minor, ino);
        inode->data = inode_data;
        return inode;
    }
    return NULL;
}

/**
 * @brief 获取文件块在分区中的块号
 * @param fblock:该文件块块号。例如文件大小4KB，总共4块，每块2个扇区，每个扇区512字节。要获取它第二块所在分区的块号
 * @param create:如果这个fblock超过了该文件大小，也就是不存在，是否创建一个数据块。
 * @return 返回该文件块在分区中的块号
 */
static int inode_get_block(minix_inode_desc_t *inode, int fblock, bool create)
{
    // 参数检查
    if (!inode || !inode->data || fblock < 0)
    {
        return -1;
    }

    minix_inode_t *minix_inode = inode->data;
    int major = inode->major;
    int minor = inode->minor;

    // 1. 直接块处理 (0-6)
    if (fblock < 7)
    {
        if (minix_inode->zone[fblock] != 0)
        {
            return minix_inode->zone[fblock]; // 已有物理块
        }

        if (create)
        {
            int new_blk = minix_dblk_alloc(major, minor);
            if (new_blk > 0)
            {
                minix_inode->zone[fblock] = new_blk;
                inode->data->mtime = sys_time(NULL); // 更新修改时间
                return new_blk;
            }
        }
        return -1;
    }

    // 2. 一级间接块处理
    int indirect_limit = 7 + BLOCK_SIZE / sizeof(uint16_t);
    if (fblock < indirect_limit)
    {
        // 检查间接块是否存在
        if (minix_inode->zone[7] == 0)
        {
            if (!create)
                return -1;

            // 分配间接块
            minix_inode->zone[7] = minix_dblk_alloc(major, minor);
            if (minix_inode->zone[7] == 0)
                return -1;
        }

        // 读取间接块
        buffer_t *indirect_buf = fs_read_to_buffer(major, minor, minix_inode->zone[7], true);
        if (!indirect_buf)
        {
            dbg_error("read buffer fail\r\r\n");
            return -2;
        }
        uint16_t *indirect_block = indirect_buf->data;

        int idx = fblock - 7;
        if (indirect_block[idx] != 0)
        {
            return indirect_block[idx]; // 已有物理块
        }

        if (create)
        {
            int new_blk = minix_dblk_alloc(major, minor);
            if (new_blk > 0)
            {
                indirect_block[idx] = new_blk;

                inode->data->mtime = sys_time(NULL);
                return new_blk;
            }
        }
        return -1;
    }

    // 3. 二级间接块处理
    // ...
    dbg_error("not handle 2-level indirect_block\r\r\n");
    // 超出文件最大范围
    return -1;
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
int minix_inode_read(minix_inode_desc_t *inode, char *buf, int buf_size, int whence, int read_size)
{
    if (!inode || !buf || buf_size <= 0 || whence < 0 || read_size < 0)
    {
        return -1;
    }

    minix_inode_t *minix_inode = inode->data;
    int major = inode->major;
    int minor = inode->minor;

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
    inode->atime = sys_time(NULL);
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
int minix_inode_write(minix_inode_desc_t *inode, const char *buf, int buf_size, int whence, int write_size)
{
    if (!inode || !buf || buf_size <= 0 || whence < 0 || write_size < 0)
    {
        return -1;
    }

    minix_inode_t *minix_inode = inode->data;
    int major = inode->major;
    int minor = inode->minor;

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

    inode->atime = sys_time(NULL);
    return bytes_written;
}

/**
 * @brief 截断inode到指定大小
 * @param inode 要截断的inode
 * @param new_size 新的文件大小
 * @return 成功返回0，失败返回-1
 */
int minix_inode_truncate(minix_inode_desc_t *inode, uint32_t new_size)
{
    if (!inode || !inode->data)
    {
        dbg_error("Invalid inode\r\r\n");
        return -1;
    }
    inode->data = get_dev_inode_data(inode->major, inode->minor, inode->idx);
    minix_inode_t *minix_inode = inode->data;
    uint32_t old_size = minix_inode->size;
    int major = inode->major;
    int minor = inode->minor;
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
        // 无需处理二级间接块
        dbg_error("file too large\r\r\n");
    }

    // 更新inode信息
    minix_inode->size = new_size;
    minix_inode->mtime = sys_time(NULL);
    return 0;
}

#include "fs/path.h"
/**
 * @brief 获取path文件对应的inode节点
 * @param path 文件路径
 * @return 成功返回inode指针，失败返回NULL
 */
minix_inode_desc_t *minix_namei(const char *path)
{
    if (path == NULL || *path == '\0')
    {
        return NULL;
    }

    task_t *cur = cur_task();
    PathParser *parse = path_parser_init(path);
    if (parse == NULL)
    {
        return NULL;
    }

    char *component = path_parser_next(parse);
    minix_inode_desc_t *parent_inode = NULL;
    minix_inode_desc_t *current_inode = NULL;

    // 确定起始inode
    if (is_rootdir(component))
    {
        // 如果是绝对路径
        parent_inode = get_root_inode();
    }
    else
    {
        // 如果是相对路径
        parent_inode = cur->ipwd;
    }

    // 遍历路径组件
    while ((component = path_parser_next(parse)) != NULL)
    {
        minix_dentry_t dentry;
        int ret = find_entry(parent_inode, component, &dentry);
        if (ret < 0)
        {
            path_parser_free(parse);
            return NULL; // 路径组件不存在
        }
        // 获取当前组件的inode
        if (dentry.nr == 1)
        {
            // 需要去挂载列表里查找挂载点
            // 获取path的绝对路径
            char *parent_abs_path = minix_inode_to_absolute_path(parent_inode);
            char *child_name = dentry.name;
            char *cwd = path_join(parent_abs_path, child_name);
            // 去mountlist中找挂载dian
            mount_point_t *point = find_point_by_path(cwd);
            current_inode = minix_inode_find(point->major, point->minor, 1);
        }
        else
        {
            // 直接去找某个设备的某个ino就可以
            current_inode = minix_inode_find(parent_inode->major, parent_inode->minor, dentry.nr);
        }

        if (current_inode == NULL)
        {
            path_parser_free(parse);
            return NULL;
        }

        current_inode->parent = parent_inode;
        strcpy(current_inode->name, component);

        // 更新父inode
        if (parent_inode != cur->ipwd && parent_inode != get_root_inode())
        {
            kfree(parent_inode);
        }
        parent_inode = current_inode;
    }

    path_parser_free(parse);
    return current_inode ? current_inode : parent_inode;
}

/**
 * @brief 获取父目录的inode节点
 */
/**
 * @brief 获取父目录的inode节点
 * @param inode 当前inode节点
 * @return 成功返回父目录的inode节点，失败返回NULL
 */
minix_inode_desc_t *minix_get_parent(minix_inode_desc_t *inode)
{
    if (inode->parent)
    {
        return inode->parent; // 如果父目录已缓存，直接返回
    }
    if (inode->idx == 1)
    {
        dbg_error("un handler\r\n");
    }
    // 获取超级块
    minix_super_desc_t *sup_blk = minix_get_super_desc(inode->major, inode->minor);
    if (!sup_blk || !sup_blk->data || !sup_blk->inode_map)
    {
        dbg_error("Failed to get super block or inode map\r\n");
        return NULL;
    }

    // 遍历所有 inode 节点
    uint16_t inodes = sup_blk->data->inodes;
    uint16_t imap_bytes = (inodes + 7) / 8;

    for (uint16_t i = 0; i < imap_bytes; i++)
    {
        if (sup_blk->inode_map[i] != 0xFF)
        { // 不是全1，说明有空闲 inode
            for (int j = 0; j < 8; j++)
            {
                if (sup_blk->inode_map[i] & (1 << j))
                {                             // 检查 inode 是否已分配
                    uint16_t ino = i * 8 + j; // inode 号从1开始

                    // 跳过当前 inode
                    if (ino == inode->idx)
                    {
                        continue;
                    }

                    // 获取 inode 数据
                    minix_inode_t *inode_data = get_dev_inode_data(inode->major, inode->minor, ino);
                    if (!inode_data)
                    {
                        continue;
                    }

                    // 检查是否是目录
                    if (!ISDIR(inode_data->mode))
                    {
                        continue;
                    }

                    // 读取目录项
                    minix_dentry_t *entries = kmalloc(inode_data->size);
                    if (!entries)
                    {
                        dbg_error("Memory allocation failed\r\n");
                        continue;
                    }

                    if (minix_inode_read(inode, (char *)entries, inode_data->size, 0, inode_data->size) < 0)
                    {
                        dbg_error("Failed to read directory entries\r\n");
                        kfree(entries);
                        continue;
                    }

                    // 遍历目录项，查找当前 inode
                    int entry_count = inode_data->size / sizeof(minix_dentry_t);
                    for (int k = 0; k < entry_count; k++)
                    {
                        if (entries[k].nr == inode->idx)
                        {
                            // 找到父目录
                            minix_inode_desc_t *parent_inode = minix_inode_find(inode->major, inode->minor, ino);
                            inode->parent = parent_inode;
                            strcpy(inode->name, entries[k].name);
                            kfree(entries);
                            return parent_inode;
                        }
                    }

                    kfree(entries);
                }
            }
        }
    }

    dbg_warning("Parent directory not found for inode %d\r\n", inode->idx);
    return NULL; // 未找到父目录
}
minix_inode_desc_t *minix_named(const char *path)
{
    if (path == NULL || *path == '\0')
    {
        return NULL;
    }
    if (strcmp(path, ".") == 0)
    {
        return minix_get_parent(cur_task()->ipwd);
    }
    char *parent = path_dirname(path);
    if (strcmp(parent, ".") == 0)
    {
        return cur_task()->ipwd;
    }
    return minix_namei(parent);
}
/**
 * @brief 打印改inode的所有文件信息，包括mode，nlinks等
 */
void stat_inode(minix_inode_desc_t *inode)
{
    if (!inode || !inode->data)
    {
        dbg_error("Invalid inode\r\r\n");
        return;
    }

    minix_inode_t *minix_inode = inode->data;

    // 打印文件类型
    char type = ISDIR(minix_inode->mode) ? 'D' : 'F';
    dbg_info("Type: %c\r\r\n", type);

    // 打印权限
    print_mode(minix_inode->mode);

    // 打印硬链接数
    dbg_info("Links: %d\r\r\n", minix_inode->nlinks);

    // 打印文件大小
    dbg_info("Size: %d bytes\r\r\n", minix_inode->size);

    // 打印时间信息
    dbg_info("Access Time: %d\r\r\n", inode->atime);
    dbg_info("Modification Time: %d\r\r\n", inode->data->mtime);
    dbg_info("Creation Time: %d\r\r\n", inode->ctime);
}
/**
 * @brief 该inode必须是目录，打印改目录下所有文件信息，包括mode，nlinks等。不递归
 */
void ls_inode(minix_inode_desc_t *inode)
{
    if (!inode || !inode->data)
    {
        dbg_error("Invalid inode\r\n");
        return;
    }

    // 检查是否是目录
    if (!ISDIR(inode->data->mode))
    {
        dbg_error("Not a directory\r\n");
        return;
    }

    // 计算目录条目数量
    int entry_count = inode->data->size / sizeof(minix_dentry_t);
    if (entry_count <= 0)
    {
        dbg_info("Empty directory\r\n");
        return;
    }

    // 读取所有目录条目
    minix_dentry_t *entries = kmalloc(inode->data->size);
    if (!entries)
    {
        dbg_error("Memory allocation failed\r\n");
        return;
    }

    if (minix_inode_read(inode, (char *)entries, inode->data->size,
                         0, inode->data->size) < 0)
    {
        dbg_error("Failed to read directory entries\r\n");
        kfree(entries);
        return;
    }

    // 打印表头
    dbg_info("Total %d entries:\r\n", entry_count);
    dbg_info("Inode\tName\tType\tSize\tLinks\r\n");
    dbg_info("--------------------------------\r\n");

    // 打印每个条目
    for (int i = 0; i < entry_count; i++)
    {
        // 获取条目 inode 信息
        minix_inode_desc_t *entry_inode = minix_inode_find(inode->major, inode->minor, entries[i].nr);
        if (!entry_inode)
        {
            continue;
        }

        // 打印条目信息
        char type = ISDIR(entry_inode->data->mode) ? 'D' : 'F';
        dbg_info("%d\t%s\t%c\t%d\t%d\r\n",
                 entries[i].nr,
                 entries[i].name,
                 type,
                 entry_inode->data->size,
                 entry_inode->data->nlinks);
    }

    kfree(entries);
}
mount_point_t* get_mount_point(minix_inode_desc_t* parent,const char* entry_name)
{
    char* abs = minix_inode_to_absolute_path(parent);
    char* path = path_join(abs,entry_name);
    mount_point_t* point = find_point_by_path(path);
   return point;
}
/**
 * @brief 打印改inode的所有文件信息，包括mode，nlinks等，递归(如果是目录则递归)
 */
void tree_inode(minix_inode_desc_t *inode, int level)
{
    if (!inode || !inode->data)
    {
        dbg_error("Invalid inode\r\n");
        return;
    }

    // 打印当前 inode 信息
    for (int i = 0; i < level; i++)
    {
        dbg_info("│   "); // 缩进符号
    }

    // 判断是否是最后一个条目
    if (level > 0)
    {
        dbg_info("├── "); // 子条目前缀
    }

    // 打印文件类型和名称
    char type = ISDIR(inode->data->mode) ? 'D' : 'F';
    dbg_info("[%c] %s (nlinks: %d)\r\n", type, inode->name,inode->data->nlinks);

    // 如果是目录，递归打印子目录和文件
    if (ISDIR(inode->data->mode))
    {
        int entry_count = inode->data->size / sizeof(minix_dentry_t);
        if (entry_count <= 0)
        {
            return;
        }

        minix_dentry_t *entries = kmalloc(inode->data->size);
        if (!entries)
        {
            dbg_error("Memory allocation failed\r\n");
            return;
        }

        if (minix_inode_read(inode, (char *)entries, inode->data->size,
                             0, inode->data->size) < 0)
        {
            dbg_error("Failed to read directory entries\r\n");
            kfree(entries);
            return;
        }

        for (int i = 0; i < entry_count; i++)
        {
            // 跳过 . 和 ..
            if (strcmp(entries[i].name, ".") == 0 ||
                strcmp(entries[i].name, "..") == 0)
            {
                continue;
            }
            minix_inode_desc_t *child_inode;
            if(entries[i].nr == 1)
            {
                //如果这是个挂载点，获取挂载点信息
                mount_point_t* point = get_mount_point(inode,entries[i].name);
                child_inode = minix_inode_find(point->major, point->minor, entries[i].nr);
            }
            else{
                child_inode = minix_inode_find(inode->major, inode->minor, entries[i].nr);
            }
            
            if (!child_inode)
            {
                continue;
            }

            tree_inode(child_inode, level + 1); // 递归打印
        }

        kfree(entries);
    }
}

minix_inode_desc_t *minix_inode_open(const char *path, int flag, int mode)
{
    time_t *now = sys_time(NULL);
    minix_inode_desc_t *inode = minix_namei(path);
    if (!inode)
    {
        if (flag & O_CREAT)
        {
            // 解析路径，获取父目录的 inode
            minix_inode_desc_t *parent_inode = minix_named(path);
            if (!parent_inode)
            {
                dbg_error("Parent directory not found\r\n");
                return -1;
            }

            // 获取新文件的名称
            char *dir_name = path_basename(path);
            if (!dir_name)
            {
                dbg_error("Invalid directory name\r\n");
                return -1;
            }

            minix_inode_desc_t *new_inode = minix_inode_create(parent_inode, dir_name);
            if (!new_inode)
            {
                dbg_error("Failed to create file\r\n");
                return NULL;
            }

            uint16_t mask = cur_task()->umask;
            mode = mode & ~mask;
            new_inode->data->mode = IFREG | mode; // 设置目录类型和权限
            new_inode->data->nlinks = 1;          //
            new_inode->data->size = 0;            // 初始大小为0
            new_inode->parent = parent_inode;
            new_inode->ctime = now;
            strcpy(new_inode->name, dir_name);
            inode->ref = 0;
            inode = new_inode;
        }
        else
        {
            dbg_error("File not found\r\n");
            return NULL;
        }
    }

    inode->ref++;
    inode->atime = now;

    return inode;
}
/**
 * @brief 获取一个分区的根目录inode
 * @param major 主设备号
 * @param minor 次设备号
 * @return 成功返回根目录的inode指针，失败返回NULL
 */
minix_inode_desc_t *minix_get_dev_root_inode(int major, int minor)
{
    // 获取超级块
    minix_super_desc_t *sup_blk = minix_get_super_desc(major, minor);
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

    // 获取根目录 inode 数据
    minix_inode_t *root_inode_data = get_dev_inode_data(major, minor, root_ino);
    if (!root_inode_data)
    {
        dbg_error("Failed to get root inode data\r\n");
        return NULL;
    }

    // 创建并返回根目录 inode 描述符
    minix_inode_desc_t *root_inode = minix_inode_find(major, minor, root_ino);
    if (!root_inode)
    {
        dbg_error("Failed to find root inode\r\n");
        return NULL;
    }
    root_inode->data = root_inode_data;
    if (strlen(root_inode->name) == 0)
    {
        strcpy(root_inode->name, "/");
    }
    return root_inode;
}

/**
 * @brief 获取 inode 对应的绝对路径
 * @param inode 目标 inode
 * @return 成功返回绝对路径字符串，失败返回NULL，需要调用者释放
 */
#define MAX_PATH_DEPTH 256
char *minix_inode_to_absolute_path(minix_inode_desc_t *inode)
{
    if (!inode)
    {
        dbg_error("Invalid inode\r\n");
        return NULL;
    }

    // 如果是根目录，直接返回 "/"
    if (inode->idx == 1)
    {
        return strdup("/");
    }

    // 动态数组存储路径组件
    char **components = kmalloc(MAX_PATH_DEPTH * sizeof(char *));
    if (!components)
    {
        dbg_error("Memory allocation failed\r\n");
        return NULL;
    }

    int depth = 0; // 路径深度
    minix_inode_desc_t *current = inode;

    // 递归遍历父目录
    while (current && current->idx != 1)
    { // 根目录的 inode 号为 1
        if (depth >= MAX_PATH_DEPTH)
        {
            dbg_error("Path depth exceeds limit\r\n");
            for (int i = 0; i < depth; i++)
            {
                kfree(components[i]);
            }
            kfree(components);
            return NULL;
        }

        // 存储当前 inode 的名称
        components[depth] = strdup(current->name);
        if (!components[depth])
        {
            dbg_error("Memory allocation failed\r\n");
            for (int i = 0; i < depth; i++)
            {
                kfree(components[i]);
            }
            kfree(components);
            return NULL;
        }

        depth++;
        current = minix_get_parent(current); // 获取父目录
    }

    // 计算路径总长度
    size_t total_len = 1; // 根目录的 "/"
    for (int i = depth - 1; i >= 0; i--)
    {
        total_len += strlen(components[i]) + 1; // 每个组件前加一个 "/"
    }

    // 分配路径字符串
    char *path = kmalloc(total_len);
    if (!path)
    {
        dbg_error("Memory allocation failed\r\n");
        for (int i = 0; i < depth; i++)
        {
            kfree(components[i]);
        }
        kfree(components);
        return NULL;
    }

    // 构建路径
    path[0] = '/'; // 根目录
    int pos = 1;
    for (int i = depth - 1; i >= 0; i--)
    {
        size_t len = strlen(components[i]);
        memcpy(path + pos, components[i], len);
        pos += len;
        if (i > 0)
        {
            path[pos++] = '/';
        }
    }
    path[pos] = '\0'; // 确保以空字符结尾

    // 释放组件数组
    for (int i = 0; i < depth; i++)
    {
        kfree(components[i]);
    }
    kfree(components);

    return path;
}

void insert_inode_to_tree(minix_inode_desc_t* inode)
{
    rb_tree_insert(&inode_tree,inode);
}
void remove_inode_from_tree(minix_inode_desc_t* inode)
{
    rb_tree_remove(&inode_tree,inode);
}