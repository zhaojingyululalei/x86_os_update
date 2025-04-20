#include "fs/fs.h"
#include "string.h"
#include "task/task.h"
#include "task/sche.h"
#include "fs/stat.h"
#include "printk.h"
#include "fs/path.h"
#include "fs/minix_inode.h"
#include "fs/minix_entry.h"
#include "fs/fs_cfg.h"
#include "fs/buffer.h"
int minix_open(file_t *file, const char *path, int flags, int mode)
{
    minix_inode_desc_t *inode = minix_inode_open(path, flags, mode);
    file->metadata = inode;
    file->ref = inode->ref;
    return 0;
}
 //读的时候要检查大小，但是写的时候不用
int minix_read(file_t *file, char *buf, int size)
{
    int whence = file->pos;
    minix_inode_desc_t *inode = (minix_inode_desc_t *)file->metadata;
    if (whence + size > inode->data->size)
    {
        return -1;
    }
    int bytes = minix_inode_read(inode, buf, size, whence, size);

    return bytes;
}
//
int minix_write(file_t *file, char *buf, int size)
{
    int whence = file->pos;
    minix_inode_desc_t *inode = (minix_inode_desc_t *)file->metadata;
    
    int bytes = minix_inode_write(inode, buf, size, whence, size);
    file->pos += bytes;
    return bytes;
}
int minix_close(file_t *file)
{
    return 0;
}
int minix_lseek(file_t *file, int offset, int whence)
{
    minix_inode_desc_t *inode = (minix_inode_desc_t *)file->metadata;
    switch (whence)
    {
    case SEEK_SET:
        file->pos = offset;
        break;
    case SEEK_CUR:
        file->pos += offset;
        break;
    case SEEK_END:
        file->pos = inode->data->size;
        file->pos += offset;
    default:
        break;
    }
}
int minix_stat(file_t *file, file_stat_t *st)
{
    minix_inode_desc_t *inode = (minix_inode_desc_t *)file->metadata;
    st->major = inode->major;
    st->minor = inode->minor;
    st->nr = inode->idx;
    st->mode = inode->data->mode;
    st->nlinks = inode->data->nlinks;
    st->uid = inode->data->uid;
    st->gid = inode->data->gid;
    st->size = inode->data->size;
    st->atime = inode->atime;
    st->mtime = inode->data->mtime;
    st->ctime = inode->ctime;
    return 0;
}
int minix_opendir(const char *path, DIR *dir) {
    if (!path || !dir) {
        dbg_error("Invalid path or dir pointer\r\n");
        return -1;
    }

    // 获取目录对应的 inode
    minix_inode_desc_t *inode = minix_namei(path);
    if (!inode) {
        dbg_error("Directory not found: %s\r\n", path);
        return -1;
    }

    // 检查是否是目录
    if (!ISDIR(inode->data->mode)) {
        dbg_error("Not a directory: %s\r\n", path);
        return -1;
    }

    // 初始化目录流
    dir->fd = inode->idx; // 使用 inode 号作为文件描述符
    dir->offset = 0;      // 初始偏移量为0
    dir->entry = NULL;    // 当前目录条目为空

    return 0;
}
int minix_readdir(DIR *dir, struct dirent *dirent) {
    if (!dir || !dirent) {
        dbg_error("Invalid dir or dirent pointer\r\n");
        return -1;
    }

    // 获取目录 inode
    minix_inode_desc_t *inode = minix_inode_find(FS_ROOT_MAJOR, FS_ROOT_MINOR, dir->fd);
    if (!inode) {
        dbg_error("Directory inode not found\r\n");
        return -1;
    }

    // 检查是否是目录
    if (!ISDIR(inode->data->mode)) {
        dbg_error("Not a directory\r\n");
        return -1;
    }

    // 计算目录条目数量
    int entry_count = inode->data->size / sizeof(minix_dentry_t);
    if (dir->offset >= entry_count) {
        return -1; // 已读取完所有条目
    }

    // 读取目录条目
    minix_dentry_t *entries = kmalloc(inode->data->size);
    if (!entries) {
        dbg_error("Memory allocation failed\r\n");
        return -1;
    }

    if (minix_inode_read(inode, (char *)entries, inode->data->size, 0, inode->data->size) < 0) {
        dbg_error("Failed to read directory entries\r\n");
        kfree(entries);
        return -1;
    }

    // 填充 dirent 结构
    minix_dentry_t *entry = &entries[dir->offset];
    dirent->d_ino = entry->nr;
    dirent->d_size = sizeof(minix_dentry_t);
    dirent->d_type = ISDIR(inode->data->mode) ? DT_DIR : DT_REG;
    strncpy(dirent->d_name, entry->name, MINIX1_NAME_LEN);

    // 更新偏移量
    dir->offset++;
    kfree(entries);

    return 0;
}
int minix_closedir(DIR *dir) {
    if (!dir) {
        dbg_error("Invalid dir pointer\r\n");
        return -1;
    }

    // 清空目录流
    dir->fd = -1;
    dir->offset = 0;
    dir->entry = NULL;

    return 0;
}

