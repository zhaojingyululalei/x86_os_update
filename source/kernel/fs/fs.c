
#include "types.h"
#include "fs/fs.h"
#include "stdlib.h"
#include "cpu_instr.h"
#include "fs/inode.h"
#include "fs/file.h"
#include "fs/minix/minix_fs.h"
#include "mem/kmalloc.h"
#include "string.h"
#include "printk.h"
#include "fs/path.h"
#include "task/sche.h"
#include "dev/dev.h"
#define DISK_SECTOR_SIZE 512
uint8_t tmp_app_buffer[512 * 1024];
uint8_t *tmp_pos;
#define SHELL_FD 1024
/**
 * 使用LBA48位模式读取磁盘
 */
static void read_disk(int sector, int sector_count, uint8_t *buf)
{
    outb(0x1F6, (uint8_t)(0xE0));

    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24)); // LBA参数的24~31位
    outb(0x1F4, (uint8_t)(0));            // LBA参数的32~39位
    outb(0x1F5, (uint8_t)(0));            // LBA参数的40~47位

    outb(0x1F2, (uint8_t)(sector_count));
    outb(0x1F3, (uint8_t)(sector));       // LBA参数的0~7位
    outb(0x1F4, (uint8_t)(sector >> 8));  // LBA参数的8~15位
    outb(0x1F5, (uint8_t)(sector >> 16)); // LBA参数的16~23位

    outb(0x1F7, (uint8_t)0x24);

    // 读取数据
    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count-- > 0)
    {
        // 每次扇区读之前都要检查，等待数据就绪
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
        }

        // 读取并将数据写入到缓存中
        for (int i = 0; i < DISK_SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
        }
    }
}
/**
 * @brief 判断文件描述符是否正确
 */
static int is_fd_bad(int file)
{

    return 0;
}
static bool permission_check(void)
{
    return true;
}
/**
 * @brief 获取 inode 的绝对路径
 * @param inode 目标 inode
 * @param buf 用于存储绝对路径的缓冲区
 * @param buf_size 缓冲区大小
 * @return 成功返回 0，失败返回 -1
 */
static int get_inode_abspath(inode_t *inode, char *buf, int buf_size)
{
    if (!inode || !buf || buf_size <= 0)
    {
        dbg_error("Invalid parameters\r\n");
        return -1;
    }

    // 临时缓冲区，用于存储路径组件
    char path_components[128][128]; // 假设最多支持 128 层路径
    int component_count = 0;

    // 从当前 inode 开始，向上遍历父节点
    inode_t *current = inode;
    while (current)
    {
        // 将当前 inode 的名称存储到路径组件中
        if (component_count >= 128)
        {
            dbg_error("Path depth exceeds maximum limit\r\n");
            return -1;
        }
        strncpy(path_components[component_count], current->name, sizeof(path_components[0]) - 1);
        path_components[component_count][sizeof(path_components[0]) - 1] = '\0';
        component_count++;

        // 移动到父节点
        current = current->i_parent;
    }

    // 从根节点开始拼接路径
    int pos = 0;
    for (int i = component_count - 1; i >= 0; i--)
    {
        // 添加路径分隔符
        if (pos > 0 && pos + 1 < buf_size)
        {
            buf[pos++] = '/';
        }

        // 添加路径组件
        int len = strlen(path_components[i]);
        if (pos + len >= buf_size)
        {
            dbg_error("Buffer overflow\r\n");
            return -1;
        }
        strncpy(buf + pos, path_components[i], len);
        pos += len;
    }

    // 确保以空字符结尾
    if (pos >= buf_size)
    {
        dbg_error("Buffer overflow\r\n");
        return -1;
    }
    buf[pos] = '\0';

    return 0;
}
/**
 * @brief 获取当前路径
 */
int sys_getcwd(char *buf, int buf_size)
{
    int ret;
    task_t *cur = cur_task();
    inode_t *icwd = cur->ipwd;
    ret = get_inode_abspath(icwd, buf, buf_size);
    return ret;
}
/**
 * @brief 根据路径获取inode结构
 */
