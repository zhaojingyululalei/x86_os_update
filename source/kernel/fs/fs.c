
#include "fs/fs.h"
#include "_stdlib.h"
#include "dev/serial.h"
int sys_write(int fd,const char* buf,size_t len){
    if(fd == stdout){
        serial_print(buf);
    }
}