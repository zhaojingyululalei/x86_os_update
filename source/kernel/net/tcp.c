#include "tcp.h"
#include "tools/list.h"
#include "net_tools/net_mmpool.h"
#include "port.h"
#include "ipv4.h"
#include "algrithem.h"
#include "_time.h"
#include "rtc.h"
static uint8_t tcp_buf[TCP_BUF_MAX_NR * (sizeof(list_node_t) + sizeof(tcp_t))];
static mempool_t tcp_pool;
list_t tcp_list;

static uint8_t tcp_data_buf[TCP_BUF_MAX_NR * (sizeof(list_node_t) + sizeof(tcp_data_t))];
static mempool_t tcp_data_pool;


static int tcp_sendto(struct _sock_t *s, const void *buf, ssize_t len, int flags,
                      const struct sockaddr *dest, socklen_t dest_len)
{
}
static int tcp_recvfrom(struct _sock_t *s, void *buf, ssize_t len, int flags,
                        struct sockaddr *src, socklen_t *addr_len)
{
}
static int tcp_setsockopt(struct _sock_t *s, int level, int optname, const char *optval, int optlen)
{
    tcp_t* tcp = (tcp_t*)s;
    switch (level)
    {
    case SOL_SOCKET:

        if (optname == SO_RCVTIMEO)
        {
            struct timeval *time = (struct timeval *)optval;
            int tmo = time->tv_sec * 1000 + time->tv_usec / 1000;
            sock_wait_set(s, tmo, SOCK_RECV_WAIT);
        }
        if (optname == SO_SNDTIMEO)
        {
            struct timeval *time = (struct timeval *)optval;
            int tmo = time->tv_sec * 1000 + time->tv_usec / 1000;
            sock_wait_set(s, tmo, SOCK_SEND_WAIT);
        }
        if(optname == SO_KEEPALIVE){
            tcp->conn.k_enable = *(optval);
            tcp->conn.k_count = 0;
            tcp->conn.k_retry = TCP_KEEP_RETRY_DEFAULT;
            tcp->conn.k_idle  = TCP_KEEP_IDLE_DEFAULT;
            tcp->conn.k_intv = TCP_KEEP_INTV_DEFAULT;
            tcp_keepalive_start(tcp);
        }
        break;
    case SOL_TCP:
        if(optname == TCP_KEEPIDLE){
            tcp->conn.k_idle = *(optval);
            tcp_keepalive_restart(tcp);
        }
        if(optname == TCP_KEEPINTVL){
            tcp->conn.k_intv = *(optval);
            tcp_keepalive_restart(tcp);
        }
        if(optname ==TCP_KEEPCNT){
            tcp->conn.k_retry = *(optval);
            tcp_keepalive_restart(tcp);
        }
        break;
    default:
        dbg_error("unkown setsockopt optname\r\n");
        return -1;
    }
    return 0;
}
static int tcp_bind(struct _sock_t *s, const struct sockaddr *addr, socklen_t len)
{
    if(s->family!=addr->sa_family)
    {
        dbg_warning("famliy not match\r\n");
        return -1;
    }
    const struct sockaddr_in* host = (const struct sockaddr_in*)addr;
    s->host_ip.q_addr = host->sin_addr.s_addr;
    s->host_port = ntohs(host->sin_port);

    //检查端口是否被占用，如果没占用就开启端口
    if(!net_port_is_free(s->host_port))
    {
        dbg_warning("bind port wrong,port is used\r\n");
        return -1;
    }
    else
    {
        net_port_use(s->host_port);
    }

    return 0;
}

