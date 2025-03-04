#ifndef __CONSOLE32_H
#define __CONSOLE32_H
#include "types.h"
void console_init(void) ;
void console_write(char *buf, uint32_t count);
#endif