/**
 * @brief 不允许递归创建目录
 */
int minix_mkdir(minix_inode_desc_t* parent_inode,const char* dir_name,uint16_t mode)
{
    

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
    minix_inode_desc_t *new_inode = minix_inode_find(parent_inode->major, parent_inode->minor, ino);
    if (!new_inode)
    {
        dbg_error("Failed to get inode\r\n");
        return -1;
    }

    uint16_t mask = cur_task()->umask;
    mode = mode & ~mask;
    new_inode->data->mode = IFDIR | mode; // 设置目录类型和权限
    new_inode->data->nlinks = 2;          // 目录的链接数为2（. 和 ..）
    new_inode->data->size = 0;            // 初始大小为0

    // 创建 . 和 .. 目录项
    minix_dentry_t dot_entry, dotdot_entry;
    strncpy(dot_entry.name, ".", MINIX1_NAME_LEN);
    dot_entry.nr = ino;
    if (minix_inode_write(new_inode, (char *)&dot_entry, sizeof(minix_dentry_t), 0, sizeof(minix_dentry_t)) < 0)
    {
        dbg_error("Failed to create . entry\r\n");
        return -1;
    }

    strncpy(dotdot_entry.name, "..", MINIX1_NAME_LEN);
    dotdot_entry.nr = parent_inode->idx;
    if (minix_inode_write(new_inode, (char *)&dotdot_entry, sizeof(minix_dentry_t), sizeof(minix_dentry_t), sizeof(minix_dentry_t)) < 0)
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
    minix_inode_desc_t *parent_inode = minix_named(path);
    // minix_inode_desc_t* inode = minix_namei(path);

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
    minix_inode_desc_t *target_inode = minix_namei(old_path);
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
    minix_inode_desc_t *parent_inode = minix_named(new_path);
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
    minix_inode_desc_t *target_inode = minix_namei(path);
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
    minix_inode_desc_t *parent_inode = minix_named(path);
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
        minix_inode_truncate(target_inode, 0);
        minix_inode_free(target_inode->major, target_inode->minor, target_inode->idx);
        // 释放inode
        minix_inode_delete(target_inode);
    }
    return 0;
}
void minix_fsync(file_t* file)
{
    // 获取 inode
    minix_inode_desc_t *inode = (minix_inode_desc_t *)file->metadata;
    if (!inode || !inode->data) {
        dbg_error("Invalid inode\r\n");
        return -1;
    }

    // 遍历直接块 (0-6)
    for (int i = 0; i < 7; i++) {
        if (inode->data->zone[i] != 0) {
            // 获取数据块对应的 buffer
            buffer_t *buf = fs_read_to_buffer(inode->major, inode->minor, inode->data->zone[i], false);
            if (buf && buf->dirty) {
                // 将脏数据块写回磁盘
                fs_sync_buffer(buf);
            }
        }
    }

    // 遍历一级间接块 (7)
    if (inode->data->zone[7] != 0) {
        // 读取一级间接块
        buffer_t *indirect_buf = fs_read_to_buffer(inode->major, inode->minor, inode->data->zone[7], false);
        if (indirect_buf) {
            uint16_t *indirect_block = (uint16_t *)indirect_buf->data;
            int indirect_limit = BLOCK_SIZE / sizeof(uint16_t);

            // 遍历一级间接块中的所有数据块
            for (int i = 0; i < indirect_limit; i++) {
                if (indirect_block[i] != 0) {
                    // 获取数据块对应的 buffer
                    buffer_t *buf = fs_read_to_buffer(inode->major, inode->minor, indirect_block[i], false);
                    if (buf && buf->dirty) {
                        // 将脏数据块写回磁盘
                        fs_sync_buffer(buf);
                    }
                }
            }
        }
    }

    

    // 更新 inode 的元数据
    inode->data->mtime = sys_time(NULL);
    buffer_t *inode_buf = fs_read_to_buffer(inode->major, inode->minor, inode->idx, true);
    if (inode_buf && inode_buf->dirty) {
        fs_sync_buffer(inode_buf);
    }
    return;
}
fs_op_t minix_opt = {
    .open = minix_open,
    .read = minix_read,
    .write = minix_write,
    .close = minix_close,
    .lseek = minix_lseek,
    .stat = minix_stat,
    .ioctl = NULL,
    .opendir = minix_opendir,
    .readdir = minix_readdir,
    .closedir = minix_closedir,
    .link  = minix_link,
    .unlink = minix_unlink
};
void minix_fs_init()
{
    fs_register(FS_MINIX,&minix_opt);
    minix_inode_tree_init();
}
