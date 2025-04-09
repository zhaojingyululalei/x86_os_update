#include "fs/minix_fs.h"
#include "fs/buffer.h"
#include "printk.h"
#include "mem/kmalloc.h"
static super_block_t root;
/**
 * @brief 获取某个分区的超级块
 */
int get_dev_super_block(super_block_t* superb,int major,int minor){
    if(!superb){
        return -1;
    }
    buffer_t* super_buf = fs_read_to_buffer(major,minor,1,true);
    minix_super_t* minix_sb = (minix_super_t*)super_buf->data;
    if(minix_sb->magic!=MINIX1_MAGIC){
        dbg_error("minix fs root super block get failz!!!\r\n");
        return;
    }
    superb->data = minix_sb;
    superb->major = major;
    superb->minor = minor;

    buffer_t* imap_buf = fs_read_to_buffer(major,minor,2,true);
    superb->inode_map = (uint8_t*)imap_buf->data;
    
    buffer_t* zmap_buf = fs_read_to_buffer(major,minor,2+minix_sb->imap_blocks,true);
    superb->zone_map = (uint8_t*)zmap_buf->data;

    return 0;
}
/**
 * @brief 获取根文件系统所在的超级快
 */
super_block_t* get_root_super_block(void){
    return &root;

}
static void mount_root(void){
    int ret = get_dev_super_block(&root,MINIX1FS_ROOT_MAJOR,MINIX1FS_ROOT_MINOR);
    if(ret < 0){
        dbg_error("minix fs root super block get failz!!!\r\n");
        return;
    }
    inode_t* iroot = (inode_t*)kmalloc(sizeof(inode_t));
    get_dev_inode(iroot,root.major,root.minor,1);
    root.iroot = iroot;
    root.imount = root.iroot;
    return;
}
void minix_super_init(void){
    mount_root();
}