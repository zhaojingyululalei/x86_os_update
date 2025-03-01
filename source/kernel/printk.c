#include "string.h"
#include "dev/serial.h"
#include "printk.h"
#include "cpu_instr.h"
#define MAX_STR_BUF_SIZE 256
void dbg_print(int level, const char *file, const char *func, int line, const char *fmt, ...)
{
    if (level > DBG_LEVEL_CTL_SET)
    {
        return;
    }

    static const char *title[] = {
        [DBG_LEVEL_ERROR] = "error",
        [DBG_LEVEL_WARNING] = "warning",
        [DBG_LEVEL_INFO] = "info",
        [DBG_LEVEL_NONE] = "none"};

    char str_buf[MAX_STR_BUF_SIZE];
    
    int offset=0;
    // 清空缓冲区
    memset(str_buf, '\0', MAX_STR_BUF_SIZE);
    // 组装文件、函数和行号信息
    if (level != DBG_LEVEL_INFO)
    {
        sprintf(str_buf, "[%s] in file:%s, func:%s, line:%d: \r\n", title[level], file, func, line);
        offset = strlen(str_buf);
    }

    va_list args;
    // 格式化日志信息
    va_start(args, fmt);
    vsprintf(str_buf + offset, fmt, args);
    va_end(args);

    ASSERT(str_buf[MAX_STR_BUF_SIZE-1]=='\0');
    str_buf[MAX_STR_BUF_SIZE-1] = '\0';//确保最后一个字符为null

// 检查调试输出标志是否启用
#ifdef DBG_OUTPUT_SERIAL
    serial_print(str_buf);
#elif defined(DBG_OUTPUT_TTY)
    serial_print("Please replace with tty device\r\n");
#else
    
#endif
}

void panic(const char *file, int line, const char *func, const char *cond)
{
    char str_buf[MAX_STR_BUF_SIZE];

    sprintf(str_buf, "[assert] in file:%s, func:%s, line:%d: %s", file, func, line, cond);
// 检查调试输出标志是否启用
#ifdef DBG_OUTPUT_SERIAL
    serial_print(str_buf);
#elif defined(DBG_OUTPUT_TTY)
    serial_print(str_buf);
    serial_print("Please replace with tty device\r\n");
#else
    
#endif
    for (;;)
    {
        hlt();
    }
}