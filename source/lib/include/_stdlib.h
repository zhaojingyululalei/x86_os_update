#ifndef __STDLIB_H
#define __stdLIB_H

#include "types.h"
#include "stdarg.h"
#define stdin   0
#define stdout  1
#define stderr  2


int write(int fd,const char* buf,size_t len);
void printf(char *fmt, ...);
#endif