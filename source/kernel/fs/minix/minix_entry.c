#include "fs/fs.h"
#include "fs/minix_inode.h"
#include "fs/minix_fs.h"
#include "fs/stat.h"
#include "printk.h"
#include "mem/kmalloc.h"
#include "string.h"
#include "fs/buffer.h"
#include "fs/minix_entry.h"
/**
 * @brief 提供一个inode，查找为name的entry并返回
 * @param entry:记录找到的entry
 * @param inode:父目录的inode
 * @return 返回该条目对应的inode
 */
int find_entry(minix_inode_desc_t *inode, const char *name, minix_dentry_t *entry)
{
    uint16_t mode = inode->data->mode;
    // 如果不是目录，找不了
    if (!ISDIR(mode))
    {
        dbg_error("current inode not dir\r\n");
        return -1;
    }
    // 该inode包含了多少条目
    int entry_nr = inode->data->size / sizeof(minix_dentry_t);
    minix_dentry_t *start = (minix_dentry_t *)kmalloc(inode->data->size);
    int ret = minix_inode_read(inode, start, inode->data->size, 0, inode->data->size);
    if (ret < 0)
    {
        dbg_error("read fail\r\n");
        kfree(start);
        return -2;
    }
    for (int i = 0; i < entry_nr; i++, start++)
    {
        if (strncmp(start->name, name, strlen(name)) == 0)
        {
            memcpy(entry, start, sizeof(minix_dentry_t));
            kfree(start);
            return i;
        }
    }
    kfree(start);
    return -3;
}
/**
 * @brief 添加一条entry
 * @param inode 父目录inode
 * @return 成功返回0
 */
int add_entry(minix_inode_desc_t *inode, const char *name,int ino)
{
    uint16_t mode = inode->data->mode;
    // 如果不是目录，找不了
    if (!ISDIR(mode))
    {
        dbg_error("current inode not dir\r\n");
        return -1;
    }
    minix_dentry_t *entry = (minix_dentry_t *)kmalloc(sizeof(minix_dentry_t));
    int entry_nr = inode->data->size / sizeof(minix_dentry_t);
    int whence = entry_nr * sizeof(minix_dentry_t);
    strncpy(entry->name, name, strlen(name));
    entry->nr = ino;
    //添加目录项
    int ret = minix_inode_write(inode,entry,sizeof(minix_dentry_t),whence,sizeof(minix_dentry_t));
    if(ret < 0)
    {
        dbg_error("write fail\r\n");
        kfree(entry);
        return -2;
    }
    kfree(entry);
    return 0;

}

/**
 * @brief 删除一条目录项（单纯的删除，啥都不处理）
 */
int delete_entry(minix_inode_desc_t *inode, const char *name)
{
    int ret = find_entry(inode, name, NULL);
    if(ret < 0)
    {
        dbg_error("Entry not found\r\n");
        return -1;
    }
    // 从目录中删除条目
    int entry_size = sizeof(minix_dentry_t);
    int entry_count = inode->data->size / entry_size;
    if (ret == entry_count - 1) {
    // 如果是最后一个条目，直接截断
        inode->data->size -= entry_size;
    } 
    // 否则用最后一个条目覆盖
    else {
        minix_dentry_t last_entry;
        if (minix_inode_read(inode, (char*)&last_entry, entry_size, 
                                  (entry_count-1)*entry_size, entry_size) < 0) {
            return -7;
        }
        
        if (minix_inode_write(inode, (char*)&last_entry, entry_size,
                                 ret*entry_size, entry_size) < 0) {
            return -8;
        }
        
        inode->data->size -= entry_size;
    }
    return 0;
}
/**
 * @brief 更新目录项
 * @param inode 父目录的inode
 * @param dentry 要更新的目录项
 * @return 成功返回0，失败返回负数
 */
int update_entry(minix_inode_desc_t *inode, minix_dentry_t *dentry) {
    // 参数检查
    if (!inode || !dentry) {
        dbg_error("Invalid parameters\r\n");
        return -1;
    }

    // 查找要更新的目录项
    minix_dentry_t old_entry;
    int ret = find_entry(inode, dentry->name, &old_entry);
    if (ret < 0) {
        dbg_error("Entry not found\r\n");
        return -2;
    }

    // 计算目录项的位置
    int entry_size = sizeof(minix_dentry_t);
    int entry_offset = ret * entry_size;

    // 更新目录项
    int write_ret = minix_inode_write(inode, dentry, entry_size, entry_offset, entry_size);
    if (write_ret < 0) {
        dbg_error("Failed to update entry\r\n");
        return -3;
    }

    return 0;
}
/**
 * @brief 释放inode所有的数据块
 * @param inode 要释放的inode指针
 * @return 成功返回0，失败返回-1
 */
