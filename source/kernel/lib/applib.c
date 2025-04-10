#include "syscall/applib.h"


#include "syscall/syscall.h"
#include "task/signal.h"
#include "mem/malloc.h"
extern void *sys_call(void *);

int write(int fd, const char *buf, size_t len)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_write;
    arg.arg0 = fd;
    arg.arg1 = buf;
    arg.arg2 = len;
    ret = sys_call(&arg);
    return ret;
}
void sleep(uint32_t ms)
{
    syscall_args_t arg;
    arg.id = SYS_sleep;
    arg.arg0 = ms ;
    sys_call(&arg);
    wait_one_tick();
    return;
}

void yield(void)
{
    syscall_args_t arg;
    arg.id = SYS_yield;
    sys_call(&arg);
    return;
}
int fork(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_fork;
    ret = sys_call(&arg);
    return ret;
}
int getpid(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_getpid;
    ret = sys_call(&arg);
    return ret;
}
int getppid(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_getppid;
    ret = sys_call(&arg);
    return ret;
}
int wait(int *status)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_wait;
    arg.arg0 = status;
    ret = sys_call(&arg);
    return ret;
}
void exit(int status)
{
    syscall_args_t arg;
    arg.id = SYS_exit;
    arg.arg0 = status;
    sys_call(&arg);
}
int geterrno(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_geterrno;
    ret = sys_call(&arg);
    return ret;
}
int signal(int signum, void (*signal_handler)(int))
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_signal;
    arg.arg0 = signum;
    arg.arg1 = signal_handler;
    ret = sys_call(&arg);
    return ret;
}
int sigpromask(int how, const sigset_t *set, sigset_t *old)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_sigpromask;
    arg.arg0 = how;
    arg.arg1 = set;
    arg.arg2 = old;
    ret = sys_call(&arg);
    return ret;
}
int raise(int signum)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_raise;
    arg.arg0 = signum;
    ret = sys_call(&arg);
    return ret;
}
int kill(int pid, int signum)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_kill;
    arg.arg0 = pid;
    arg.arg1 = signum;
    ret = sys_call(&arg);
    return ret;
}
int sigpending(sigset_t *set)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_sigpending;
    arg.arg0 = set;
    ret = sys_call(&arg);
    return ret;
}
int pause(void)
{
    int ret;
    syscall_args_t arg;
    arg.id = SYS_pause;
    ret = sys_call(&arg);
    wait_one_tick();
    return ret;
}
void* malloc(size_t size){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_malloc;
    arg.arg0 = size;
    ret = sys_call(&arg);
    return ret;
}
void free(void* ptr){
  
    syscall_args_t arg;
    arg.id = SYS_free;
    arg.arg0 = ptr;
    sys_call(&arg);
}
int open(const char *path, int flags, uint32_t mode){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_open;
    arg.arg0 = path;
    arg.arg1 = flags;
    arg.arg2 = mode;
    ret = sys_call(&arg);
    return ret;
}
int read(int fd, char *buf, int len){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_read;
    arg.arg0 = fd;
    arg.arg1 = buf;
    arg.arg2 = len;
    ret = sys_call(&arg);
    return ret;
}
int lseek(int fd, int offset, int whence){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_lseek;
    arg.arg0 = fd;
    arg.arg1 = offset;
    arg.arg2 = whence;
    ret = sys_call(&arg);
    return ret;
}
int close(int fd){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_close;
    arg.arg0 = fd;
    ret = sys_call(&arg);
    return ret;
}
int execve(const char *path, char *const *argv, char *const *env){
    int ret;
    syscall_args_t arg;
    arg.id = SYS_execve;
    arg.arg0 = path;
    arg.arg1 = argv;
    arg.arg2 = env;
    ret = sys_call(&arg);
    return ret;
}
void printf(char *fmt, ...)
{

#define PRINT_MAX_STR_BUF_SIZE 512
    char str_buf[PRINT_MAX_STR_BUF_SIZE];
    va_list args;
    int offset = 0;
    // 清空缓冲区
    tmp_memset(str_buf, '\0', sizeof(str_buf));

    // 格式化日志信息
    va_start(args, fmt);
    tmp_vsprintf(str_buf + offset, fmt, args);
    va_end(args);
    // printf_inner(str_buf);
    write(stdout, str_buf, tmp_strlen(str_buf));
}

// 字符串拷贝函数 tmp_strcpy
void tmp_strcpy(char *dest, const char *src)
{
    while ((*dest++ = *src++))
        ;
}

// 有限长度字符串拷贝函数 tmp_strncpy
void tmp_strncpy(char *dest, const char *src, int size)
{
    int i;
    for (i = 0; i < size && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    for (; i < size; i++)
    {
        dest[i] = '\0';
    }
}

// 有限长度字符串比较函数 tmp_strncmp
int tmp_strncmp(const char *s1, const char *s2, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (s1[i] != s2[i])
        {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0')
        {
            return 0;
        }
    }
    return 0;
}

// 字符串长度函数 tmp_strlen
int tmp_strlen(const char *str)
{
    int len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}

// 内存拷贝函数 tmp_memcpy
void tmp_memcpy(void *dest, const void *src, int size)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    for (int i = 0; i < size; i++)
    {
        d[i] = s[i];
    }
}

