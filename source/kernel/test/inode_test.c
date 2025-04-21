#include "fs/minix/minix_supper.h"
#include "fs/devfs/devfs.h"
void inode_test(void)
{
   int major = 7;
   int minor = 16;
   dev_t dev = MKDEV(major,minor);
   minix_super_t* super = minix_get_super(dev);
   
    
    
    
    return;
}
