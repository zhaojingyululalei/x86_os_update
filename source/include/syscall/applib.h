#ifndef __APP_LIB_H
#define __APP_LIB_H

#include "types.h"
#include "time/time.h"
#include "stdarg.h"
#define stdin   0
#define stdout  1
#define stderr  2




// 系统调用函数
int write(int fd, const char *buf, size_t len);
void sleep(uint32_t ms);
void yield(void);
int fork(void);
int getpid(void);
int getppid(void);
// printf 相关
void printf(char *fmt, ...);

// 字符串相关
void tmp_strcpy(char *dest, const char *src);
void tmp_strncpy(char *dest, const char *src, int size);
int tmp_strncmp(const char *s1, const char *s2, int size);
int tmp_strlen(const char *str);
char *tmp_strchr(const char *str, int c);
char *tmp_strtok(char *str, const char *delim);

// 内存操作
void tmp_memcpy(void *dest, const void *src, int size);
void tmp_memmove(void *dest, const void *src, int size);
void tmp_memset(void *dest, uint8_t v, int size);
int tmp_memcmp(const void *d1, const void *d2, int size);

// 数字转换
void tmp_itoa(char *buf, uint32_t num, int base);
void tmp_itoa_with_padding(char *buf, uint32_t num, int base, int width, int zero_padding);

// 格式化输出
int tmp_sprintf(char *buffer, const char *fmt, ...);
int tmp_snprintf(char *buffer, int buf_size, const char *fmt, ...);
int tmp_vsprintf(char *buffer, const char *fmt, va_list args);
int tmp_vsnprintf(char *buffer, int buf_len, const char *fmt, va_list args);



#endif