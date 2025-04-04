#ifndef __FS_H
#define __FS_H

#include "types.h"
int sys_open(const char *path, int flags, uint32_t mode);
int sys_read(int fd, char *buf, int len);
int sys_write(int fd,const char* buf,size_t len);
int sys_lseek(int fd, int offset, int whence);
int sys_close(int fd);
int sys_fsync(int fd);
#endif