void tmp_memmove(void *dest, const void *src, int size)
{
    static uint8_t cp_src_buf[1024] = {0};
    if (size > 1024)
    {
        // dbg_error("tmp_tmp_memmove size out of boundry\r\n");
        return;
    }
    tmp_memset(cp_src_buf, 0, 1024);
    tmp_memcpy(cp_src_buf, src, size);
    tmp_memcpy(dest, cp_src_buf, size);
    return;
}
// 内存填充函数 tmp_memset
void tmp_memset(void *dest, uint8_t v, int size)
{
    uint8_t *d = dest;
    for (int i = 0; i < size; i++)
    {
        d[i] = v;
    }
}

// 内存比较函数 tmp_memcmp
int tmp_memcmp(const void *d1, const void *d2, int size)
{
    const uint8_t *s1 = d1;
    const uint8_t *s2 = d2;
    for (int i = 0; i < size; i++)
    {
        if (s1[i] != s2[i])
        {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

void tmp_itoa(char *buf, uint32_t num, int base)
{
    char *ptr = buf;

    // 临时变量
    uint32_t temp = num;

    // 转换为字符串（低位在前）
    do
    {
        int remainder = temp % base;
        *ptr++ = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
    } while (temp /= base);

    // 添加字符串结束符
    *ptr = '\0';

    // 反转字符串
    char *start = buf;
    char *end = ptr - 1;
    while (start < end)
    {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
}

void tmp_itoa_with_padding(char *buf, uint32_t num, int base, int width, int zero_padding)
{
    char temp[32];
    char *ptr = temp;

    // 特殊情况：num == 0
    if (num == 0)
    {
        *ptr++ = '0';
    }
    else
    {
        while (num > 0)
        {
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
    if (zero_padding)
    {
        while (pad_len-- > 0)
        {
            *buf_ptr++ = '0';
        }
    }
    else
    {
        while (pad_len-- > 0)
        {
            *buf_ptr++ = ' ';
        }
    }

    // 反转字符串并复制到目标缓冲区
    ptr--;
    while (ptr >= temp)
    {
        *buf_ptr++ = *ptr--;
    }

    *buf_ptr = '\0'; // 添加字符串结束符
}

int tmp_sprintf(char *buffer, const char *fmt, ...)
{
    int ret;
    va_list args;
    va_start(args, fmt);
    ret = tmp_vsprintf(buffer, fmt, args);
    va_end(args);
    return ret;
}
int tmp_snprintf(char *buffer, int buf_size, const char *fmt, ...)
{
    int ret;
    va_list args;
    va_start(args, fmt);
    ret = tmp_vsnprintf(buffer, buf_size, fmt, args);
    va_end(args);
    return ret;
}

int tmp_vsprintf(char *buffer, const char *fmt, va_list args)
{
    char *buf_ptr = buffer;
    const char *fmt_ptr = fmt;

    while (*fmt_ptr)
    {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) != '%')
        {
            fmt_ptr++; // 跳过 '%'

            // 解析宽度和填充
            int width = 0;
            int zero_padding = 0;
            if (*fmt_ptr == '0')
            {
                zero_padding = 1;
                fmt_ptr++;
            }
            while (*fmt_ptr >= '0' && *fmt_ptr <= '9')
            {
                width = width * 10 + (*fmt_ptr - '0');
                fmt_ptr++;
            }

            // 解析格式化符号
            switch (*fmt_ptr)
            {
            case 'd':
            {
                int num = va_arg(args, int);
                char temp_buf[32];
                tmp_itoa_with_padding(temp_buf, num, 10, width, zero_padding);
                tmp_strcpy(buf_ptr, temp_buf);
                buf_ptr += tmp_strlen(temp_buf);
                break;
            }
            case 'x':
            {
                unsigned int num = va_arg(args, unsigned int);
                char temp_buf[32];
                tmp_itoa_with_padding(temp_buf, num, 16, width, zero_padding);
                tmp_strcpy(buf_ptr, temp_buf);
                buf_ptr += tmp_strlen(temp_buf);
                break;
            }
            case 's':
            {
                const char *str = va_arg(args, const char *);
                int len = tmp_strlen(str);

                // 填充宽度
                if (width > len)
                {
                    while (len < width)
                    {
                        *buf_ptr++ = ' ';
                        len++;
                    }
                }
                tmp_strcpy(buf_ptr, str);
                buf_ptr += len;
                break;
            }
            case 'c':
            {
                char c = (char)va_arg(args, int);
                *buf_ptr++ = c;
                break;
            }
            default:
            {
                *buf_ptr++ = *fmt_ptr;
                break;
            }
            }
        }
        else
        {
            *buf_ptr++ = *fmt_ptr;
        }
        fmt_ptr++;
    }

    *buf_ptr = '\0'; // 确保字符串以 NULL 结尾

    // 返回写入缓冲区的字节数
    return (int)(buf_ptr - buffer);
}
int tmp_vsnprintf(char *buffer, int buf_len, const char *fmt, va_list args)
{
    if (buffer == NULL || fmt == NULL || buf_len <= 0)
    {
        return -1; // 参数检查
    }

    char *buf_ptr = buffer;
    const char *fmt_ptr = fmt;
    int remaining = buf_len - 1; // 剩余可用空间（为终止符预留）

    while (*fmt_ptr && remaining > 0)
    {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) != '%')
        {
            fmt_ptr++; // 跳过 '%'

            // 解析宽度和填充
            int width = 0;
            int zero_padding = 0;
            if (*fmt_ptr == '0')
            {
                zero_padding = 1;
                fmt_ptr++;
            }
            while (*fmt_ptr >= '0' && *fmt_ptr <= '9')
            {
                width = width * 10 + (*fmt_ptr - '0');
                fmt_ptr++;
            }

            // 解析格式化符号
            switch (*fmt_ptr)
            {
            case 'd':
            {
                int num = va_arg(args, int);
                char temp_buf[32];
                tmp_itoa_with_padding(temp_buf, num, 10, width, zero_padding);
                int len = tmp_strlen(temp_buf);

                if (len > remaining)
                {
                    len = remaining; // 截断
                }
                tmp_strncpy(buf_ptr, temp_buf, len);
                buf_ptr += len;
                remaining -= len;
                break;
            }
            case 'x':
            {
                unsigned int num = va_arg(args, unsigned int);
                char temp_buf[32];
                tmp_itoa_with_padding(temp_buf, num, 16, width, zero_padding);
                int len = tmp_strlen(temp_buf);

                if (len > remaining)
                {
                    len = remaining; // 截断
                }
                tmp_strncpy(buf_ptr, temp_buf, len);
                buf_ptr += len;
                remaining -= len;
                break;
            }
            case 's':
            {
                const char *str = va_arg(args, const char *);
                int len = tmp_strlen(str);

                // 填充宽度
                if (width > len)
                {
                    while (len < width && remaining > 0)
                    {
                        *buf_ptr++ = ' ';
                        remaining--;
                        len++;
                    }
                }

                if (len > remaining)
                {
                    len = remaining; // 截断
                }
                tmp_strncpy(buf_ptr, str, len);
                buf_ptr += len;
                remaining -= len;
                break;
            }
            case 'c':
            {
                char c = (char)va_arg(args, int);
                if (remaining > 0)
                {
                    *buf_ptr++ = c;
                    remaining--;
                }
                break;
            }
            default:
            {
                if (remaining > 0)
                {
                    *buf_ptr++ = *fmt_ptr;
                    remaining--;
                }
                break;
            }
            }
        }
        else
        {
            if (remaining > 0)
            {
                *buf_ptr++ = *fmt_ptr;
                remaining--;
            }
        }
        fmt_ptr++;
    }

    *buf_ptr = '\0'; // 确保字符串以 NULL 结尾

    // 返回写入缓冲区的字节数
    return (int)(buf_ptr - buffer);
}
/**
 * @brief 找到字符c第一次出现的位置
 */
char *tmp_strchr(const char *str, int c)
{
    // 将字符 c 转换为 char 类型
    char target = (char)c;

    // 遍历字符串，直到找到目标字符或到达字符串结尾
    while (*str)
    {
        if (*str == target)
        {
            return (char *)str;
        }
        str++;
    }

    // 如果没找到目标字符，返回 NULL
    return NULL;
}
/**
 * @brief 按delim分割字符串
 */
char *tmp_strtok(char *str, const char *delim)
{
    static char *current_pos = NULL;

    // 如果传入一个新的字符串，更新 current_pos 指针
    if (str != NULL)
    {
        current_pos = str;
    }

    // 如果当前字符串已经处理完，返回 NULL
    if (current_pos == NULL || *current_pos == '\0')
    {
        return NULL;
    }

    // 跳过分隔符
    while (*current_pos && tmp_strchr(delim, *current_pos))
    {
        current_pos++;
    }

    // 如果到达了字符串末尾，返回 NULL
    if (*current_pos == '\0')
    {
        return NULL;
    }

    // 标记单词的开始位置
    char *start = current_pos;

    // 找到下一个分隔符并将其替换为 '\0'
    while (*current_pos && !tmp_strchr(delim, *current_pos))
    {
        current_pos++;
    }

    if (*current_pos != '\0')
    {
        *current_pos = '\0';
        current_pos++;
    }

    return start;
}

void wait_one_tick(void){
    int a = 0; //等一个时间片，让实时中断去检查信号
    for (int i = 0; i < 0xFFFFF; i++)
    {
        a += 1;
        a += 2;
        a += 3;
    }
}