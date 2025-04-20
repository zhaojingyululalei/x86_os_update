
#include "fs/devfs/devfs.h"
#include "fs/fs.h"
#include "printk.h"
#define SECTOR_SIZE 512
extern fs_op_t devfs_opts;
void devfs_test(void)
{
    file_t file;
    file.pos = 0;
    
    int major = 7;
    int minor = 16;
    
    file.dev = MKDEV(major,minor);
    

    dev_show_all();
    int sector_cnt = 16;
    uint8_t *write_buf = kmalloc(SECTOR_SIZE * sector_cnt);
    uint8_t *read_buf = kmalloc(SECTOR_SIZE * sector_cnt);
    for (int i = 0; i < SECTOR_SIZE * 2; i++)
    {
        if (i % 2 == 0)
        {
            write_buf[i] = '0x66';
        }
        else
        {
            write_buf[i] = '0xbb';
        }
    }

    devfs_opts.open(&file,NULL,0,0);
    devfs_opts.write(&file,write_buf,SECTOR_SIZE * sector_cnt);
    devfs_opts.read(&file,read_buf,SECTOR_SIZE*sector_cnt);
    
    int ret = strncmp(write_buf, read_buf, SECTOR_SIZE * sector_cnt);
    if (ret == 0)
    {
        dbg_info("dma read write successful\r\n");
    }
    else
    {
        dbg_info("dma read write failed\r\n");
    }
    kfree(write_buf);
    kfree(read_buf);
}