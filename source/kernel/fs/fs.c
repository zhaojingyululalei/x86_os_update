
#include "types.h"
#include "fs/fs.h"
#include "stdlib.h"
#include "cpu_instr.h"
#define DISK_SECTOR_SIZE 512
uint8_t tmp_app_buffer[512 * 1024];
uint8_t *tmp_pos;
#define SHELL_FD    1024
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
int sys_open(const char *path, int flags, mode_t mode)
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
}

int sys_read(int fd, char *buf, int len)
{
    if (fd == SHELL_FD)
    {
        memcpy(buf, tmp_pos, len);
        tmp_pos += len;
        return len;
    }
}

int sys_write(int fd, const char *buf, size_t len)
{
    if(fd == stdout){
        serial_print(buf);
        return;
    }
}
int sys_close(int fd)
{
}

int sys_lseek(int fd, int offset, int whence)
{
    if (fd == SHELL_FD)
    {
        tmp_pos = tmp_app_buffer + offset;
        return;
    }
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
int sys_readdir(DIR *dir, dirent_t *dirent)
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
extern void fs_buffer_init(void);
//extern void file_table_init(void);
void fs_init(void)
{
    fs_buffer_init();
    //file_table_init();
}
void fs_register(fs_type_t type, fs_op_t *opt)
{
}