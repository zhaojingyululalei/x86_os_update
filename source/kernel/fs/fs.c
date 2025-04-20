
#include "fs/fs.h"
#include "stdlib.h"
#include "dev/serial.h"
#include "cpu_instr.h"
#include "printk.h"
#include "fs/file.h"
#include "fs/path.h"
#include "task/task.h"
#include "task/sche.h"
#include "fs/stat.h"
#include "fs/fs_cfg.h"
#include "fs/mount.h"
#include "fs/buffer.h"
#define DISK_SECTOR_SIZE 512
uint8_t tmp_app_buffer[512 * 1024];
uint8_t *tmp_pos;
#define SHELL_FD    1024
fs_op_t* fs_arr[16];//最多支持16个文件系统

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
static int is_fd_bad (int file) {

	return 0;
}
int sys_open(const char *path, int flags, uint32_t mode)
{
    int ret;

    ret = strncmp(path, "/shell", 6);
    // 路径解析正确
    if (ret == 0)
    {
        read_disk(10240, 512 * 1024/ 512, tmp_app_buffer);
        tmp_pos = tmp_app_buffer;
        return SHELL_FD;
    }
    // 分配文件描述符链接
	file_t * file = file_alloc();
    int fd = task_alloc_fd(file);

	if (!file) {
		return -1;
	}
    mount_point_t* point = find_point_by_path(path);
    fs_type_t type = point->type;
    file->fs_type = type;
    fs_arr[type]->open(file,path,flags,mode);

    
    return 0;
}

int sys_read(int fd, char *buf, int len)
{
    if (fd == SHELL_FD)
    {
        memcpy(buf, tmp_pos, len);
        tmp_pos += len;
        return len;
    }
    file_t* file = task_file(fd);
    if(!file)
    {
        return -1;
    }
    int ret = fs_arr[file->fs_type]->read(file,buf,len);
    return ret;
}

int sys_write(int fd,const char* buf,size_t len){
    if(fd == stdout){
        serial_print(buf);
        return;
    }
    file_t* file = task_file(fd);
    if(!file)
    {
        return -1;
    }
    int ret = fs_arr[file->fs_type]->write(file,buf,len);
    return ret;
}
int sys_lseek(int fd, int offset, int whence)
{
    if (fd == SHELL_FD)
    {
        tmp_pos = tmp_app_buffer + offset;
        return;
    }
    file_t* file = task_file(fd);
    if(!file)
    {
        return -1;
    }
    int ret = fs_arr[file->fs_type]->lseek(file,offset,whence);
    return ret;
}

int sys_close(int fd)
{
    
    return 0;
}
/**
 * @brief 将该文件同步回磁盘
 */
int sys_fsync(int fd) {
    // 获取文件描述符对应的文件对象
    file_t *file = task_file(fd);
    if (!file) {
        dbg_error("Invalid file descriptor: %d\r\n", fd);
        return -1;
    }

    

    return 0;
}

int sys_mkdir(const char* path, uint16_t mode) {
    // 解析路径，获取父目录的 inode
    //minix_inode_desc_t *parent_inode = minix_named(path);
    minix_inode_desc_t *parent_inode = minix_named(path);
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
    minix_mkdir(parent_inode,);
}
int sys_rmdir(const char* path) {
    return minix_rmdir(path);
}
extern void fs_buffer_init(void);
extern void minix_fs_init(void);
extern void file_table_init(void);
void fs_init(void)
{

    fs_buffer_init();
    minix_fs_init();
    sys_mount("/",FS_ROOT_MAJOR,FS_ROOT_MINOR,FS_MINIX);
    file_table_init();
}

void fs_register(fs_type_t type,fs_op_t* opt)
{
    fs_arr[type] = opt  ;
}