inode_t *sys_namei(const char *path)
{
    int ret;
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
    inode_t *parent_inode = NULL;
    inode_t *current_inode = NULL;

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
    inode_opts_t *ops = parent_inode->ops;
    dev_t dev = parent_inode->dev;
    // 遍历路径组件
    while ((component = path_parser_next(parse)) != NULL)
    {
        struct dirent dentry;
        ret = ops->find_entry(parent_inode, component, &dentry);
        if (ret < 0)
        {
            path_parser_free(parse);
            return NULL; // 路径组件不存在
        }
        inode_t *subnode = ops->get(dev, dentry.nr);
        // 如果当前inode已经被其他设备挂载了
        if (subnode->mount)
        {
            // 去mountlist中找挂载dian
            mount_point_t *point = find_point_by_inode(subnode);
            int rootnr = inode_root_nr(point->type);
            // 获取挂载设备的根inode节点
            current_inode = ops->get(dev, rootnr);
        }
        else
        {
            // 直接去找某个设备的某个ino就可以
            current_inode = ops->get(dev, dentry.nr);
        }

        if (current_inode == NULL)
        {
            path_parser_free(parse);
            return NULL;
        }

        current_inode->i_parent = parent_inode;
        strcpy(current_inode->name, component);

        parent_inode = current_inode;
    }

    path_parser_free(parse);
    return current_inode ? current_inode : parent_inode;
}

int sys_open(const char *path, int flags, mode_t mode)
{
    int ret;

    ret = strncmp(path, "/shell", 6);
    // 路径解析正确
    if (ret == 0)
    {
        read_disk(10240, 512 * 1024 / 512, tmp_app_buffer);
        tmp_pos = tmp_app_buffer;
        return SHELL_FD;
    }

    if (!permission_check())
    {
        return -1;
    }

    // 分配文件描述符链接
    file_t *file = file_alloc();
    if (!file)
    {
        goto file_alloc_fail;
    }
    file_inc_ref(file);
    int fd = task_alloc_fd(file);
    if (fd < 0)
    {
        goto file_alloc_fail;
    }
    const char *dir = path_dirname(path);
    const char *base = path_basename(path);

    inode_t *idir = sys_namei(dir);
    if (!idir)
    {
        dbg_error("parent dir not exist\r\n");
        goto file_alloc_fail;
    }
    // 有就打开，没有根据flag和mode创建，如果是目录返回null
    inode_t *ibase = inode_open(idir, base, flags, mode);
    if (!ibase)
    {
        goto namei_open_fail;
    }
    if (strcmp(ibase->name, base) != 0)
    {
        goto namei_open_fail;
    }

    file->inode = ibase;
    if (flags & O_APPEND)
    {
        file->pos = inode_acquire_size(file->inode);
    }

    kfree(dir);
    kfree(base);
    return fd;
namei_open_fail:
    kfree(dir);
    kfree(base);
file_alloc_fail:
    file_free(file);
    return -1;
}

int sys_read(int fd, char *buf, int len)
{
    if (fd == SHELL_FD)
    {
        memcpy(buf, tmp_pos, len);
        tmp_pos += len;
        return len;
    }
    int ret;
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }

    inode_t *inode = file->inode;
    mode_t mode = inode_acquire_mode(inode);
    if (ISREG(mode))
    {
        ret = inode_read(inode, buf, len, file->pos, len);
        if (ret < 0)
        {
            dbg_error("write fail\r\n");
            return -1;
        }
    }
    else if (ISCHR(mode) || ISBLK(mode))
    {
        // 先把设备号读出来，然后再去读取设备
        dev_t dev;

        ret = inode_read(inode, &dev, sizeof(dev_t), 0, sizeof(dev_t));
        if (ret < 0)
        {
            dbg_error("devfs failz\r\n");
            return -1;
        }
        ret = dev_read(dev, buf, len, &file->pos);
        if (ret < 0)
        {
            dbg_error("dev read failz\r\n");
            return -2;
        }
    }
    else
    {
        dbg_warning("unhandling\r\n");
    }

    return ret;
}

