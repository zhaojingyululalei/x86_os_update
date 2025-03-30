#ifndef __TTY_H
#define __TTY_H


#include "ipc/semaphore.h"


#define TTY_NR						8		// 最大支持的tty设备数量
#define TTY_IBUF_SIZE				512		// tty输入缓存大小
#define TTY_OBUF_SIZE				512		// tty输出缓存大小
#define TTY_CMD_ECHO				0x1		// 开回显
#define TTY_CMD_IN_COUNT			0x2		// 获取输入缓冲区中已有的数据量

typedef struct _tty_fifo_t {
	char * buf;
	int size;				// 最大字节数
	int read, write;		// 当前读写位置
	int count;				// 当前已有的数据量
}tty_fifo_t;

int get_char_from_fifo(tty_fifo_t * fifo, char * character) ;
int put_char_to_fifo(tty_fifo_t * fifo, char c) ;

#define TTY_INLCR			(1 << 0)		// 将\n转成\r\n
#define TTY_IECHO			(1 << 2)		// 是否回显

#define TTY_OCRLF			(1 << 0)		// 输出是否将\n转换成\r\n

/**
 * tty设备
 */
typedef struct _tty_t {
	sem_t output_semaphore;
	char input_buffer[TTY_IBUF_SIZE];
	tty_fifo_t input_fifo;				// 输入处理后的队列
	sem_t input_semaphore;
	int open_cnt;							//打开次数
	int input_flags;						// 输入标志
    int output_flags;						// 输出标志
	int console_index;				// 控制台索引号
}tty_t;

void tty_select (int tty);
void tty_in (char ch);


#endif /* TTY_H */

