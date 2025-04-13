#include "fs/file.h"
#include "string.h"
#include "task/task.h"
#include "task/sche.h"
#include "fs/inode.h"
#include "fs/stat.h"
#include "printk.h"
#include "fs/path.h"
int minix_open (inode_t *dir, char *name, int flags, int mode)
{
    


}

int minix_mkdir (char *path, int mode)
{
    // 解析路径，获取父目录的 inode
    inode_t* parent_inode = named(path);
    if (!parent_inode) {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取新目录的名称
    char* dirname = path_basename(path);
    char dir_name[MINIX1_NAME_LEN];
    strcpy(dir_name,dirname);
    if (!dir_name) {
        dbg_error("Invalid directory name\r\n");
        return -1;
    }

    // 检查目录是否已经存在
    minix_dentry_t entry;
    if (find_entry(parent_inode, dir_name, &entry) >= 0) {
        dbg_error("Directory already exists\r\n");
        return -1;
    }

    // 分配一个新的 inode
    int ino ;
    
    // 在父目录中添加新目录的目录项
    if ((ino = add_entry(parent_inode, dir_name)) < 0) {
        dbg_error("Failed to add directory entry\r\n");
        return -1;
    }
    if (ino < 0) {
        dbg_error("Failed to allocate inode\r\n");
        return -1;
    }
    

    // 初始化新目录的 inode
    inode_t new_inode;
    if (get_dev_inode(&new_inode, parent_inode->major, parent_inode->minor, ino) < 0) {
        dbg_error("Failed to get inode\r\n");
        return -1;
    }
    uint16_t mask = cur_task()->umask;
    mode = mode  & ~ mask;
    new_inode.data->mode = IFDIR | mode; // 设置目录类型和权限
    new_inode.data->nlinks = 2; // 目录的链接数为2（. 和 ..）
    new_inode.data->size = 0; // 初始大小为0

    
    

    // 创建 . 和 .. 目录项
    minix_dentry_t dot_entry, dotdot_entry;
    strncpy(dot_entry.name, ".", MINIX1_NAME_LEN);
    dot_entry.nr = ino;
    if (write_content_to_izone(&new_inode, (char*)&dot_entry, sizeof(minix_dentry_t), 0, sizeof(minix_dentry_t)) < 0) {
        dbg_error("Failed to create . entry\r\n");
        return -1;
    }

    strncpy(dotdot_entry.name, "..", MINIX1_NAME_LEN);
    dotdot_entry.nr = parent_inode->idx;
    if (write_content_to_izone(&new_inode, (char*)&dotdot_entry, sizeof(minix_dentry_t), sizeof(minix_dentry_t), sizeof(minix_dentry_t)) < 0) {
        dbg_error("Failed to create .. entry\r\n");
        return -1;
    }

    // 增加父目录的硬链接计数
    parent_inode->data->nlinks++;

    return 0;


}
int minix_rmdir(const char* path)
{
    // 解析路径，获取父目录的 inode
    inode_t* parent_inode = named(path);
    if (!parent_inode) {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取新目录的名称
    char* dir_name = path_basename(path);
    if (!dir_name) {
        dbg_error("Invalid directory name\r\n");
        return -1;
    }

    if (delete_entry_dir(parent_inode, dir_name, false) < 0) {
        dbg_error("Failed to delete directory entry\r\n");
        return -1;
    }
    return 0;
}