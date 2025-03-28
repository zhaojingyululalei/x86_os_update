#include "dev/console.h"
#include "string.h"
#include "printk.h"
#define ESC_CMD2(Pn, cmd)		    "\x1b["#Pn#cmd
#define	ESC_COLOR_ERROR			    ESC_CMD2(31, m)	// 红色错误
#define	ESC_COLOR_DEFAULT		    ESC_CMD2(39, m)	// 默认颜色
#define ESC_CLEAR_SCREEN		    ESC_CMD2(2, J)	// 擦除整屏幕
#define	ESC_MOVE_CURSOR(row, col)  "\x1b["#row";"#col"H"





void console_test(void) {
    int idx = 0;  // 假设 idx 代表控制台设备的索引，可能是 0 或其他设备的标识
    
    // 1. 初始化控制台
    if (console_init(idx) != 0) {
        dbg_error("Console initialization failed!\n");
        return;
    }

    // 2. 写入一些文本，测试颜色和样式
    const char* buf = ESC_COLOR_ERROR "This is an error message in red.\r\n" ESC_COLOR_DEFAULT;
    int buf_len = strlen(buf);
    console_write(idx, buf, buf_len);  // 红色错误

    buf = "This is some normal text.\r\n";
    buf_len = strlen(buf);
    console_write(idx, buf, buf_len);  // 默认颜色文本

    // 3. 清屏操作
    buf = ESC_CLEAR_SCREEN;
    buf_len = strlen(buf);
    console_write(idx, buf, buf_len);  // 清除屏幕
    buf = "Screen cleared.\r\n";
    buf_len = strlen(buf);
    console_write(idx, buf, buf_len);

    // 4. 移动光标到指定位置（例如 5 行 10 列）
    buf = ESC_MOVE_CURSOR(5, 10);
    buf_len = strlen(buf);
    console_write(idx, buf, buf_len);  // 光标移动到(5,10)
    buf = "Moved cursor to (5, 10).\r\n";
    buf_len = strlen(buf);
    console_write(idx, buf, buf_len);

    // 5. 显示/隐藏光标（例如，设置为可见）
    console_set_cursor(idx, 1);  // 设置光标可见
    buf = "Cursor is now visible.\r\n";
    buf_len = strlen(buf);
    console_write(idx, buf, buf_len);

    // 6. 关闭控制台
    console_close(idx);
}