#include "fs/file.h"
#include "string.h"
#include "task/task.h"
#include "task/sche.h"
#include "fs/inode.h"
#include "fs/stat.h"
#include "printk.h"
#include "fs/path.h"
int minix_open(inode_t *dir, char *name, int flags, int mode)
{
}
/**
 * @brief 不允许递归创建目录
 */
int minix_mkdir(char *path, int mode)
{
    // 解析路径，获取父目录的 inode
    inode_t *parent_inode = named(path);
    if (!parent_inode)
    {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取新目录的名称
    char *dir_name = path_basename(path);
    if (!dir_name)
    {
        dbg_error("Invalid directory name\r\n");
        return -1;
    }

    // 检查目录是否已经存在
    minix_dentry_t entry;
    if (find_entry(parent_inode, dir_name, &entry) >= 0)
    {
        dbg_error("Directory already exists\r\n");
        return -1;
    }

    // 分配一个新的 inode
    int ino = minix_inode_alloc(parent_inode->major, parent_inode->minor);
    if (ino < 0)
    {
        dbg_error("Failed to allocate inode\r\n");
        return -1;
    }

    // 在父目录中添加新目录的目录项
    if (add_entry(parent_inode, dir_name, ino) < 0)
    {
        dbg_error("Failed to add directory entry\r\n");
        return -1;
    }

    // 初始化新目录的 inode
    inode_t new_inode;
    if (get_dev_inode(&new_inode, parent_inode->major, parent_inode->minor, ino) < 0)
    {
        dbg_error("Failed to get inode\r\n");
        return -1;
    }
    uint16_t mask = cur_task()->umask;
    mode = mode & ~mask;
    new_inode.data->mode = IFDIR | mode; // 设置目录类型和权限
    new_inode.data->nlinks = 2;          // 目录的链接数为2（. 和 ..）
    new_inode.data->size = 0;            // 初始大小为0

    // 创建 . 和 .. 目录项
    minix_dentry_t dot_entry, dotdot_entry;
    strncpy(dot_entry.name, ".", MINIX1_NAME_LEN);
    dot_entry.nr = ino;
    if (write_content_to_izone(&new_inode, (char *)&dot_entry, sizeof(minix_dentry_t), 0, sizeof(minix_dentry_t)) < 0)
    {
        dbg_error("Failed to create . entry\r\n");
        return -1;
    }

    strncpy(dotdot_entry.name, "..", MINIX1_NAME_LEN);
    dotdot_entry.nr = parent_inode->idx;
    if (write_content_to_izone(&new_inode, (char *)&dotdot_entry, sizeof(minix_dentry_t), sizeof(minix_dentry_t), sizeof(minix_dentry_t)) < 0)
    {
        dbg_error("Failed to create .. entry\r\n");
        return -1;
    }

    // 增加父目录的硬链接计数
    parent_inode->data->nlinks++;

    return 0;
}
/**
 * @brief 不允许递归删除目录
 */
int minix_rmdir(const char *path)
{
    // 解析路径，获取父目录的 inode
    inode_t *parent_inode = named(path);
    if (!parent_inode)
    {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取新目录的名称
    char *dir_name = path_basename(path);
    if (!dir_name)
    {
        dbg_error("Invalid directory name\r\n");
        return -1;
    }

    if (delete_entry_dir(parent_inode, dir_name, false) < 0)
    {
        dbg_error("Failed to delete directory entry\r\n");
        return -1;
    }
    return 0;
}

/**
 * @brief 创建硬链接
 */
int minix_link(const char *old_path, const char *new_path)
{
    if (!old_path || !new_path)
    {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    // 解析 old_path，获取目标文件的 inode
    inode_t *target_inode = namei(old_path);
    if (!target_inode)
    {
        dbg_error("Target file not found\r\n");
        return -1;
    }

    // 检查目标文件是否是目录（不允许对目录创建硬链接）
    if (ISDIR(target_inode->data->mode))
    {
        dbg_error("Cannot create hard link to a directory\r\n");
        return -1;
    }

    // 解析 new_path，获取父目录的 inode
    inode_t *parent_inode = named(new_path);
    if (!parent_inode)
    {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取新链接的名称
    char *link_name = path_basename(new_path);
    if (!link_name)
    {
        dbg_error("Invalid link name\r\n");
        return -1;
    }

    // 检查新路径是否已经存在
    minix_dentry_t entry;
    if (find_entry(parent_inode, link_name, &entry) >= 0)
    {
        dbg_error("Link name already exists\r\n");
        return -1;
    }

    // 在父目录中添加新目录项，指向目标文件的 inode (不用创建新的inode）
    if (add_entry(parent_inode, link_name, target_inode->idx) < 0)
    {
        dbg_error("Failed to add directory entry\r\n");
        return -1;
    }

    // 增加目标文件的硬链接计数
    target_inode->data->nlinks++;

    return 0;
}
/**
 * @brief 删除硬链接
 */
int minix_unlink(const char *path)
{
    if (!path)
    {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    // 解析 path，获取目标文件的 inode
    inode_t *target_inode = namei(path);
    if (!target_inode)
    {
        dbg_error("Target file not found\r\n");
        return -1;
    }

    // 检查目标文件是否是目录（不允许删除目录）
    if (ISDIR(target_inode->data->mode))
    {
        dbg_error("Cannot unlink a directory\r\n");
        return -1;
    }

    // 解析 path，获取父目录的 inode
    inode_t *parent_inode = named(path);
    if (!parent_inode)
    {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取要删除的文件名
    char *file_name = path_basename(path);
    if (!file_name)
    {
        dbg_error("Invalid file name\r\n");
        return -1;
    }

    // 在父目录中删除目标文件的目录项
    if (delete_entry(parent_inode, file_name) < 0)
    {
        dbg_error("Failed to delete directory entry\r\n");
        return -1;
    }

    // 减少目标文件的硬链接计数
    target_inode->data->nlinks--;

    // 如果硬链接计数为 0，并且没有进程打开该文件，则释放 inode 和数据块
    if (target_inode->data->nlinks == 0 && target_inode->ref == 0)
    {
        // 释放 inode 和数据块
        inode_truncate(target_inode, 0);
        minix_inode_free(target_inode->major, target_inode->minor, target_inode->idx);

    }
    return 0;
}