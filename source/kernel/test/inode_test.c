#include "fs/buffer.h"
#include "printk.h"
#include "dev/dev.h"
#include "dev/ide.h"
#include "fs/minix_fs.h"

void inode_test(void){
    inode_t inode;
    get_dev_inode(&inode,7,16,1);
    minix_inode_t* inode_data = inode.data;
    buffer_t* bufa = fs_read_to_buffer(7,16,inode_data->zone[0],false);
    minix_dentry_t* dir= bufa->data;
    
    while (dir->nr)
    {
        dbg_info("dir name = %s\r\n",dir->name);
        dir++;
    }
    inode_get_block(&inode,8,true);
}