static int free_inode_zones(minix_inode_desc_t *inode) {
    if (!inode || !inode->data) {
        return -1;
    }

    minix_inode_t *minix_inode = inode->data;
    int major = inode->major;
    int minor = inode->minor;

    // 遍历所有zone条目
    for (int i = 0; i < 9; i++) {
        if (minix_inode->zone[i] == 0) {
            continue; // 未使用的块
        }

        // 处理直接块 (0-6)
        if (i < 7) {
            minix_dblk_free(major, minor, minix_inode->zone[i]);
            minix_inode->zone[i] = 0;
        } 
        // 处理一级间接块 (7)
        else if (i == 7) {
            
            
            uint16_t* indirect_blocks;
            buffer_t* blk_buf = fs_read_to_buffer(major,minor,minix_inode->zone[i],true);
            indirect_blocks = blk_buf->data;
            // 释放间接块指向的所有块
            for (int j = 0; j < BLOCK_SIZE/sizeof(uint16_t); j++) {
                if (indirect_blocks[j] == 0) break;
                minix_dblk_free(major, minor, indirect_blocks[j]);
            }

            // 释放间接块本身
            minix_dblk_free(major, minor, minix_inode->zone[i]);
            minix_inode->zone[i] = 0;
        }
        // 处理二级间接块 (8) - 可选实现
        else if (i == 8) {
            dbg_error("file too large\r\n");
            return -3;
        }
    }

    // 重置文件大小
    minix_inode->size = 0;
    return 0;
}
/**
 * @brief 删除非目录文件
 * @param inode 父目录inode
 * @param name 要删除的文件名
 * @return 成功返回0，失败返回-1
 */
int delete_entry_not_dir(minix_inode_desc_t* inode, const char* name) {
    // 参数检查
    if (!inode || !name) {
        return -1;
    }

    // 查找目录条目
    minix_dentry_t entry;
    int ret = find_entry(inode, name, &entry);
    if (ret < 0) {
        dbg_error("Entry not found\n");
        return -2;
    }

    // 获取目标inode
    minix_inode_desc_t* sub_inode = minix_inode_find(inode->major, inode->minor, entry.nr);
    if(!sub_inode)
    {
        dbg_error("sub_inode not found\r\n");
        return -3;
    }

    // 检查文件类型
    if (ISDIR(sub_inode->data->mode)) {
        dbg_error("Cannot delete directory with this function\n");
        return -4;
    }

    // 处理硬链接
    if (sub_inode->data->nlinks > 1) {
        sub_inode->data->nlinks--;
        goto remove_entry; // 只需要减少链接计数
    }

    // 释放所有数据块
    if (free_inode_zones(sub_inode) < 0) {
        dbg_error("Failed to free zones\n");
        return -5;
    }

    // 释放inode
    if (minix_inode_free(sub_inode->major, sub_inode->minor, sub_inode->idx) < 0) {
        dbg_error("Failed to free inode\n");
        return -6;
    }

    //从树中移除，并释放inode
    minix_inode_delete(sub_inode);
remove_entry:
    // 从目录中删除条目
    int entry_size = sizeof(minix_dentry_t);
    int entry_count = inode->data->size / entry_size;
    
    // 如果是最后一个条目，直接截断
    if (ret == entry_count - 1) {
        inode->data->size -= entry_size;
    } 
    // 否则用最后一个条目覆盖
    else {
        minix_dentry_t last_entry;
        if (minix_inode_read(inode, (char*)&last_entry, entry_size, 
                                  (entry_count-1)*entry_size, entry_size) < 0) {
            return -7;
        }
        
        if (minix_inode_write(inode, (char*)&last_entry, entry_size,
                                 ret*entry_size, entry_size) < 0) {
            return -8;
        }
        
        inode->data->size -= entry_size;
    }

    return 0;
}
/**
 * @brief 删除目录项
 * @param recursion 是否递归删除子目录
 * @param inode 父目录inode
 */