static uint32_t tcp_alloc_iss(void)
{
    uint32_t timeseed = get_time_seed();
    return random(timeseed);
}
/**初始化发送和接受缓冲区，以及tcp有序性处理的相关结构 */
int tcp_init_connect(tcp_t *tcp)
{

    tcp->snd.init_seq = tcp_alloc_iss();
    tcp->snd.next = tcp->snd.unack = tcp->snd.init_seq;
    //初始化接受和发送缓冲区
    tcp_buf_init(&tcp->snd.buf,tcp->snd.data_buf,TCP_SND_BUF_MAX_SIZE);
    tcp_buf_init(&tcp->rcv.buf,tcp->rcv.data_buf,TCP_RCV_BUF_MAX_SIZE);
    tcp->snd.win_size = TCP_SND_WIN_SIZE;
    tcp->rcv.win_size = TCP_RCV_WIN_SIZE;
    tcp_seq_init();
     //接受缓冲区特殊处理一下
    tcp_buf_expand(&tcp->rcv.buf,tcp->rcv.win_size);
    return 0;
}
static int tcp_connect(struct _sock_t *s, const struct sockaddr *addr, socklen_t len)
{

    int ret;
    tcp_t *tcp = (tcp_t *)s;
    if (tcp->state != TCP_STATE_CLOSED)
    {
        dbg_error("tcp state err\r\n");
        return -3;
    }

    //先查看路由，看看想要连接的目标能不能通
    ip_route_entry_t *entry = ip_route(&s->target_ip);
    if (!entry)
    {
        dbg_warning("ip route err\r\n");
        return -6;
    }
    // 之前没绑定ip
    if (s->host_ip.q_addr == 0)
    {
        s->host_ip.q_addr = entry->netif->info.ipaddr.q_addr;
    }
    else
    {
        // 如果之前绑定了
        if (entry->netif->info.ipaddr.q_addr != s->host_ip.q_addr)
        {
            dbg_warning("the ip which bind can not ip route\r\n");
            return -4;
        }
    }
    const struct sockaddr_in *sockaddr = (const struct sockaddr_in *)addr;
    s->target_ip.q_addr = sockaddr->sin_addr.s_addr;
    s->target_port = ntohs(sockaddr->sin_port);

    // 前面可能没有绑定，因此动态分配端口
    if (s->host_port != 0)
    {
        if (net_port_is_free(s->host_port))
        {
            // 如果绑定的端口并没有开启
            dbg_warning("connect port err\r\n");
            return -5;
        }
    }
    else
    {
        s->host_port = net_port_dynamic_alloc();
    }

    // 检测tcp链接的唯一性(唯一的4元组)
    if (find_tcp_connection(tcp->base.host_ip.q_addr, tcp->base.host_port, tcp->base.target_ip.q_addr, tcp->base.target_port))
    {
        dbg_warning("tcp is already exisit\r\n");
        return -7;
    }
    // 初始化各个标志位,现在先别加hash表，都不知道是否成功连接
    // hash表里存的都是成功的链接
    ret = tcp_init_connect(tcp);
    if (ret < 0)
    {
        dbg_warning("connect fail\r\n");
        return -2;
    }

    // 发送syn数据帧
    ret = tcp_send_syn(tcp);
    if (ret < 0)
    {
        dbg_warning("send syn fail\r\n");
        return -3;
    }
    // 修改状态
    tcp_set_state(tcp, TCP_STATE_SYN_SEND);
}
static int tcp_send(struct _sock_t *s, const void *buf, ssize_t len, int flags)
{
    int ret;
    tcp_t* tcp = (tcp_t*)s;
    //状态判断
    if(tcp->state!=TCP_STATE_ESTABLISHED && tcp->state != TCP_STATE_CLOSE_WAIT){
        dbg_warning("tcp state wrong,can not snd data\r\n");
        return -1;
    }
    int write_cnt = tcp_write_snd_buf(tcp,(uint8_t*)buf,len);
    if(write_cnt<0){
        return -2;
    }else if(write_cnt == 0){
        return NET_ERR_NEED_WAIT;
    }else{
        ret = tcp_send_data(tcp,NULL);
        if(ret < 0){
            return -3;
        }
    }
    return write_cnt;
}
static int tcp_recv(struct _sock_t *s, void *buf, ssize_t len, int flags)
{
    tcp_t* tcp = (tcp_t*)s;
    if(tcp->state!= TCP_STATE_ESTABLISHED && tcp->state != TCP_STATE_CLOSE_WAIT){
        dbg_warning("tcp is closed,can not recv data\r\n");
        return -1;
    }
    //rcv.nxt和rcv.win之间的数据全是已接收，但是用户未读取数据
    int rcv_cnt = (int)(tcp->rcv.next - tcp->rcv.win);
    if(rcv_cnt == 0){
        return 0;
    }
    int cpy_cnt = rcv_cnt < len?rcv_cnt:len;

    tcp_buf_read(&tcp->rcv.buf,buf,0,cpy_cnt);
    //移动滑动窗口
    tcp_buf_remove(&tcp->rcv.buf,cpy_cnt);
    tcp_buf_expand(&tcp->rcv.buf,cpy_cnt);
    tcp->rcv.win += cpy_cnt;
    tcp_seq_remove(cpy_cnt);
    return cpy_cnt;

}
static int tcp_listen(struct _sock_t *s, int backlog)
{
    int ret;
    tcp_t* tcp = (tcp_t*)s;
    tcp->conn.backlog = backlog;
    //初始化全连接和半链接队列
    ret = tcp_connQ_init(tcp);
    if(ret < 0){
        dbg_error("listen queue init fail\r\n");
        return -1;
    }

    //初始化发送和接受缓冲区
    tcp_init_connect(tcp);



    tcp_set_state(tcp,TCP_STATE_LISTEN);
    return 0;
}
static int tcp_accept(struct _sock_t *s, struct sockaddr *addr, socklen_t *len, struct _sock_t **client)
{
    tcp_t* tcp = (tcp_t*)s;
    tcp_t* client_tcp = tcp_connQ_dequeue(tcp);
    if(!client_tcp){
        dbg_error("accpt fail\r\n");
        return -1;
    }
    struct sockaddr_in* xaddr = (struct sockaddr_in*)addr;
    xaddr->sin_port = client_tcp->base.target_port;
    xaddr->sin_addr.s_addr = client_tcp->base.target_ip.q_addr;
    len = sizeof(struct sockaddr);
    return socket_get_index(tcp);
}
static void tcp_destroy(struct _sock_t *s)
{
}
static int tcp_close(struct _sock_t* s){
    tcp_t* tcp = (tcp_t*)s;
    switch (tcp->state)
    {
    case TCP_STATE_CLOSED:
        tcp_free(tcp);
        break;
    //如果处于正在连接 状态
    case TCP_STATE_SYN_SEND:
    case TCP_STATE_SYN_RECVD:
        tcp_reset(tcp,NET_ERR_CLOSE);
        tcp_free(tcp);
        tcp_set_state(tcp,TCP_STATE_CLOSED);
        break;
    case TCP_STATE_ESTABLISHED:
        tcp_send_fin(tcp);
        tcp_set_state(tcp,TCP_STATE_FIN_WAIT1);
        return NET_ERR_NEED_WAIT;//调用完close的线程得等着ack
    case TCP_STATE_CLOSE_WAIT:
        tcp_send_fin(tcp);
        tcp_set_state(tcp,TCP_STATE_LAST_ACK);
        return NET_ERR_NEED_WAIT;//调用完close的线程得等着ack
    default:
        TCP_DBG_PRINT("not handle\r\n");
        return -1;
    
    }
    return 0;
}
static const sock_ops_t tcp_ops = {
    .setsockopt = tcp_setsockopt,
    .sendto = tcp_sendto,
    .recvfrom = tcp_recvfrom,
    .connect = tcp_connect,
    .send = tcp_send,
    .recv = tcp_recv,
    .bind = tcp_bind,
    .close = tcp_close,
};

