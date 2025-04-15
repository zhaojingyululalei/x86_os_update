#include "fs/mount.h"
#include "mem/kmalloc.h"
#include "fs/minix_inode.h"
#include "task/sche.h"
#include "task/task.h"
#include "fs/path.h"
#include "string.h"
#include "fs/minix_entry.h"
#include "fs/minix_super.h"
#include "fs/fs_cfg.h"
#include "printk.h"
#include "fs/stat.h"
#include "fs/fs.h"
static list_t mount_list;

/**
 * @brief 将设备挂载到指定路径
 * @param path 挂载点路径
 * @param major 主设备号
 * @param minor 次设备号
 * @param type 文件系统类型
 * @return 成功返回0，失败返回-1
 */
int sys_mount(const char *path, int major, int minor, fs_type_t type)
{
    if (!path)
    {
        dbg_error("Path is NULL\r\n");
        return -1;
    }

    // 分配挂载点结构体
    mount_point_t *mp = kmalloc(sizeof(mount_point_t));
    if (!mp)
    {
        dbg_error("Memory allocation failed\r\n");
        return -1;
    }

    // 处理路径
    if (!path_is_absolute(path))
    {
        // 如果不是绝对路径，转换为绝对路径
        task_t *ctask = cur_task();
        char *cwd = minix_inode_to_absolute_path(ctask->ipwd);
        if (!cwd)
        {
            dbg_error("Failed to get current working directory\r\n");
            kfree(mp);
            return -1;
        }

        char *abs = path_to_absolute(cwd, path);
        if (!abs)
        {
            dbg_error("Failed to convert path to absolute\r\n");
            kfree(cwd);
            kfree(mp);
            return -1;
        }

        strcpy(mp->point_path, abs);
        kfree(cwd);
        kfree(abs);
    }
    else
    {
        strcpy(mp->point_path, path);
    }

    // 设置挂载点信息
    mp->major = major;
    mp->minor = minor;
    mp->type = type;

    // 将挂载点添加到挂载列表
    list_insert_last(&mount_list, &mp->node);

    // 如果挂载点不是根目录 "/"
    if (strlen(mp->point_path) > 1)
    {
        // 找到挂载点对应的目录项
        minix_inode_desc_t *mount_inode = minix_namei(mp->point_path);
        if (!mount_inode)
        {
            dbg_error("Failed to find mount point inode\r\n");
            list_remove(&mount_list, &mp->node);
            kfree(mp);
            return -1;
        }
        if(!ISDIR(mount_inode->data->mode))
        {
            dbg_error("%s is not a dir,can`t mount device\r\n");
            list_remove(&mount_list, &mp->node);
            kfree(mp);
            return -3;

        }
        if(mount_inode->data->size>2*sizeof(minix_dentry_t))
        {
            dbg_error("the entry not empty,can`t mount device\r\n");
            list_remove(&mount_list, &mp->node);
            kfree(mp);
            return -2;
        }
       

        // 将目录项的 inode 号改为 1（根目录）
        minix_dentry_t dentry;
        if (find_entry(mount_inode->parent, mount_inode->name, &dentry) < 0)
        {
            dbg_error("Failed to find directory entry\r\n");
            list_remove(&mount_list, &mp->node);
            kfree(mp);
            return -1;
        }

        dentry.nr = 1; // 根目录的 inode 号
        if (update_entry(mount_inode->parent, &dentry) < 0)
        {
            dbg_error("Failed to update directory entry\r\n");
            list_remove(&mount_list, &mp->node);
            kfree(mp);
            return -1;
        }
        mp->father_major = mount_inode->major;
        mp->father_minor = mount_inode->minor;
        mp->orign_nr = mount_inode->idx;
        remove_inode_from_tree(mount_inode);
        mount_inode->idx =1;
        mount_inode->major = major;
        mount_inode->minor = minor;
        mount_inode->data = get_dev_inode_data(major,minor,1);
        ASSERT(mount_inode->parent!=NULL);
        insert_inode_to_tree(mount_inode);
    }

    dbg_info("Mounted device %d:%d at %s\r\n", major, minor, mp->point_path);
    return 0;
}
mount_point_t* find_point_by_path(const char* abs_path)
{
    list_node_t* cur = mount_list.first;
    while (cur)
    {
        mount_point_t* point = list_node_parent(cur,mount_point_t,node);
        if(strcmp(abs_path,point->point_path)==0)
        {
            return point;
        }
        cur = cur->next;
    }
    
}
/**
 * @brief 卸载已挂载的设备
 * @param path 挂载点路径
 * @return 成功返回0，失败返回负数
 */
int sys_unmount(const char *path) {
    if (!path) {
        dbg_error("Path is NULL\r\n");
        return -1;
    }

    // 处理路径，确保是绝对路径
    char *abs_path = NULL;
    if (!path_is_absolute(path)) {
        // 如果不是绝对路径，转换为绝对路径
        task_t *ctask = cur_task();
        char *cwd = minix_inode_to_absolute_path(ctask->ipwd);
        if (!cwd) {
            dbg_error("Failed to get current working directory\r\n");
            return -1;
        }

        abs_path = path_to_absolute(cwd, path);
        kfree(cwd);
    } else {
        abs_path = strdup(path);
    }

    if (!abs_path) {
        dbg_error("Failed to convert path to absolute\r\n");
        return -1;
    }

    // 查找挂载点
    mount_point_t *mp = find_point_by_path(abs_path);
    if (!mp) {
        dbg_error("Mount point not found: %s\r\n", abs_path);
        kfree(abs_path);
        return -2;
    }

    // 如果挂载点不是根目录 "/"
    if (strlen(abs_path) > 1) {
        // 找到挂载点对应的目录项
        minix_inode_desc_t *mount_inode = minix_namei(abs_path);
        if (!mount_inode) {
            dbg_error("Failed to find mount point inode\r\n");
            kfree(abs_path);
            return -3;
        }

        // 恢复目录项的 inode 号
        minix_dentry_t dentry;
        if (find_entry(mount_inode->parent, mount_inode->name, &dentry) < 0) {
            dbg_error("Failed to find directory entry\r\n");
            kfree(abs_path);
            return -4;
        }

        dentry.nr = mp->orign_nr; // 恢复原始的 inode 号
        if (update_entry(mount_inode->parent, &dentry) < 0) {
            dbg_error("Failed to update directory entry\r\n");
            kfree(abs_path);
            return -5;
        }

        // 恢复 inode 的原始信息
        mount_inode->idx = mp->orign_nr;
        mount_inode->major = mp->father_major;
        mount_inode->minor = mp->father_minor;
        mount_inode->data = get_dev_inode_data(mp->father_major, mp->father_minor, mp->orign_nr);
        insert_inode_to_tree(mount_inode);
    }

    // 从挂载列表中移除挂载点
    list_remove(&mount_list, &mp->node);
    kfree(mp);
    kfree(abs_path);

    dbg_info("Unmounted device at %s\r\n", path);
    return 0;
}
/**
 * @brief 获取根文件系统节点
 */
minix_inode_desc_t* get_root_inode(void)
{
    return minix_get_dev_root_inode(FS_ROOT_MAJOR,FS_ROOT_MINOR);
}