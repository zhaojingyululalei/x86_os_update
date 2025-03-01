
#ifndef __STDARG_H
#define __STDARG_H
typedef char *va_list;

#define stack_size(t) (sizeof(t) <= sizeof(char *) ? sizeof(char *) : sizeof(t))
#define va_start(v, l) (v = (va_list)&l + sizeof(char *))
#define va_arg(v, t) (*(t *)((v += stack_size(t)) - stack_size(t)))
#define va_end(v) (v = (va_list)0)

#endif