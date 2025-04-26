#include "tcp.h"
#include "ipv4.h"

static int tcp_get_winsize(tcp_t* tcp){
    return tcp->rcv.win_size-(int)(tcp->rcv.next - tcp->rcv.win);
}
int tcp_send_out(pkg_t *package, ipaddr_t *src, ipaddr_t *dest)
{
    int ret;
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    head->checksum = package_checksum_peso(package, src, dest, IPPROTO_TCP);

    ret = ipv4_out(package, PROTOCAL_TYPE_TCP, dest);
    if (ret < 0)
    {
        package_collect(package);
        return -1;
    }
    return 0;
}
/**
 * @param tcp_recv:接收到的tcp数据包
 */
int tcp_send_reset(tcp_parse_t *tcp_recv, ipaddr_t *src, ipaddr_t *dest)
{
    int ret;
    pkg_t *package = package_alloc(sizeof(tcp_head_t));
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp_recv->dest_port;
    parse.dest_port = tcp_recv->src_port;
    parse.ack_num = tcp_recv->seq_num + 1;
    // seq_num没有，因为没收到ack_num
    parse.head_len = sizeof(tcp_head_t); // 没选项字段
    parse.rst = true;
    parse.ack = true;

    // 设置包头
    tcp_set(package, &parse);
    return tcp_send_out(package, src, dest);
}

int tcp_send_syn(tcp_t *tcp)
{
    // 无选项字段
    pkg_t *package = package_alloc(sizeof(tcp_head_t));
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp->base.host_port;
    parse.dest_port = tcp->base.target_port;
    parse.seq_num = tcp->snd.init_seq;
    parse.ack_num = tcp->rcv.init_seq;
    parse.syn = true;
    parse.win_size = tcp_get_winsize(tcp);
    parse.head_len = sizeof(tcp_head_t);
    tcp_set(package, &parse);
    tcp->snd.next += parse.syn;
    return tcp_send_out(package, &tcp->base.host_ip, &tcp->base.target_ip);
}

int tcp_send_ack(tcp_t *tcp, tcp_parse_t *tcp_recv)
{
    int ret;
    pkg_t *package = package_alloc(sizeof(tcp_head_t));
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp->base.host_port;
    parse.dest_port = tcp->base.target_port;
    parse.seq_num = tcp->snd.next;
    parse.ack_num = tcp->rcv.next;
    parse.ack = true;
    parse.win_size = tcp_get_winsize(tcp);
    parse.head_len = sizeof(tcp_head_t);
    tcp_set(package, &parse);
    return tcp_send_out(package, &tcp->base.host_ip, &tcp->base.target_ip);
}

int tcp_send_syn_ack(tcp_t *tcp, tcp_parse_t *tcp_recv)
{
    // 无选项字段
    pkg_t *package = package_alloc(sizeof(tcp_head_t));
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp->base.host_port;
    parse.dest_port = tcp->base.target_port;
    parse.seq_num = tcp->snd.init_seq;
    parse.ack_num = tcp->rcv.next;
    parse.syn = true;
    parse.ack = true;
    parse.win_size = tcp_get_winsize(tcp);
    parse.head_len = sizeof(tcp_head_t);
    tcp_set(package, &parse);
    tcp->snd.next++;
    return tcp_send_out(package, &tcp->base.host_ip, &tcp->base.target_ip);
}
int tcp_send_fin(tcp_t *tcp)
{
    pkg_t *package = package_alloc(sizeof(tcp_head_t));
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp->base.host_port;
    parse.dest_port = tcp->base.target_port;
    parse.seq_num = tcp->snd.next;
    parse.ack_num = tcp->rcv.next;
    parse.fin = true;
    parse.ack = true;
    parse.win_size = tcp_get_winsize(tcp);
    parse.head_len = sizeof(tcp_head_t);
    tcp_set(package, &parse);
    tcp->snd.next++;

    return tcp_send_out(package, &tcp->base.host_ip, &tcp->base.target_ip);
}

int tcp_send_data(tcp_t *tcp, tcp_flag_t *flag)
{
    int ret;
    // 缓存里有多少数据发多少
    int offset = (int)(tcp->snd.next - tcp->snd.unack);
    int send_cnt = tcp->snd.buf.size - offset;
    if(send_cnt < 0){
        return -1;
    }else if(send_cnt ==0){
        return 0;
    }
    send_cnt = (send_cnt < tcp->mss) ? send_cnt:tcp->mss;
    pkg_t *package = package_alloc(sizeof(tcp_head_t) + send_cnt);
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp->base.host_port;
    parse.dest_port = tcp->base.target_port;
    parse.seq_num = tcp->snd.next;
    parse.ack_num = tcp->rcv.next;
    parse.ack = true;
    parse.win_size = tcp_get_winsize(tcp);
    parse.head_len = sizeof(tcp_head_t);
    if (flag)
    {
        parse.urg = flag->urg;
        parse.psh = flag->psh;
        parse.rst = flag->rst;
        parse.syn = flag->syn;
        parse.fin = flag->fin;
    }

    tcp_set(package, &parse);

    tcp_read_snd_buf(tcp, package, sizeof(tcp_head_t), offset,send_cnt);

    ret = tcp_send_out(package, &tcp->base.host_ip, &tcp->base.target_ip);
    if(ret < 0){
        package_collect(package);
        return ret;
    }
    tcp->snd.next += (send_cnt + parse.fin);
    return 0;
}

int tcp_send_keepalive(tcp_t* tcp){
    int ret;
    pkg_t *package = package_alloc(sizeof(tcp_head_t));
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp->base.host_port;
    parse.dest_port = tcp->base.target_port;
    parse.seq_num = tcp->snd.next-1;
    parse.ack_num = tcp->rcv.next;
    parse.win_size = tcp_get_winsize(tcp);
    // seq_num没有，因为没收到ack_num
    parse.head_len = sizeof(tcp_head_t); // 没选项字段
    parse.ack = true;

    // 设置包头
    tcp_set(package, &parse);
    return tcp_send_out(package, &tcp->base.host_ip, &tcp->base.target_ip);
}