int sys_write(int fd, const char *buf, size_t len)
{
    if (fd == stdout)
    {
        serial_print(buf);
        return;
    }
    int ret;
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }

    inode_t *inode = file->inode;
    mode_t mode = inode_acquire_mode(inode);
    if (ISREG(mode))
    {
        ret = inode_write(inode, buf, len, file->pos, len);
        if (ret < 0)
        {
            dbg_error("write fail\r\n");
            return -1;
        }
        file->pos += ret;
    }
    else if (ISCHR(mode) || ISBLK(mode))
    {
        // 先把设备号读出来，然后再去读取设备
        dev_t dev;

        ret = inode_read(inode, &dev, sizeof(dev_t), 0, sizeof(dev_t));
        if (ret < 0)
        {
            dbg_error("devfs failz\r\n");
            return -1;
        }
        ret = dev_write(dev, buf, len, &file->pos);
        if (ret < 0)
        {
            dbg_error("dev write failz\r\n");
            return -2;
        }
    }
    else
    {
        dbg_warning("unhandling\r\n");
    }

    return ret;
}
int sys_close(int fd)
{
    int ret;
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }

    inode_t *inode = file->inode;
    mode_t mode = inode_acquire_mode(inode);
    if (ISREG(mode))
    {
        inode_close(inode);
        file_free(file);
        if (file->ref == 0)
        {
            task_remove_fd(fd);
        }
    }
    else if (ISCHR(mode) || ISBLK(mode))
    {
        // 先把设备号读出来，然后再去读取设备
        dev_t dev;

        ret = inode_read(inode, &dev, sizeof(dev_t), 0, sizeof(dev_t));
        if (ret < 0)
        {
            dbg_error("devfs failz\r\n");
            return -1;
        }
        dev_close(dev);
    }
    else
    {
        dbg_warning("unhandling\r\n");
    }

    return ret;
}

int sys_lseek(int fd, int offset, int whence)
{
    if (fd == SHELL_FD)
    {
        tmp_pos = tmp_app_buffer + offset;
        return;
    }

    int ret;
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }

    inode_t *inode = file->inode;
    mode_t mode = inode_acquire_mode(inode);
    if (ISREG(mode))
    {
        switch (whence)
        {
        case SEEK_SET:
            file->pos = offset;
            break;
        case SEEK_CUR:
            file->pos += offset;
            break;
        case SEEK_END:
            file->pos = inode_acquire_size(inode);
            if(offset > 0){
                dbg_error("whence + offset out of boundry\r\n");
                return -1;
            }
            file->pos += offset;
        default:
            break;
        }
    }
    else if (ISCHR(mode) || ISBLK(mode))
    {
        // 先把设备号读出来，然后再去读取设备
        dev_t dev;

        ret = inode_read(inode, &dev, sizeof(dev_t), 0, sizeof(dev_t));
        if (ret < 0)
        {
            dbg_error("devfs failz\r\n");
            return -1;
        }
        dev_lseek(dev,&file->pos,offset,whence);
    }
    else
    {
        dbg_warning("unhandling\r\n");
    }

    return ret;
}

int sys_stat(int fd,file_stat_t* stat)
{
    int ret;
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }

    inode_t *inode = file->inode;
    mode_t mode = inode_acquire_mode(inode);
    if (ISREG(mode))
    {
        inode_stat(inode,stat);
    }
    else if (ISCHR(mode) || ISBLK(mode))
    {
        // 先把设备号读出来，然后再去读取设备
        dev_t dev;

        ret = inode_read(inode, &dev, sizeof(dev_t), 0, sizeof(dev_t));
        if (ret < 0)
        {
            dbg_error("devfs failz\r\n");
            return -1;
        }
        dbg_warning("unhandling\r\n");
    }
    else
    {
        dbg_warning("unhandling\r\n");
    }

    return ret;
}
int sys_ioctl(int fd, int cmd, int arg0, int arg1)
{
    int ret;
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }

    inode_t *inode = file->inode;
    mode_t mode = inode_acquire_mode(inode);
    if (ISREG(mode))
    {
        return;
    }
    else if (ISCHR(mode) || ISBLK(mode))
    {
        // 先把设备号读出来，然后再去读取设备
        dev_t dev;

        ret = inode_read(inode, &dev, sizeof(dev_t), 0, sizeof(dev_t));
        if (ret < 0)
        {
            dbg_error("devfs failz\r\n");
            return -1;
        }
        dev_control(dev,cmd,arg0,arg1);
    }
    else
    {
        dbg_warning("unhandling\r\n");
    }

    return ret;
}