static tcp_t *tcp_alloc(int tmo)
{
    tcp_t *ret = NULL;
    ret = mempool_alloc_blk(&tcp_pool, tmo);
    if (ret)
    {
        memset(ret, 0, sizeof(tcp_t));
    }
    return ret;
}
void tcp_free(tcp_t *tcp)
{
    sock_wait_destory(&tcp->base.conn_wait);
    sock_wait_destory(&tcp->base.recv_wait);
    sock_wait_destory(&tcp->base.send_wait);
    sock_wait_destory(&tcp->close_wait);
    list_remove(&tcp_list,&tcp->node);
    hash_delete_tcp_connection(tcp);
    if(tcp->state == TCP_STATE_LISTEN){
        msgQ_destory(&tcp->conn.cmplt_conn_q);
    }
    memset(tcp,0,sizeof(tcp_t));
    mempool_free_blk(&tcp_pool, tcp);
}
/**
 * 因为啥要复位tcp协议
 */
void tcp_reset(tcp_t *tcp, net_err_t err)
{
    tcp->state = TCP_STATE_CLOSED;
    sock_wait_notify(&tcp->base.conn_wait, err);
    sock_wait_notify(&tcp->base.recv_wait, err);
    sock_wait_notify(&tcp->base.send_wait, err);
    sock_wait_notify(&tcp->close_wait,err);
    //tcp_free(tcp);去唤醒的地方 释放tcp
}
tcp_data_t *tcp_data_alloc()
{
    tcp_data_t *ret = NULL;
    ret = mempool_alloc_blk(&tcp_data_pool, -1);
    if (ret)
    {
        memset(ret, 0, sizeof(tcp_data_t));
    }
    return ret;
}
void tcp_data_free(tcp_data_t *tcp_data)
{
    mempool_free_blk(&tcp_data_pool, tcp_data);
}

