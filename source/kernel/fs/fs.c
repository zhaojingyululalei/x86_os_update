
#include "fs/fs.h"
#include "stdlib.h"
#include "dev/serial.h"
#include "cpu_instr.h"
#include "printk.h"
#define DISK_SECTOR_SIZE 512
uint8_t tmp_app_buffer[512 * 1024];
uint8_t *tmp_pos;
int shell_fd = 9;

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
        return shell_fd;
    }
    else{
        dbg_error("unkown file:%s\r\n",path);
    }
    
    return 0;
}

int sys_read(int fd, char *buf, int len)
{
    if (fd == shell_fd)
    {
        memcpy(buf, tmp_pos, len);
        tmp_pos += len;
        return len;
    }
    else
    {
        dbg_error("unkown fd:%d\r\n",fd);
    }
    return 0;
}

int sys_write(int fd,const char* buf,size_t len){
    if(fd == stdout){
        serial_print(buf);
    }
}
int sys_lseek(int fd, int offset, int whence)
{
    if (fd == shell_fd)
    {
        tmp_pos = tmp_app_buffer + offset;
    }
    return 0;
}

int sys_close(int fd)
{
    sys_fsync(fd);
    return 0;
}
/**
 * @brief 将该文件同步回磁盘
 */
int sys_fsync(int fd){
    return 0;
}

extern void fs_buffer_init(void);
extern void minix_super_init(void);
void fs_init(void)
{
    fs_buffer_init();
    minix_super_init();
}