int delete_entry_dir(minix_inode_desc_t* inode, const char* name, bool recursion) {
    // 参数检查
    if (!inode || !name) {
        return -1;
    }

    // 查找目录条目
    minix_dentry_t entry;
    int ret = find_entry(inode, name, &entry);
    if (ret < 0) {
        dbg_error("Entry not found\n");
        return -2;
    }

    // 获取目标inode
    minix_inode_desc_t* sub_inode = minix_inode_find(inode->major, inode->minor, entry.nr);

    if(!sub_inode)
    {
        dbg_error("sub_inode not found\r\n");
        return -3;
    }

    // 检查文件类型
    if (!ISDIR(sub_inode->data->mode)) {
        dbg_error("Not a directory\n");
        return -4;
    }

    // 检查目录是否为空
    int entry_count = sub_inode->data->size / sizeof(minix_dentry_t);
    if (entry_count > 2 && !recursion) {
        dbg_error("Directory not empty\n");
        return -5;
    }

    // 递归删除子目录和文件
    if (recursion && entry_count > 2) {
        minix_dentry_t *entries = kmalloc(sub_inode->data->size);
        if (!entries) {
            return -6;
        }

        if (minix_inode_read(sub_inode, (char*)entries, 
                                  sub_inode->data->size, 0, sub_inode->data->size) < 0) {
            kfree(entries);
            return -7;
        }

        for (int i = 0; i < entry_count; i++) {
            // 跳过.和..
            if (strcmp(entries[i].name, ".") == 0 || 
                strcmp(entries[i].name, "..") == 0) {
                continue;
            }

            minix_inode_desc_t* child_inode = minix_inode_find(sub_inode->major, sub_inode->minor,entries[i].nr);
            if(!child_inode)
            {
                continue;
            }

            if (ISDIR(child_inode->data->mode)) {
                delete_entry_dir(sub_inode, entries[i].name, true);
            } else {
                delete_entry_not_dir(sub_inode, entries[i].name);
            }
        }
        
        kfree(entries);
    }

    // 减少父目录的硬链接计数
    inode->data->nlinks--;

    // 删除目录本身
    // 清空目录内容
    sub_inode->data->size = 0;
    
    // 释放所有数据块
    if (free_inode_zones(sub_inode) < 0) {
        dbg_error("Failed to free zones\n");
        return -8;
    }

    // 释放inode
    if (minix_inode_free(sub_inode->major, sub_inode->minor, sub_inode->idx) < 0) {
        dbg_error("Failed to free inode\n");
        return -9;
    }

    //从树中移除inode并释放inode
    minix_inode_delete(sub_inode);
    // 从父目录中删除条目
    int entry_size = sizeof(minix_dentry_t);
    int parent_count = inode->data->size / entry_size;
    
    if (ret == parent_count - 1) {
        inode->data->size -= entry_size;
    } else {
        minix_dentry_t last_entry;
        if (minix_inode_read(inode, (char*)&last_entry, entry_size,
                                  (parent_count-1)*entry_size, entry_size) < 0) {
            return -10;
        }
        
        if (minix_inode_write(inode, (char*)&last_entry, entry_size,
                                 ret*entry_size, entry_size) < 0) {
            return -11;
        }
        
        inode->data->size -= entry_size;
    }

    return 0;
}
/**
 * @brief 打印目录中的所有条目
 * @param inode 目录inode指针
 */
void print_entrys(minix_inode_desc_t* inode) {
    // 参数检查
    if (!inode || !inode->data) {
        dbg_info("Invalid inode\r\n");
        return;
    }

    // 检查是否是目录
    if (!ISDIR(inode->data->mode)) {
        dbg_info("Not a directory\r\n");
        return;
    }

    // 计算目录条目数量
    int entry_count = inode->data->size / sizeof(minix_dentry_t);
    if (entry_count <= 0) {
        dbg_info("Empty directory\r\n");
        return;
    }

    // 读取所有目录条目
    minix_dentry_t* entries = kmalloc(inode->data->size);
    if (!entries) {
        dbg_info("Memory allocation failed\r\n");
        return;
    }

    if (minix_inode_read(inode, (char*)entries, inode->data->size, 
                              0, inode->data->size) < 0) {
        dbg_info("Failed to read directory entries\r\n");
        kfree(entries);
        return;
    }

    // 打印表头
    dbg_info("Total %d entries:\r\n", entry_count);
    dbg_info("Inode\tName\r\n");
    dbg_info("----------------\r\n");

    // 打印每个条目
    for (int i = 0; i < entry_count; i++) {
        // 获取条目inode信息
        minix_inode_desc_t* entry_inode = minix_inode_find(inode->major, inode->minor, 
                                                             entries[i].nr);
        if(!entry_inode)
        {
            continue;
        }

        // 打印条目信息
        char type = ISDIR(entry_inode->data->mode) ? 'D' : 'F';
        dbg_info("[%c] %d\t%s\r\n", 
                type,
                entries[i].nr,
                entries[i].name);
    }

    kfree(entries);
}