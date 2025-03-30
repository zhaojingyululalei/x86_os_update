

#include "dev/console.h"
#include "dev/tty.h"
#include "printk.h"
#include "irq/irq.h"
#include "mem/kmalloc.h"
#include "dev/kbd.h"
#include "dev/dev.h"
#include "string.h"
static tty_t tty_devices[TTY_NR];
static int active_tty = 0;

/**
 * @brief 初始化FIFO缓冲区
 */
void init_fifo(tty_fifo_t *fifo, char *buffer, int buffer_size)
{
    fifo->buf = buffer;
    fifo->count = 0;
    fifo->size = buffer_size;
    fifo->read = fifo->write = 0;
}

/**
 * @brief 从FIFO缓冲区读取一个字节
 */
int get_char_from_fifo(tty_fifo_t *fifo, char *character)
{
    if (fifo->count <= 0)
    {
        return -1; // 没有数据可读
    }

    irq_state_t irq_state = irq_enter_protection();
    *character = fifo->buf[fifo->read++];
    if (fifo->read >= fifo->size)
    {
        fifo->read = 0;
    }
    fifo->count--;
    irq_leave_protection(irq_state);
    return 0; // 成功读取
}

/**
 * @brief 向FIFO缓冲区写入一个字节
 */
int put_char_to_fifo(tty_fifo_t *fifo, char c)
{
    if (fifo->count >= fifo->size)
    {
        return -1; // FIFO已满
    }

    irq_state_t state = irq_enter_protection();
    fifo->buf[fifo->write++] = c;
    if (fifo->write >= fifo->size)
    {
        fifo->write = 0;
    }
    fifo->count++;
    irq_leave_protection(state);

    return 0; // 成功写入
}

/**
 * @brief 判断tty设备是否有效
 */
static inline tty_t *get_tty(int idx)
{
    int tty = idx;
    if ((tty < 0) || (tty >= TTY_NR))
    {
        dbg_info("Failed to open tty device. Invalid tty id = %d", idx);
        return NULL;
    }
    tty_t *target = tty_devices + tty;
    if (target->open_cnt <= 0)
    {
        dbg_info("tty is not opened. tty = %d", tty);
        return NULL;
    }
    return target;
}

/**
 * @brief 打开tty设备
 */
int open_tty_device(int idx)
{
    int tty_id = idx;
    if ((tty_id < 0) || (tty_id >= TTY_NR))
    {
        dbg_info("Failed to open tty device. Invalid tty id = %d", tty_id);
        return -1;
    }

    tty_t *tty = tty_devices + tty_id;

    if (tty->open_cnt > 0)
    {
        tty->open_cnt++;
        return 0;
    }
    init_fifo(&tty->input_fifo, tty->input_buffer, TTY_IBUF_SIZE);
    sys_sem_init(&tty->input_semaphore, 0);

    tty->input_flags = TTY_INLCR | TTY_IECHO;
    tty->output_flags = TTY_OCRLF;
    tty->open_cnt = 1;
    tty->console_index = tty_id;

    console_init(tty_id);
    return 0;
}

/**
 * @brief 向tty设备写入数据
 */
int write_to_tty(int idx, int addr, char *buffer, int length)
{
    if (length < 0)
    {
        return -1;
    }

    tty_t *tty = get_tty(idx);
    if (tty->output_flags & TTY_OCRLF)
    {
        // 创建一个新的缓冲区，大小是原来的一倍（最多插入\r，最坏的情况下每个\n都会变成\r\n）
        char *new_buffer = kmalloc(length * 2);
        if (new_buffer == NULL)
        {
            return -1; // 内存分配失败
        }

        int i = 0, j = 0;
        while (i < length)
        {
            if (buffer[i] == '\n')
            {
                // 替换 \n 为 \r\n
                new_buffer[j++] = '\r';
                new_buffer[j++] = '\n';
            }
            else
            {
                // 保持原有字符
                new_buffer[j++] = buffer[i];
            }
            i++;
        }

        // 传递新的缓冲区到 console_write
        console_write(idx, new_buffer, j);

        // 释放临时缓冲区
        kfree(new_buffer);

        return j; // 返回写入的字符数
    }
    else
    {
        // 如果不需要转换，只是直接输出
        console_write(idx, buffer, length);
        return length;
    }
}

/**
 * @brief 从tty设备读取数据
 */
