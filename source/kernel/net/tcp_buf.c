#include "tcp.h"

// 检查队列是否满
int tcp_buf_is_full(tcp_buf_t *buf)
{
    return buf->size == buf->capacity;
}

// 检查队列是否空
int tcp_buf_is_empty(tcp_buf_t *buf)
{
    return buf->size == 0;
}

// 插入数据
int tcp_buf_insert(tcp_buf_t *buf, uint8_t *data, int data_size)
{
    if (tcp_buf_is_full(buf))
    {
        return -1; // 队列已满，无法插入
    }

    // 插入数据
    for (int i = 0; i < data_size; i++)
    {
        buf->data[buf->tail] = data[i];
        buf->tail = (buf->tail + 1) % buf->capacity;
    }

    buf->size += data_size;
    return 0; // 插入成功
}

// 取出数据
int tcp_buf_remove(tcp_buf_t *buf, int max_size)
{
    if (tcp_buf_is_empty(buf)|| buf->size < max_size)
    {
        dbg_warning("max_size too large\r\n");
        return -1; // 队列为空，无法取出
    }
    int count = (buf->size < max_size) ? buf->size : max_size;
    buf->head = (buf->head + count) % buf->capacity;

    buf->size -= count;
    return count; // 返回取出的数据量
}
/**读数据 */
int tcp_buf_read(tcp_buf_t *buf, uint8_t *out_data, int offset, int len)
{
    int start = (buf->head + offset) % buf->capacity;
    int count = (buf->size - offset) > len ? len : (buf->size - offset);
    for (int i = 0; i < count; i++)
    {
        out_data[i] = buf->data[start];
        start = (start + 1) % buf->capacity;
    }
    return count;
}
/**写数据 */
int tcp_buf_write(tcp_buf_t *buf, int offset, int data_len, uint8_t *data)
{
    int start = (buf->head + offset) % buf->capacity;
    for (int i = 0; i < data_len; i++)
    {
        buf->data[start] = data[i];
        start = (start + 1) % buf->capacity;
    }
    return data_len;
}
int tcp_buf_expand(tcp_buf_t *buf, int len)
{
    int remain_len = buf->capacity - buf->size;
    if (len <= remain_len)
    {
        buf->tail = (buf->tail + len) % buf->capacity;
        buf->size += len;
        return len;
    }
    else
    {
        return -1;
    }
}
// 打印队列状态
void tcp_buf_print(tcp_buf_t *buf)
{
    dbg_info("Queue size: %d\n", buf->size);
    dbg_info("Head: %d, Tail: %d\n", buf->head, buf->tail);
    dbg_info("Data: ");
    for (int i = 0; i < buf->size; i++)
    {
        dbg_info("%02X ", buf->data[(buf->head + i) % buf->capacity]);
    }
    dbg_info("\n");
}

// 释放队列内存
void tcp_buf_free(tcp_buf_t *buf)
{
    memset(buf, 0, sizeof(tcp_buf_t));
}

void tcp_buf_init(tcp_buf_t *buf, uint8_t *data, int capacity)
{
    buf->data = data;
    buf->capacity = capacity;
    buf->size = 0;
    buf->head = 0;
    buf->tail = 0;
}
/**snd buf */
int tcp_write_snd_buf(tcp_t *tcp, const uint8_t *data, int len)
{
    tcp_buf_t *buf = &tcp->snd.buf;
    if (tcp_buf_is_full(buf))
    {
        dbg_warning("tcp snd buf is full\r\n");
        return 0;
    }
    int free_cnt = buf->capacity - buf->size;
    int write_cnt = free_cnt > len ? len : free_cnt;

    tcp_buf_insert(buf, data, write_cnt);

    return write_cnt;
}
/*len:要取出多少字节*/
int tcp_read_snd_buf(tcp_t *tcp, pkg_t *tcp_pkg, int pkg_offset, int buf_offset, int len)
{
    int ret;
    uint8_t tmp[TCP_SND_BUF_MAX_SIZE] = {0};
    tcp_buf_t *buf = &tcp->snd.buf;
    // 先把要发送的数据读出来
    ret = tcp_buf_read(buf, tmp, buf_offset, len);
    if (ret != len)
    {
        return -1;
    }
    // 然后写到包里
    ret = package_write_pos(tcp_pkg, tmp, len, pkg_offset);
    if (ret < 0)
    {
        return ret;
    }

    return len;
}

/*rcv buf*/
/**向buf里存放数据 */
int tcp_write_rcv_buf(tcp_t *tcp, int offset, const uint8_t *data, int data_len)
{
    tcp_buf_t *buf = &tcp->rcv.buf;
    uint32_t limit = (buf->head + offset + data_len) % buf->capacity;
    if (seq_compare(limit, buf->tail) <= 0)
    {
        return tcp_buf_write(buf, offset, data_len, data);
    }
    else
    {
        // 出界了,写不了
        return -1;
    }
}
