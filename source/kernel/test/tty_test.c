#include "dev/dev.h"
#include "types.h"
#include "task/task.h"

static char read_buf[128];
void tty_test(void){
    int idx = 0;
    dev_open(DEV_MAJOR_TTY,0,NULL);
   int devfd = dev_open(DEV_MAJOR_TTY,1,NULL);
    while (true)
    {
        dev_read(devfd,0,read_buf,128,0);
    }
    
    
    
}