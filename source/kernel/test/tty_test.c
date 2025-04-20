#include "dev/dev.h"
#include "types.h"
#include "task/task.h"
#include "string.h"
#include "fs/devfs/devfs.h"
static char read_buf[128];
void tty_test(void){
    int idx = 0;
    int major = DEV_MAJOR_TTY;
    int devfd = MKDEV(major,0);
    dev_open(devfd);
    const char* str = "hello,tty\r\n";
    dev_write(devfd,str,strlen(str),NULL);
    int devfd2 = MKDEV(major,1);
   dev_open(devfd2);
    while (true)
    {
        dev_read(devfd2,read_buf,128,NULL);
        dev_write(devfd,read_buf,128,NULL);
        memset(read_buf,0,128);
    }
    
    
    
}