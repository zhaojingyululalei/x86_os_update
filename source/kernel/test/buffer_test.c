#include "fs/buffer.h"
#include "printk.h"
#include "dev/dev.h"
#include "dev/ide.h"
#include "fs/minix_fs.h"
static void test(void){
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
void buffer_test(void){
    dev_show_all();
    const char* part_name = "/dev/sdb1";
    int major = DEV_MAJOR_IDE_PART;
    int minor = part_get_minor_by_name(part_name);
    buffer_t* boot = fs_read_to_buffer(major,minor,0,false);
    buffer_t* super = fs_read_to_buffer(major,minor,1,false);
    minix_super_t * sb = (minix_super_t*)super->data;
    ASSERT(sb->magic == MINIX1_MAGIC);
    //inode位图
    buffer_t* inode_map = fs_read_to_buffer(major,minor,2,false);
    //block 位图
    buffer_t* block_map = fs_read_to_buffer(major,minor,2+sb->imap_blocks,false);

    //读取第一个inode,根目录
    buffer_t* inode_1_buf = fs_read_to_buffer(major,minor,2+sb->imap_blocks+sb->zmap_blocks,false);
    minix_inode_t* inode_1 = inode_1_buf->data;

    buffer_t* bufa = fs_read_to_buffer(major,minor,inode_1->zone[0],false);
    minix_dentry_t* dir= bufa->data;
    
    while (dir->nr)
    {
        dbg_info("dir name = %s\r\n",dir->name);
        dir++;
    }
    /**
     * 输出
     * .
     * ..
     * test_dir
     */
    int blk_idx = minix_dblk_alloc(major,minor);
    minix_dblk_free(major,minor,blk_idx);

    int inode_idx = minix_inode_alloc(major,minor);
    minix_inode_free(major,minor,inode_idx);
    if(!boot){
        return;
    }
}