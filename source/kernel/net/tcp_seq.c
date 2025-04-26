#include "tcp.h"
#include "net_tools/net_mmpool.h"
#define TCP_SEQ_MAX_NR 1024
/**
 * tcp包有序性处理*/
static uint8_t tcp_seq_buf[(sizeof(tcp_seq_t) + sizeof(list_node_t)) * TCP_SEQ_MAX_NR];
static mempool_t tcp_seq_pool;
static list_t tcp_seq_list;

void tcp_seq_init(void)
{
    mempool_init(&tcp_seq_pool, tcp_seq_buf, TCP_SEQ_MAX_NR, sizeof(tcp_seq_t));
    list_init(&tcp_seq_list);
}
tcp_seq_t *tcp_seq_alloc(void)
{
    tcp_seq_t *seq;
    seq = mempool_alloc_blk(&tcp_seq_pool, -1);
    memset(seq, 0, sizeof(tcp_seq_t));
    return seq;
}
void tcp_seq_free(tcp_seq_t *seq)
{
    memset(seq, 0, sizeof(tcp_seq_t));
    mempool_free_blk(&tcp_seq_pool, seq);
}
/**通过检查包的连续性来更新rcv.next */
static uint32_t seq_updata_rcv_nxt(tcp_t *tcp)
{
    int continue_len = 0;
    // 链表中就一个包
    if (list_count(&tcp_seq_list) == 1)
    {
        tcp_seq_t *seq_first = list_node_parent(tcp_seq_list.first, tcp_seq_t, node);
        continue_len += seq_first->len;
    }
    else
    {
        list_node_t *cur_node = tcp_seq_list.first;
        list_node_t *nxt_node = cur_node->next;
        // 遍历链表，看看有多少字节是连续的
        while (nxt_node)
        {
            tcp_seq_t *cur = list_node_parent(cur_node, tcp_seq_t, node);
            tcp_seq_t *nxt = list_node_parent(nxt_node, tcp_seq_t, node);
            if (cur->seq_num + cur->len == nxt->seq_num)
            {
                // 这就是连续的
                continue_len += cur->len;
            }
            else
            {
                // 遍历到了不连续的包
                continue_len += cur->len;
                break;
            }
            cur_node = nxt_node;
            nxt_node = nxt_node->next;
        }
    }
    return tcp->rcv.win + continue_len;
}
/**找到第一个序号比该seq大的，插到前面 */
void tcp_seq_insert(tcp_seq_t *seq)
{
    if (list_is_empty(&tcp_seq_list))
    {
        list_insert_first(&tcp_seq_list, &seq->node);
    }
    else
    {
        tcp_seq_t *target = NULL;
        list_node_t *cur_node = tcp_seq_list.first;
        // 遍历链表，找到第一个比seq序号大的
        while (cur_node)
        {
            tcp_seq_t *cur = list_node_parent(cur_node, tcp_seq_t, node);
            if (cur->seq_num > seq->seq_num)
            {
                target = cur;
                break;
            }
            cur_node = cur_node->next;
        }
        if (!target)
        {
            // 没找到比他大的，插到最后
            list_insert_last(&tcp_seq_list, &seq->node);
        }
        else
        {
            if (seq_compare(seq->seq_num + seq->len, target->seq_num) > 0)
            {
                seq->len = (int)(target->seq_num - seq->seq_num);
            }
            // 插到target前面
            list_insert_front(&tcp_seq_list, &target->node, &seq->node);
        }
    }
}
/**移除连续的若干字节 */
int tcp_seq_remove(int size)
{
    int continue_len = 0;
    // 链表中就一个包
    if (list_count(&tcp_seq_list) == 1)
    {
        tcp_seq_t *seq_first = list_node_parent(tcp_seq_list.first, tcp_seq_t, node);
        if (size > seq_first->len)
        {
            dbg_error("size too large\r\n");
            return -1;
        }
        else if (size < seq_first->len)
        {
            seq_first->seq_num += size;
            seq_first->len -= size;
        }
        else
        {
            list_remove(&tcp_seq_list, &seq_first->node);
            tcp_seq_free(seq_first);
        }
    }
    else
    {
        list_node_t *cur_node = tcp_seq_list.first;
        list_node_t *nxt_node = cur_node->next;
        // 遍历链表，看看有多少字节是连续的
        while (nxt_node)
        {
            tcp_seq_t *cur = list_node_parent(cur_node, tcp_seq_t, node);
            tcp_seq_t *nxt = list_node_parent(nxt_node, tcp_seq_t, node);
            
            if (size >= cur->len)
            {
                size -= cur->len;
                list_remove(&tcp_seq_list, &cur->node);
                tcp_seq_free(cur);
            }
            else
            {
                size = 0;
                cur->len -= size;
                cur->seq_num += size;
            }
            if (cur->seq_num + cur->len == nxt->seq_num)
            {
                // 这就是连续的
            }
            else
            {
                // 遍历到了不连续的包
                
                break;
            }
            cur_node = nxt_node;
            nxt_node = nxt_node->next;
        }
        if (size != 0)
        {
            dbg_error("tcp rcv win fail\r\n");
            return -1;
        }
        else
        {
            return 0;
        }
    }
}
/**
 * 返回正数 a>b
 * 返回负数 a<b
 * 返回0   a=b
 */
