
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

int sys_stat(int fd)
{
}
int sys_ioctl(int fd, int cmd, int arg0, int arg1)
{
}

int sys_fsync(int fd)
{
}
int sys_opendir(const char *path, DIR *dir)
{
}
int sys_readdir(DIR *dir, struct dirent *dirent)
{
}
int sys_closedir(DIR *dir)
{
}

int sys_link(const char *old_path, const char *new_path)
{
}
int sys_unlink(const char *path)
{
}
int sys_mkdir(const char *path, uint16_t mode)
{
}

int sys_rmdir(const char *path)
{
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
}
int sys_unmount(const char *path)
{
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
void fs_register(fs_type_t type, fs_op_t *opt)
{
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