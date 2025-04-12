#include "fs/minix_fs.h"
#include "string.h"
#include "printk.h"
#include "fs/buffer.h"
/**
 * @brief 获取某个设备的某个inode
 * @note  inode表的第一个节点，其实是imap位图的第1个节点，而不是第0个，第0个是无效节点，在imap中，第0个节点永远为1
 */
int get_dev_inode(inode_t *inode, int major, int minor, int idx)
{
   
    // 参数检查
    if (inode == NULL || idx < 1)
    {
        dbg_error("param fault\r\n");
        return -1;
    }

    // 获取设备的超级块
    super_block_t sup_blk;
    if (get_dev_super_block(&sup_blk, major, minor) != 0)
    {
        dbg_error("param fault\r\n");
        return -1;
    }

    // 检查inode号是否有效
    if (idx > sup_blk.data->inodes)
    {
        dbg_error("param fault\r\n");
        return -1;
    }

  
    // 检查inode是否已分配
    uint16_t byte_offset = idx / 8;
    uint8_t bit_mask = 1 << (idx % 8);
    if (!(sup_blk.inode_map[byte_offset] & bit_mask))
    {
        dbg_error("param fault\r\n");
        return -1; // inode未分配
    }

    // 计算inode表起始块号
    uint32_t inode_table_start = 2 + sup_blk.data->imap_blocks + sup_blk.data->zmap_blocks;

    // 计算inode大小
    uint32_t inode_size = sizeof(minix_inode_t);

    // 计算每个块能存放的inode数量
    uint32_t inodes_per_block = BLOCK_SIZE / inode_size; // 需要定义BLOCK_SIZE

    // 计算目标inode所在的块号
    uint32_t inode_block = inode_table_start + (idx-1) / inodes_per_block;

    // 计算inode在块内的偏移
    uint32_t inode_offset = ((idx-1) % inodes_per_block) * inode_size;

    // 读取包含目标inode的块
    buffer_t *inode_buf = fs_read_to_buffer(major, minor, inode_block, true);
    minix_inode_t *inode_data = (minix_inode_t *)(inode_buf->data + inode_offset);

    inode->data = inode_data;

    // 填充其他信息
    inode->major = major;
    inode->minor = minor;
    inode->idx = idx ;

    // 初始化时间(使用inode中的mtime作为基准)
    time_t now = sys_time(NULL);
    inode->atime = now;
    inode->mtime = inode->data->mtime;
    // 创建时间，只要有了就不要动
    if (inode->ctime == 0)
    {
        inode->ctime = inode->data->mtime;
    }

    return 0;
}
/**
 * @brief 获取根目录的inode
 */
inode_t *get_root_inode(void)
{
    super_block_t *sup_blk = get_root_super_block();
    return sup_blk->iroot;
}

/**
 * @brief 获取文件块在分区中的块号
 * @param fblock:该文件块块号。例如文件大小4KB，总共4块，每块2个扇区，每个扇区512字节。要获取它第二块所在分区的块号
 * @param create:如果这个fblock超过了该文件大小，也就是不存在，是否创建一个数据块。
 * @return 返回该文件块在分区中的块号
 */
int inode_get_block(inode_t *inode, int fblock, bool create)
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
            dbg_error("read buffer fail\r\n");
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
    dbg_error("not handle 2-level indirect_block\r\n");
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
int read_content_from_izone(inode_t *inode, char *buf, int buf_size, int whence, int read_size)
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
int write_content_to_izone(inode_t *inode, const char *buf, int buf_size, int whence, int write_size)
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

    return bytes_written;
}

/**
 * @brief 截断inode到指定大小
 * @param inode 要截断的inode
 * @param new_size 新的文件大小
 * @return 成功返回0，失败返回-1
 */
int inode_truncate(inode_t* inode, uint32_t new_size) {
    if (!inode || !inode->data) {
        dbg_error("Invalid inode\r\n");
        return -1;
    }

    minix_inode_t* minix_inode = inode->data;
    uint32_t old_size = minix_inode->size;
    int major = inode->major;
    int minor = inode->minor;
    buffer_t* tmp_buf;
    // 如果新大小等于当前大小，直接返回
    if (new_size == old_size) {
        return 0;
    }

    // 如果是扩大的情况
    if (new_size > old_size) {
        // 计算需要增加的块数
        uint32_t old_blocks = (old_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        uint32_t new_blocks = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        
        // 分配新的数据块
        for (uint32_t i = old_blocks; i < new_blocks; i++) {
            int blk = inode_get_block(inode, i, true);
            if (blk <= 0) {
                dbg_error("Failed to allocate block %d\r\n", i);
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
    for (uint32_t i = new_blocks; i < old_blocks; i++) {
        uint32_t fblock = i;
        uint16_t* zone = minix_inode->zone;

        // 处理直接块 (0-6)
        if (fblock < 7) {
            if (zone[fblock] != 0) {
                minix_dblk_free(major, minor, zone[fblock]);
                zone[fblock] = 0;
            }
            continue;
        }

        // 处理一级间接块 (7)
        if (fblock < 7 + BLOCK_SIZE/sizeof(uint16_t)) {
            if (zone[7] == 0) continue;

            uint16_t *indirect;
            buffer_t* indirect_buf = fs_read_to_buffer(major,minor,zone[7],true);
            ASSERT(indirect!=NULL);
            indirect = indirect_buf->data;

            uint32_t idx = fblock - 7;
            if (indirect[idx] != 0) {
                minix_dblk_free(major, minor, indirect[idx]);
                indirect[idx] = 0;
            }
            continue;
        }
        //无需处理二级间接块
        dbg_error("file too large\r\n");
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
inode_t* namei(const char* path) {
    if (path == NULL || *path == '\0') {
        return NULL;
    }

    task_t* cur = cur_task();
    PathParser *parse = path_parser_init(path);
    if (parse == NULL) {
        return NULL;
    }

    char* component = path_parser_next(parse);
    inode_t* parent_inode = NULL;
    inode_t* current_inode = NULL;

    // 确定起始inode
    if (is_rootdir(component)) {
        parent_inode = get_root_inode();
    } else {
        parent_inode = cur->ipwd;
    }

    // 遍历路径组件
    while ((component = path_parser_next(parse)) != NULL) {
        minix_dentry_t dentry;
        int ret = find_entry(parent_inode, component, &dentry);
        if (ret < 0) {
            path_parser_free(parse);
            return NULL; // 路径组件不存在
        }

        // 获取当前组件的inode
        current_inode = (inode_t*)kmalloc(sizeof(inode_t));
        if (current_inode == NULL) {
            path_parser_free(parse);
            return NULL;
        }

        if (get_dev_inode(current_inode, parent_inode->major, 
                         parent_inode->minor, dentry.nr) < 0) {
            kfree(current_inode);
            path_parser_free(parse);
            return NULL;
        }

        // 更新父inode
        if (parent_inode != cur->ipwd && parent_inode != get_root_inode()) {
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
inode_t* named(const char* path)
{
    char* parent = path_dirname(path);
    return namei(parent);
}