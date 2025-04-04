#include "fs/buffer.h"
#include "printk.h"
#include "dev/dev.h"
#include "dev/ide.h"

void buffer_test(void){
    dev_show_all();
    int sector_cnt = 24;
    uint8_t *write_buf = kmalloc(SECTOR_SIZE * sector_cnt);
    for (int i = 0; i < SECTOR_SIZE * 2; i++)
    {
        write_buf[i] = 'a'+i%24;
    }
    int major = 6;
    int minor = 3;
    int blk = 2;
    int devfd = dev_open(6,3,0);
    dev_write(devfd,0,write_buf,SECTOR_SIZE * sector_cnt,0);
    dev_close(devfd,0);
    buffer_t* buf = fs_read_to_buffer(major,minor,blk,false);
    int ret = strncmp(write_buf+blk*2*SECTOR_SIZE, buf->data, BLOCK_SIZE);
    if (ret == 0)
    {
        dbg_info("buffer successful\r\n");
    }
    else
    {
        dbg_info("buffer failed\r\n");
    }
    blk=6;
    buf = fs_read_to_buffer(major,minor,blk,false);
    ret = strncmp(write_buf+blk*2*SECTOR_SIZE, buf->data, BLOCK_SIZE);
    if (ret == 0)
    {
        dbg_info("buffer successful\r\n");
    }
    else
    {
        dbg_info("buffer failed\r\n");
    }
    blk=10;
    buf = fs_read_to_buffer(major,minor,blk,false);
    ret = strncmp(write_buf+blk*2*SECTOR_SIZE, buf->data, BLOCK_SIZE);
    if (ret == 0)
    {
        dbg_info("buffer successful\r\n");
    }
    else
    {
        dbg_info("buffer failed\r\n");
    }
    kfree(write_buf);

    sys_sync();
}