sock_t *tcp_create(int family, int protocol)
{
    int ret;
    tcp_t *tcp = tcp_alloc(0);
    if (!tcp)
    {
        dbg_error(" tcp pool out of memory\r\n");
        return NULL;
    }
    int proto = protocol == 0 ? TCP_DEFAULT_PROTOCAL : protocol;
    sock_init((sock_t *)tcp, family, proto, &tcp_ops);

    
    sock_wait_init(&tcp->close_wait);

    list_insert_last(&tcp_list, &tcp->node);

    return (sock_t *)tcp;
}
void tcp_init(void)
{
    list_init(&tcp_list);
    mempool_init(&tcp_pool, tcp_buf, TCP_BUF_MAX_NR, sizeof(tcp_t));
    mempool_init(&tcp_data_pool, tcp_data_buf, TCP_BUF_MAX_NR, sizeof(tcp_data_t));
}

void tcp_parse(pkg_t *package, tcp_parse_t *parse)
{
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    parse->src_port = ntohs(head->src_port);
    parse->dest_port = ntohs(head->dest_port);
    parse->seq_num = ntohl(head->seq);
    parse->ack_num = ntohl(head->ack);
    uint16_t hlen_and_flags = ntohs(head->hlen_and_flags);
    parse->head_len = (hlen_and_flags >> 12) * 4;
    parse->urg = hlen_and_flags & TCP_URG_POS;
    parse->ack = hlen_and_flags & TCP_ACK_POS;
    parse->psh = hlen_and_flags & TCP_PSH_POS;
    parse->rst = hlen_and_flags & TCP_RST_POS;
    parse->syn = hlen_and_flags & TCP_SYN_POS;
    parse->fin = hlen_and_flags & TCP_FIN_POS;

    parse->win_size = ntohs(head->win_size);
    parse->checksum = ntohs(head->checksum);
    parse->urgptr = ntohs(head->urg_ptr);

    if (parse->head_len > sizeof(tcp_head_t))
    {
        parse->options = (uint8_t *)head + sizeof(tcp_head_t);
        parse->option_len = parse->head_len - sizeof(tcp_head_t);
    }
}
void tcp_set(pkg_t *package, const tcp_parse_t *parse)
{
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t) + parse->option_len, 0);
    head->src_port = htons(parse->src_port);
    head->dest_port = htons(parse->dest_port);
    head->seq = htonl(parse->seq_num);
    head->ack = htonl(parse->ack_num);

    uint16_t hlen_and_flags = 0;
    hlen_and_flags |= ((parse->head_len / 4) << 12);
    // 设置 TCP 标志位
    if (parse->urg)
        hlen_and_flags |= (1 << 5); // 设置 URG 标志位
    if (parse->ack)
        hlen_and_flags |= (1 << 4); // 设置 ACK 标志位
    if (parse->psh)
        hlen_and_flags |= (1 << 3); // 设置 PSH 标志位
    if (parse->rst)
        hlen_and_flags |= (1 << 2); // 设置 RST 标志位
    if (parse->syn)
        hlen_and_flags |= (1 << 1); // 设置 SYN 标志位
    if (parse->fin)
        hlen_and_flags |= (1 << 0); // 设置 FIN 标志位
    head->hlen_and_flags = htons(hlen_and_flags);

    // 设置窗口大小
    head->win_size = htons(parse->win_size); // 窗口大小

    // 设置校验和和紧急指针
    head->checksum = 0;                   // 校验和
    head->urg_ptr = htons(parse->urgptr); // 紧急指针

    // 如果有 TCP 选项，填充选项字段
    if (parse->options && parse->option_len > 0)
    {
        uint8_t *option_ptr = (uint8_t *)(head + 1);           // 跳过头部字段
        memcpy(option_ptr, parse->options, parse->option_len); // 拷贝选项数据
    }
}
// TCP 报文信息打印函数
void tcp_show(const tcp_parse_t *tcp)
{
#ifdef TCP_DBG
    dbg_info("TCP Header Information:\r\n");
    dbg_info("-----------------------\r\n");
    dbg_info("Source Port: %d\r\n", tcp->src_port);
    dbg_info("Destination Port: %d\r\n", tcp->dest_port);
    dbg_info("Sequence Number: %d\r\n", tcp->seq_num);
    dbg_info("Acknowledgment Number: %d\r\n", tcp->ack_num);
    dbg_info("Header Length: %d bytes\r\n", tcp->head_len);
    dbg_info("Flags:\r\n");
    dbg_info("  URG: %s\r\n", tcp->urg ? "Yes" : "No");
    dbg_info("  ACK: %s\r\n", tcp->ack ? "Yes" : "No");
    dbg_info("  PSH: %s\r\n", tcp->psh ? "Yes" : "No");
    dbg_info("  RST: %s\r\n", tcp->rst ? "Yes" : "No");
    dbg_info("  SYN: %s\r\n", tcp->syn ? "Yes" : "No");
    dbg_info("  FIN: %s\r\n", tcp->fin ? "Yes" : "No");
    dbg_info("Window Size: %d\r\n", tcp->win_size);
    dbg_info("Checksum: 0x%04x\r\n", tcp->checksum);
    dbg_info("Urgent Pointer: %d\r\n", tcp->urgptr);

    // 打印选项（如果有）
    if (tcp->options && tcp->option_len > 0)
    {
        dbg_info("Options (%d bytes):\r\n", tcp->option_len);
        for (uint16_t i = 0; i < tcp->option_len; i++)
        {
            dbg_info("  0x%02x ", tcp->options[i]);
            if ((i + 1) % 8 == 0)
                dbg_info("\r\n"); // 每 8 个字节换行
        }
        dbg_info("\r\n");
    }
    else
    {
        dbg_info("Options: None\r\n");
    }
    dbg_info("-----------------------\r\n");
#endif
}

int tcp_read_option(tcp_t* tcp,tcp_parse_t* recv_parse){
    if(recv_parse->option_len <=0){
        return -1;
    }
    uint8_t* options = recv_parse->options;
    switch (options[0])
    {
    case TCP_OPTION_MSS:
        if(options[1]!=recv_parse->option_len){
            return -2;
        }
        uint16_t mss = *((uint16_t*)options +1);
        mss= ntohs(mss);
        ip_route_entry_t* entry = ip_route(&tcp->base.target_ip);
        if(!entry) return -3;
        if(entry->gateway.q_addr !=0){
            //如果存在网关，那么
            tcp->mss = TCP_DEFAULT_MSS;
        }else{
            tcp->mss = mss;
        }

        break;
    case TCP_OPTION_END:
    case TCP_OPTION_NOP:
    default:
        dbg_warning("unhandle \r\n");
        break;
    }
    return 0;
}