int seq_compare(uint32_t a, uint32_t b)
{
    return (int)(a - b);
}
/**检查序号的正确性 */
int seq_check(tcp_t *tcp, tcp_parse_t *recv_parse, pkg_t *recv_pkg)
{
    int data_len = recv_pkg->total - recv_parse->head_len;
    if (data_len == 0)
    {
        if (tcp->rcv.win_size == 0)
        {
            return recv_parse->seq_num == tcp->rcv.next;
        }
        else
        {
            return (recv_parse->seq_num >= tcp->rcv.next && recv_parse->seq_num < tcp->rcv.win + tcp->rcv.win_size);
        }
    }
    else
    {
        if (tcp->rcv.win_size == 0)
        {
            return 0;
        }
        else
        {
            return (recv_parse->seq_num >= tcp->rcv.next && recv_parse->seq_num + data_len - 1 < tcp->rcv.win + tcp->rcv.win_size);
        }
    }
}

/**
 * 收到一个包，写入缓存
 * @param package:带着tcp头的数据包
 * 返回-1 丢弃 返回0成功写入缓存
 */
int seq_handle(tcp_t *tcp, tcp_parse_t *recv_parse, pkg_t *recv_pkg)
{
    int ret;
    int data_len = recv_pkg->total - recv_parse->head_len;

    if (!seq_check(tcp, recv_parse, recv_pkg))
    {
        return -1;
    }
    // 包正确，肯定要放进缓存
    int offset = recv_parse->seq_num - tcp->rcv.win;
    // 将数据写入rcv缓存
    uint8_t data[TCP_RCV_BUF_MAX_SIZE] = {0};
    package_read_pos(recv_pkg, data, data_len, recv_parse->head_len);
    ret = tcp_write_rcv_buf(tcp, offset, data, data_len);
    if (ret < 0)
    {
        // 数据没写进去，丢弃
        return -1;
    }

    // 构造tcp_seq_t并插入链表
    tcp_seq_t *seq = tcp_seq_alloc();
    seq->seq_num = recv_parse->seq_num;
    seq->len = data_len;
    tcp_seq_insert(seq);
    // 每插入一个包，都尝试更新一下rcv.nxt
    uint32_t rcv_nxt = seq_updata_rcv_nxt(tcp);
    // 更新失败，因为是不连续包
    if (rcv_nxt == tcp->rcv.next)
    {
        // 虽然收到了一个包，但是是不连续包，再次发送ack帧，请求连续的ack
        tcp_send_ack(tcp, recv_parse);
    }
    else
    {
        // 更新成功
        tcp->rcv.next = rcv_nxt;
        tcp_send_ack(tcp, recv_parse); // 发送ack帧确认
    }
}