int sys_fsync(int fd)
{
    int ret;
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }

    inode_t *inode = file->inode;
    mode_t mode = inode_acquire_mode(inode);
    if (ISREG(mode))
    {
        inode_fsync(inode);
    }
    else if (ISCHR(mode) || ISBLK(mode))
    {
        return ret;
    }
    else
    {
        dbg_warning("unhandling\r\n");
    }

    return ret;
}


int sys_link(const char *old_path, const char *new_path)
{
    if (!old_path || !new_path)
    {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    // 解析 old_path，获取目标文件的 inode
    inode_t *target_inode = sys_namei(old_path);
    if (!target_inode)
    {
        dbg_error("Target file not found\r\n");
        return -1;
    }

    // 检查目标文件是否是目录（不允许对目录创建硬链接）
    if (ISDIR(inode_acquire_mode(target_inode)))
    {
        dbg_error("Cannot create hard link to a directory\r\n");
        return -1;
    }

    // 解析 new_path，获取父目录的 inode
    const char* parent_path = path_dirname(new_path);
    inode_t *parent_inode = sys_namei(parent_path);
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
    struct dirent entry;
    
    if (parent_inode->ops->find_entry(parent_inode, link_name, &entry) >= 0)
    {
        dbg_error("Link name already exists\r\n");
        return -1;
    }

    // 在父目录中添加新目录项，指向目标文件的 inode (不用创建新的inode）
    struct dirent new_entry;
    new_entry.nr = target_inode->nr;
    strcpy(new_entry.name,link_name);
    if (parent_inode->ops->add_entry(parent_inode,&new_entry ) < 0)
    {
        dbg_error("Failed to add directory entry\r\n");
        return -1;
    }

    // 增加目标文件的硬链接计数
    inode_modify_nlinks(target_inode,inode_acquire_nlinks(target_inode)+1);
    

    return 0;
}
int sys_unlink(const char *path)
{
    if (!path)
    {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    // 解析 path，获取目标文件的 inode
    inode_t *target_inode = sys_namei(path);
    if (!target_inode)
    {
        dbg_error("Target file not found\r\n");
        return -1;
    }

    // 检查目标文件是否是目录（不允许删除目录）
    if (ISDIR(inode_acquire_mode(target_inode)))
    {
        dbg_error("Cannot unlink a directory\r\n");
        return -1;
    }

    // 解析 path，获取父目录的 inode
    const char* parent_path = path_dirname(path);
    inode_t *parent_inode = sys_namei(parent_path);
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
    struct dirent parent_entry;
    strcpy(parent_entry.name,file_name);
    
    if (parent_inode->ops->delete_entry(parent_inode,&parent_entry) < 0)
    {
        dbg_error("Failed to delete directory entry\r\n");
        return -1;
    }

    // 减少目标文件的硬链接计数
    inode_modify_nlinks(target_inode,inode_acquire_nlinks(target_inode)-1);

    // 如果硬链接计数为 0，并且没有进程打开该文件，则释放 inode 和数据块
    if (inode_acquire_nlinks(target_inode) == 0 && target_inode->ref==0)
    {
        // 释放 inode 和数据块
        inode_truncate(target_inode, 0);
        inode_free(target_inode->dev,target_inode->nr);
        
    }
    return 0;
}
int sys_mkdir(const char *path, uint16_t mode)
{
    if (!path)
    {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    
    // 解析 path，获取父目录的 inode
    const char* parent_path = path_dirname(path);
    inode_t *parent_inode = sys_namei(parent_path);
    if (!parent_inode)
    {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取要创建的文件名
    char *file_name = path_basename(path);
    if (!file_name)
    {
        dbg_error("Invalid file name\r\n");
        return -1;
    }

    return inode_mkdir(parent_inode,file_name,mode);

}

int sys_rmdir(const char *path)
{
    if (!path)
    {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    
    // 解析 path，获取父目录的 inode
    const char* parent_path = path_dirname(path);
    inode_t *parent_inode = sys_namei(parent_path);
    if (!parent_inode)
    {
        dbg_error("Parent directory not found\r\n");
        return -1;
    }

    // 获取要创建的文件名
    char *file_name = path_basename(path);
    if (!file_name)
    {
        dbg_error("Invalid file name\r\n");
        return -1;
    }

    return inode_rmdir(parent_inode,file_name);
}
static list_t mount_list;
/**
 * @brief 根据绝对路径找挂载点，匹配最长的绝对路径
 * @note 例如：传入绝对路径/home/zhao/test.txt   / 和 /home如果都是挂载点，那么选择/home
 */
mount_point_t *find_point_by_abspath(const char *abs_path)
{
    mount_point_t *ret = NULL;
    list_node_t *cur = mount_list.first;
    while (cur)
    {
        mount_point_t *point = list_node_parent(cur, mount_point_t, node);
        if (strcmp(abs_path, point->point_path) == 0)
        {
            if (ret != NULL && strlen(point->point_path) > strlen(ret->point_path))
            {
                ret = point;
            }
        }
        cur = cur->next;
    }
    return ret;
}
mount_point_t *find_point_by_inode(inode_t *inode)
{
    mount_point_t *ret = NULL;
    list_node_t *cur = mount_list.first;
    while (cur)
    {
        mount_point_t *point = list_node_parent(cur, mount_point_t, node);
        if (point->ipoint == inode)
        {
            return point;
        }
        cur = cur->next;
    }
    return ret;
}
int sys_mount(const char *path, int major, int minor, fs_type_t type)
{
    if (!path || !path_is_absolute(path)) {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    // 获取挂载点的 inode
    inode_t *mount_inode = sys_namei(path);
    if (!mount_inode) {
        dbg_error("Mount point not found\r\n");
        return -1;
    }

    // 检查挂载点是否已经被挂载
    if (mount_inode->mount) {
        dbg_error("Mount point already mounted\r\n");
        return -1;
    }

    // 创建挂载点结构
    mount_point_t *point = (mount_point_t *)kmalloc(sizeof(mount_point_t));
    if (!point) {
        dbg_error("Failed to allocate mount point\r\n");
        return -1;
    }

    // 初始化挂载点信息
    strncpy(point->point_path, path, MAX_PATH_BUF - 1);
    point->point_path[MAX_PATH_BUF - 1] = '\0';
    point->dev = MKDEV(major, minor);
    point->type = type;
    point->orign_dev = mount_inode->dev;
    point->orign_type = mount_inode->type;
    point->ipoint = mount_inode;

    // 将挂载点添加到挂载列表
    list_insert_last(&mount_list, &point->node);

    // 标记 inode 为已挂载状态
    mount_inode->mount = true;

    return 0;
}
int sys_unmount(const char *path)
{
    if (!path || !path_is_absolute(path)) {
        dbg_error("Invalid path\r\n");
        return -1;
    }

    // 查找挂载点结构
    mount_point_t *point = find_point_by_abspath(path);
    if (!point) {
        dbg_error("Mount point not found\r\n");
        return -1;
    }

    // 恢复原始设备信息
    inode_t *mount_inode = point->ipoint;
    mount_inode->dev = point->orign_dev;
    mount_inode->type = point->orign_type;
    mount_inode->mount = false;

    // 从挂载列表中移除
    list_remove(&mount_list, &point->node);

    // 释放挂载点结构
    kfree(point);

    return 0;
}
extern void fs_buffer_init(void);
/**挂载根文件系统 */
static void mount_root_fs(void)
{
    int major = FS_ROOT_MAJOR;
    int minor = FS_ROOT_MINOR;
    dev_t dev = MKDEV(major, minor);
    fs_type_t root_type = FS_ROOT_TYPE;
    inode_t *root_inode = inode_get(dev, 1);
    root_inode->mount = true;
    strcpy(root_inode->name, "/");

    mount_point_t *point = (mount_point_t *)kmalloc(sizeof(mount_point_t));
    point->dev = dev;
    point->type = FS_ROOT_TYPE;
    strcpy(point->point_path, "/");
    list_insert_last(&mount_list, &point->node);
}
void fs_init(void)
{
    fs_buffer_init();
    inode_tree_init();
    file_table_init();
    minixfs_init();
    mount_root_fs();
}

/**
 * @brief 获取根文件系统节点
 */
inode_t *get_root_inode(void)
{
    int major = FS_ROOT_MAJOR;
    int minor = FS_ROOT_MINOR;
    dev_t dev = MKDEV(major, minor);
    return get_dev_root_inode(dev);
}