int read_from_tty(int idx, int addr, char *buffer, int size)
{
    if (size < 0)
    {
        return -1;
    }

    tty_t *tty = get_tty(idx);
    char *pbuf = buffer;
    int len = 0;

    while (len < size)
    {
        sys_sem_wait(&tty->input_semaphore);
        char character;
        get_char_from_fifo(&tty->input_fifo, &character);

        switch (character)
        {
        case ASCII_DEL:
            if (len == 0)
            {
                continue;
            }
            len--;
            pbuf--;
            break;
        case '\n':
            if ((tty->input_flags & TTY_INLCR) && (len < size - 1))
            { // \n变成\r\n
                *pbuf++ = '\r';
                len++;
            }
            *pbuf++ = '\n';
            len++;
            break;
        default:
            *pbuf++ = character;
            len++;
            break;
        }

        // 回显字符，如果启用了回显标志
        if (tty->input_flags & TTY_IECHO)
        {
            write_to_tty(idx, 0, &character, 1);
        }

        // 遇到一行结束也直接跳出
        if ((character == '\r') || (character == '\n'))
        {
            break;
        }
    }

    return len;
}

/**
 * @brief 向tty设备发送命令
 */
int control_tty(int idx, int cmd, int arg0, int arg1)
{
    tty_t *tty = get_tty(idx);

    switch (cmd)
    {
    case TTY_CMD_ECHO:
        if (arg0)
        {
            tty->input_flags |= TTY_IECHO;
            console_set_cursor(tty->console_index, 1);
        }
        else
        {
            tty->input_flags &= ~TTY_IECHO;
            console_set_cursor(tty->console_index, 0);
        }
        break;
    case TTY_CMD_IN_COUNT:
        if (arg0)
        {
            *(int *)arg0 = sys_sem_count(&tty->input_semaphore);
        }
        break;
    default:
        break;
    }
    return 0;
}

/**
 * @brief 关闭tty设备
 */
void close_tty(int idx)
{
}

/**
 * @brief 输入tty字符
 */
void tty_in(char ch)
{
    tty_t *tty = tty_devices + active_tty;

    // 辅助队列要有空闲空间可代写入
    if (sys_sem_count(&tty->input_semaphore) >= TTY_IBUF_SIZE)
    {
        return;
    }

    // 写入辅助队列，通知数据到达
    put_char_to_fifo(&tty->input_fifo, ch);
    sys_sem_notify(&tty->input_semaphore);
}

/**
 * @brief 选择tty
 */
void tty_select(int tty)
{
    if (tty != active_tty)
    {
        console_select(tty);
        active_tty = tty;
    }
}

int tty_open(int devfd, int flag)
{
    device_t *dev = dev_get(devfd);
    int major = dev->desc.major;
    int minor = dev->desc.minor;
    if (major != DEV_MAJOR_TTY)
    {
        dbg_error("mpjor=%d,when open tty device\r\n", major);
        return -1;
    }
    int idx = minor;
    return open_tty_device(idx);
}
int tty_read(int devfd, int addr, char *buf, int size)
{
    device_t *device = dev_get(devfd);
    if (device->desc.major != DEV_MAJOR_TTY)
    {
        dbg_error("mpjor=%d,when read tty device\r\n", device->desc.major);
        return -1;
    }
    int idx = device->desc.minor;
    return read_from_tty(idx, addr, buf, size);
}
int tty_write(int devfd, int addr, char *buf, int size)
{
    device_t *device = dev_get(devfd);
    if (device->desc.major != DEV_MAJOR_TTY)
    {
        dbg_error("mpjor=%d,when write tty device\r\n", device->desc.major);
        return -1;
    }
    int idx = device->desc.minor;
    return write_to_tty(idx, addr, buf, size);
}
int tty_control(int devfd, int cmd, int arg0, int arg1)
{
    device_t *device = dev_get(devfd);
    if (device->desc.major != DEV_MAJOR_TTY)
    {
        dbg_error("mpjor=%d,when control tty device\r\n", device->desc.major);
        return -1;
    }
    int idx = device->desc.minor;
    return control_tty(idx, cmd, arg0, arg1);
}
void tty_close(int devfd)
{
    device_t *device = dev_get(devfd);
    if (device->desc.major != DEV_MAJOR_TTY)
    {
        dbg_error("mpjor=%d,when close tty device\r\n", device->desc.major);
        return -1;
    }
    int idx = device->desc.minor;
    return close_tty(idx);
}
// 设备描述表: 描述一个设备所具备的特性
dev_ops_t dev_tty_opts = {

    .open = tty_open,
    .read = tty_read,
    .write = tty_write,
    .control = tty_control,
    .close = tty_close,
};

void tty_init(void)
{
    for (int i = 0; i < TTY_NR; i++)
    {
        char name_buf[5]={0};
        sprintf(name_buf,"tty%d",i);
        dev_install(DEV_TYPE_CHAR,DEV_MAJOR_TTY,i,name_buf,NULL,&dev_tty_opts);
        memset(name_buf,0,5);
    }
}