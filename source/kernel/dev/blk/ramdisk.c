#include  "dev/ramdisk.h"
#include "printk.h"
#include "fs/devfs/devfs.h"
#include "cpu_cfg.h"
#include "dev/dev.h"
#include "mem/kmalloc.h"
#include "dev/ide.h"
#include "string.h"
static ram_disk_t ramdisk[RAMDISK_NR];
static ram_disk_t* dev_get_data(dev_t devfd)
{
    int major = MAJOR(devfd);
    int minor = MINOR(devfd);
    ASSERT(major == DEV_MAJOR_RAMDISK);
    if(minor >= RAMDISK_NR)
    {
        dbg_error("get ramdisk failz\r\n");
        return  NULL;
    }
    return &ramdisk[minor];
}
static int ram_disk_open(dev_t devfd) {
    ram_disk_t *ram_disk = (ram_disk_t *)dev_get_data(devfd);
    if (!ram_disk) {
        dbg_error("Failed to get ram disk data\r\n");
        return -1;
    }
    dbg_info("Open ram disk %s success\r\n", ram_disk->name);
    return 0;
}
/**
 * @brief pos的单位是块
 * @brief 读必须以块为单位读
 */
static int ram_disk_read(dev_t devfd, char *buf, int size, int *pos) {
    ram_disk_t *ram_disk = (ram_disk_t *)dev_get_data(devfd);
    if (!ram_disk) {
        dbg_error("Failed to get ram disk data\r\n");
        return -1;
    }
    int cnt = size/BLOCK_SIZE;//读多少块
    
    // 检查读取范围是否越界
    if ((*pos + size)*BLOCK_SIZE > ram_disk->total_sectors * ram_disk->sector_size) {
        dbg_error("Read out of boundary\r\n");
        return -2;
    }

    // 从内存缓冲区拷贝数据
    memcpy(buf, ram_disk->buffer + (*pos)*BLOCK_SIZE, cnt*BLOCK_SIZE);
    
    return size;
}
/**
 * @brief pos的单位是块
 * @note 写可以任意写，不一定以块为单位
 */
static int ram_disk_write(dev_t devfd, char *buf, int size, int *pos) {
    ram_disk_t *ram_disk = (ram_disk_t *)dev_get_data(devfd);
    if (!ram_disk) {
        dbg_error("Failed to get ram disk data\r\n");
        return -1;
    }
    
    // 检查写入范围是否越界
    if ((*pos + size)*BLOCK_SIZE > ram_disk->total_sectors * ram_disk->sector_size) {
        dbg_error("Write out of boundary\r\n");
        return -2;
    }

    // 将数据写入内存缓冲区
    memcpy(ram_disk->buffer + (*pos)*BLOCK_SIZE, buf, size);
    
    return size;
}

static int ram_disk_control(dev_t devfd, int cmd, int arg0, int arg1) {
    ram_disk_t *ram_disk = (ram_disk_t *)dev_get_data(devfd);
    if (!ram_disk) {
        dbg_error("Failed to get ram disk data\r\n");
        return -1;
    }

    switch (cmd) {
        case DEV_CMD_GETINFO:
            return (int)ram_disk; // 返回设备结构体指针
        case RAMDISK_CMD_SECTOR_COUNT:
            return ram_disk->total_sectors; // 返回总扇区数
        case RAMDISK_CMD_SECTOR_SIZE:
            return ram_disk->sector_size; // 返回扇区大小
        case DEV_CMD_COMMENTS:
            dbg_info("DEV_CMD_COMMENTS: return all cmd \r\n");
            dbg_info("DEV_CMD_GETINFO : return ram_disk_t struct\r\n");
            dbg_info("RAMDISK_CMD_SECTOR_COUNT : return total sectors\r\n");
            dbg_info("RAMDISK_CMD_SECTOR_SIZE: return sector size\r\n");
            dbg_info("RAMDISK_CMD_GET_FS_TYPE:return fs type\r\n");
            dbg_info("RAMDISK_CMD_SET_FS_TYPE:arg0 is fs type\r\n");
            return 0;
        case RAMDISK_CMD_GET_FS_TYPE:
            return ram_disk->type;

        case RAMDISK_CMD_SET_FS_TYPE:
            ram_disk->type = arg0;
            return;
        default:
            dbg_error("Unknown command %d\r\n", cmd);
            return -2;
    }
}

/**
 * @brief pos的单位是块
 */
static int ram_disk_lseek(dev_t devfd, int *pos, int offset, int whence) {
    ram_disk_t *ram_disk = (ram_disk_t *)dev_get_data(devfd);
    if (!ram_disk) {
        dbg_error("Failed to get ram disk data\r\n");
        return -1;
    }

    *pos = offset;
    return 0;
}


static void ram_disk_close(dev_t devfd) {
    ram_disk_t *ram_disk = (ram_disk_t *)dev_get_data(devfd);
    if (!ram_disk) {
        dbg_error("Failed to get ram disk data\r\n");
        return;
    }
    dbg_info("Close ram disk %s success\r\n", ram_disk->name);
}

void ramdisk_init(void)
{
    uint32_t size = 5*1024*1024;
    uint32_t sec_size = 512;
    for (int i = 0; i < RAMDISK_NR; i++)
    {
        ramdisk[i].buffer = (uint8_t*)kmalloc(size);
        memset(ramdisk[i].name,0,16);
        sprintf(ramdisk[i].name,"ramdisk%d",i);
        ramdisk[i].sector_size = 512;
        ramdisk[i].total_sectors = size / sec_size;
        dev_install(DEV_TYPE_BLOCK,DEV_MAJOR_RAMDISK,i,ramdisk[i].name,&ramdisk[i],&ram_disk_ops);
    }
    
}

dev_ops_t ram_disk_ops = {
    .open = ram_disk_open,
    .read = ram_disk_read,
    .write = ram_disk_write,
    .control = ram_disk_control,
    .lseek = ram_disk_lseek,
    .close = ram_disk_close
};