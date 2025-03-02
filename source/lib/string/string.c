#include "string.h"
#include "types.h"
#include "stdarg.h"
// 字符串拷贝函数 strcpy
void strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

// 有限长度字符串拷贝函数 strncpy
void strncpy(char *dest, const char *src, int size) {
    int i;
    for (i = 0; i < size && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < size; i++) {
        dest[i] = '\0';
    }
}

// 有限长度字符串比较函数 strncmp
int strncmp(const char *s1, const char *s2, int size) {
    for (int i = 0; i < size; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

// 字符串长度函数 strlen
int strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// 内存拷贝函数 memcpy
void memcpy(void *dest, const void *src, int size) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    for (int i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

void memmove(void *dest, const void *src, int size)
{
    static uint8_t cp_src_buf[1024] = {0};
    if(size >1024)
    {
        //dbg_error("memmove size out of boundry\r\n");
        return;
    }
    memset(cp_src_buf,0,1024);
    memcpy(cp_src_buf,src,size);
    memcpy(dest,cp_src_buf,size);
    return;

}
// 内存填充函数 memset
void memset(void *dest, uint8_t v, int size) {
    uint8_t *d = dest;
    for (int i = 0; i < size; i++) {
        d[i] = v;
    }
}

// 内存比较函数 memcmp
int memcmp(const void *d1, const void *d2, int size) {
    const uint8_t *s1 = d1;
    const uint8_t *s2 = d2;
    for (int i = 0; i < size; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

void itoa(char *buf, uint32_t num, int base) {
    char *ptr = buf;

    // 临时变量
    uint32_t temp = num;

    // 转换为字符串（低位在前）
    do {
        int remainder = temp % base;
        *ptr++ = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
    } while (temp /= base);

    // 添加字符串结束符
    *ptr = '\0';

    // 反转字符串
    char *start = buf;
    char *end = ptr - 1;
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
}


static void itoa_with_padding(char *buf, uint32_t num, int base, int width, int zero_padding) {
    char temp[32];
    char *ptr = temp;

    // 特殊情况：num == 0
    if (num == 0) {
        *ptr++ = '0';
    } else {
        while (num > 0) {
            int remainder = num % base;
            *ptr++ = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
            num /= base;
        }
    }
    *ptr = '\0';

    // 计算实际长度
    int len = ptr - temp;
    int pad_len = width > len ? width - len : 0;

    // 填充零或空格
    char *buf_ptr = buf;
    if (zero_padding) {
        while (pad_len-- > 0) {
            *buf_ptr++ = '0';
        }
    } else {
        while (pad_len-- > 0) {
            *buf_ptr++ = ' ';
        }
    }

    // 反转字符串并复制到目标缓冲区
    ptr--;
    while (ptr >= temp) {
        *buf_ptr++ = *ptr--;
    }

    *buf_ptr = '\0'; // 添加字符串结束符
}

int sprintf(char *buffer, const char *fmt, ...) {
    int ret;
    va_list args;
    va_start(args, fmt);
    ret = vsprintf(buffer, fmt, args);
    va_end(args);
    return ret;
}
int snprintf(char * buffer, int buf_size,const char * fmt, ...){
    int ret;
    va_list args;
    va_start(args, fmt);
    ret = vsnprintf(buffer, buf_size,fmt, args);
    va_end(args);
    return ret;
}


int vsprintf(char *buffer, const char *fmt, va_list args) {
    char *buf_ptr = buffer;
    const char *fmt_ptr = fmt;

    while (*fmt_ptr) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) != '%') {
            fmt_ptr++;  // 跳过 '%'

            // 解析宽度和填充
            int width = 0;
            int zero_padding = 0;
            if (*fmt_ptr == '0') {
                zero_padding = 1;
                fmt_ptr++;
            }
            while (*fmt_ptr >= '0' && *fmt_ptr <= '9') {
                width = width * 10 + (*fmt_ptr - '0');
                fmt_ptr++;
            }

            // 解析格式化符号
            switch (*fmt_ptr) {
                case 'd': {
                    int num = va_arg(args, int);
                    char temp_buf[32];
                    itoa_with_padding(temp_buf, num, 10, width, zero_padding);
                    strcpy(buf_ptr, temp_buf);
                    buf_ptr += strlen(temp_buf);
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp_buf[32];
                    itoa_with_padding(temp_buf, num, 16, width, zero_padding);
                    strcpy(buf_ptr, temp_buf);
                    buf_ptr += strlen(temp_buf);
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char *);
                    int len = strlen(str);

                    // 填充宽度
                    if (width > len) {
                        while (len < width) {
                            *buf_ptr++ = ' ';
                            len++;
                        }
                    }
                    strcpy(buf_ptr, str);
                    buf_ptr += len;
                    break;
                }
                case 'c': {
                    char c = (char) va_arg(args, int);
                    *buf_ptr++ = c;
                    break;
                }
                default: {
                    *buf_ptr++ = *fmt_ptr;
                    break;
                }
            }
        } else {
            *buf_ptr++ = *fmt_ptr;
        }
        fmt_ptr++;
    }

    *buf_ptr = '\0'; // 确保字符串以 NULL 结尾

    // 返回写入缓冲区的字节数
    return (int)(buf_ptr - buffer);
}
int vsnprintf(char * buffer, int buf_len,const char * fmt, va_list args){
    if (buffer == NULL || fmt == NULL || buf_len <= 0) {
        return -1;  // 参数检查
    }

    char *buf_ptr = buffer;
    const char *fmt_ptr = fmt;
    int remaining = buf_len - 1;  // 剩余可用空间（为终止符预留）

    while (*fmt_ptr && remaining > 0) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) != '%') {
            fmt_ptr++;  // 跳过 '%'

            // 解析宽度和填充
            int width = 0;
            int zero_padding = 0;
            if (*fmt_ptr == '0') {
                zero_padding = 1;
                fmt_ptr++;
            }
            while (*fmt_ptr >= '0' && *fmt_ptr <= '9') {
                width = width * 10 + (*fmt_ptr - '0');
                fmt_ptr++;
            }

            // 解析格式化符号
            switch (*fmt_ptr) {
                case 'd': {
                    int num = va_arg(args, int);
                    char temp_buf[32];
                    itoa_with_padding(temp_buf, num, 10, width, zero_padding);
                    int len = strlen(temp_buf);

                    if (len > remaining) {
                        len = remaining;  // 截断
                    }
                    strncpy(buf_ptr, temp_buf, len);
                    buf_ptr += len;
                    remaining -= len;
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp_buf[32];
                    itoa_with_padding(temp_buf, num, 16, width, zero_padding);
                    int len = strlen(temp_buf);

                    if (len > remaining) {
                        len = remaining;  // 截断
                    }
                    strncpy(buf_ptr, temp_buf, len);
                    buf_ptr += len;
                    remaining -= len;
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char *);
                    int len = strlen(str);

                    // 填充宽度
                    if (width > len) {
                        while (len < width && remaining > 0) {
                            *buf_ptr++ = ' ';
                            remaining--;
                            len++;
                        }
                    }

                    if (len > remaining) {
                        len = remaining;  // 截断
                    }
                    strncpy(buf_ptr, str, len);
                    buf_ptr += len;
                    remaining -= len;
                    break;
                }
                case 'c': {
                    char c = (char) va_arg(args, int);
                    if (remaining > 0) {
                        *buf_ptr++ = c;
                        remaining--;
                    }
                    break;
                }
                default: {
                    if (remaining > 0) {
                        *buf_ptr++ = *fmt_ptr;
                        remaining--;
                    }
                    break;
                }
            }
        } else {
            if (remaining > 0) {
                *buf_ptr++ = *fmt_ptr;
                remaining--;
            }
        }
        fmt_ptr++;
    }

    *buf_ptr = '\0';  // 确保字符串以 NULL 结尾

    // 返回写入缓冲区的字节数
    return (int)(buf_ptr - buffer);
}
char* strchr(const char *str, int c) {
    // 将字符 c 转换为 char 类型
    char target = (char)c;

    // 遍历字符串，直到找到目标字符或到达字符串结尾
    while (*str) {
        if (*str == target) {
            return (char*)str;
        }
        str++;
    }

    // 如果没找到目标字符，返回 NULL
    return NULL;
}

char* strtok(char *str, const char *delim) {
    static char *current_pos = NULL;
    
    // 如果传入一个新的字符串，更新 current_pos 指针
    if (str != NULL) {
        current_pos = str;
    }
    
    // 如果当前字符串已经处理完，返回 NULL
    if (current_pos == NULL || *current_pos == '\0') {
        return NULL;
    }
    
    // 跳过分隔符
    while (*current_pos && strchr(delim, *current_pos)) {
        current_pos++;
    }

    // 如果到达了字符串末尾，返回 NULL
    if (*current_pos == '\0') {
        return NULL;
    }

    // 标记单词的开始位置
    char *start = current_pos;

    // 找到下一个分隔符并将其替换为 '\0'
    while (*current_pos && !strchr(delim, *current_pos)) {
        current_pos++;
    }

    if (*current_pos != '\0') {
        *current_pos = '\0';
        current_pos++;
    }

    return start;
}



