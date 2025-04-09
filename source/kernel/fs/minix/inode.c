#include "fs/minix_fs.h"
#include "string.h"
#include "printk.h"
#include "fs/buffer.h"
/**
 * @brief 获取某个设备的某个inode
 * @note  inode表的第一个节点，其实是imap位图的第1个节点，而不是第0个，第0个是无效节点，在imap中，第0个节点永远为1
 */

int get_dev_inode(inode_t* inode, int major, int minor, int idx) {
    
    // 参数检查
    if (inode == NULL || idx < 1) {
        dbg_error("param fault\r\n");
        return -1;
    }

    // 获取设备的超级块
    super_block_t sup_blk;
    if (get_dev_super_block(&sup_blk, major, minor) != 0) {
        dbg_error("param fault\r\n");
        return -1;
    }

    // 检查inode号是否有效
    if (idx > sup_blk.data->inodes) {
        dbg_error("param fault\r\n");
        return -1;
    }

    idx -=1;
    // 检查inode是否已分配
    uint16_t byte_offset = idx / 8;
    uint8_t bit_mask = 1 << (idx % 8);
    if (!(sup_blk.inode_map[byte_offset] & bit_mask)) {
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
    uint32_t inode_block = inode_table_start + idx / inodes_per_block;
    
    // 计算inode在块内的偏移
    uint32_t inode_offset = (idx  % inodes_per_block) * inode_size;

    // 读取包含目标inode的块
    buffer_t* inode_buf = fs_read_to_buffer(major,minor,inode_block,true);
    minix_inode_t* inode_data = (minix_inode_t*)(inode_buf->data + inode_offset);
    
    inode->data = inode_data;

    

    // 填充其他信息
    inode->major = major;
    inode->minor = minor;
    inode->idx = idx+1;
    
    // 初始化时间(使用inode中的mtime作为基准)
    time_t now = sys_time(NULL);
    inode->atime = now;
    inode->mtime = inode->data->mtime;
    //创建时间，只要有了就不要动
    if(inode->ctime==0){
        inode->ctime = inode->data->mtime;
    }

    return 0;
}
/**
 * @brief 获取根目录的inode
 */
inode_t* get_root_inode(void){
    super_block_t* sup_blk = get_root_super_block();
    return  sup_blk->iroot;
}

/**
 * @brief 获取文件块在分区中的块号
 * @param fblock:该文件块块号。例如文件大小4KB，总共4块，每块2个扇区，每个扇区512字节。要获取它第二块所在分区的块号
 * @param create:如果这个fblock超过了该文件大小，也就是不存在，是否创建一个数据块。
 * @return 返回该文件块在分区中的块号
 */
int inode_get_block(inode_t* inode, int fblock, bool create) {
    // 参数检查
    if (!inode || !inode->data || fblock < 0) {
        return -1;
    }

    minix_inode_t *minix_inode = inode->data;
    int major = inode->major;
    int minor = inode->minor;

    // 1. 直接块处理 (0-6)
    if (fblock < 7) {
        if (minix_inode->zone[fblock] != 0) {
            return minix_inode->zone[fblock]; // 已有物理块
        }
        
        if (create) {
            int new_blk = minix_dblk_alloc(major, minor);
            if (new_blk > 0) {
                minix_inode->zone[fblock] = new_blk;
                inode->data->mtime = sys_time(NULL); // 更新修改时间
                return new_blk;
            }
        }
        return -1;
    }

    // 2. 一级间接块处理
    int indirect_limit = 7 + BLOCK_SIZE/sizeof(uint16_t);
    if (fblock < indirect_limit) {
        // 检查间接块是否存在
        if (minix_inode->zone[7] == 0) {
            if (!create) return -1;
            
            // 分配间接块
            minix_inode->zone[7] = minix_dblk_alloc(major, minor);
            if (minix_inode->zone[7] == 0) return -1;
            
            
        }

        // 读取间接块
        buffer_t* indirect_buf = fs_read_to_buffer(major,minor,minix_inode->zone[7],true);
        if(!indirect_buf){
            dbg_error("read buffer fail\r\n");
            return -2;
        }
        uint16_t* indirect_block = indirect_buf->data;

        int idx = fblock - 7;
        if (indirect_block[idx] != 0) {
            return indirect_block[idx]; // 已有物理块
        }

        if (create) {
            int new_blk = minix_dblk_alloc(major, minor);
            if (new_blk > 0) {
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