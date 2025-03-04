#ifndef __PRINTK_32_H
#define __PRINTK_32_H
typedef char* va_list;

#define va_start(p, count) (p = (va_list)&count + sizeof(char*))
#define va_arg(p, t) (*(t*)((p += sizeof(char*)) - sizeof(char*)))
#define va_end(p) (p = 0)
int vsprintf(char *buf, const char *fmt, va_list args) ;
int printk(const char * fmt, ...);
#endif