#include "raw.h"
#include "net_tools/net_mmpool.h"
#include "sock.h"
#include "printk.h"
#include "ipv4.h"
static mempool_t raw_pool;
static uint8_t raw_buf[RAW_MAX_NR * (sizeof(raw_t) + sizeof(list_node_t))];
static list_t raw_list;

static int raw_sendto(struct _sock_t *s, const void *buf, ssize_t len, int flags,
                      const struct sockaddr *dest, socklen_t dest_len)
{
    int ret;
    pkg_t *package = package_create(buf, len);
    struct sockaddr_in *addr = (struct sockaddr_in *)dest;
    if (addr->sin_family != s->family)
    {
        dbg_error("sockaddr set wrong :in filed sin_family\r\n");
        return -1;
    }
    if (s->target_ip.q_addr == 0)
    {
        s->target_ip.q_addr = addr->sin_addr.s_addr;
    }
    else
    {
        if(s->target_ip.q_addr != addr->sin_addr.s_addr)
        {
            dbg_error("raw socket and dest ip not match\r\n");
            return -4;
        }
    }
    ret = ipv4_out(package, s->protocal, &addr->sin_addr);
    if (ret < 0)
    {
        return -1;
    }
    else
    {
        return ret;
    }
}
/**找到第一个符合的raw */
static raw_t *raw_find(ipaddr_t *src, int protocal)
{
    list_node_t *cur = raw_list.first;
    while (cur)
    {
        raw_t *cur_raw = list_node_parent(cur, raw_t, node);
        if (cur_raw->base.target_ip.q_addr == src->q_addr && cur_raw->base.protocal == protocal)
        {
            return cur_raw;
        }
        cur = cur->next;
    }
    return NULL;
}
static int raw_recvfrom(struct _sock_t *s, void *buf, ssize_t len, int flags,
                        struct sockaddr *src, socklen_t *addr_len)
{
    int ret;

    raw_t *raw = (raw_t *)s;
    struct sockaddr_in src_addr;
    src_addr.sin_addr.s_addr = s->target_ip.q_addr;
    src_addr.sin_family = s->family;
    memcpy(src, &src_addr, sizeof(struct sockaddr_in));
    *addr_len = sizeof(struct sockaddr_in);

    list_node_t *pkg_node = list_remove_first(&raw->recv_list);
    pkg_t *package = list_node_parent(pkg_node, pkg_t, node);
    int cpy_len = len > package->total ? package->total : len;
    ret = package_read_pos(package, buf, cpy_len, 0);
    if (ret < 0)
    {
        list_insert_first(&raw->recv_list, &package->node);
        dbg_error("package_read fault\r\n");
        return -2;
    }
    package_collect(package);
    return cpy_len;
}
static int raw_setsockopt(struct _sock_t *s, int level, int optname, const char *optval, int optlen)
{
    switch (level)
    {
    case SOL_SOCKET:

        if(optname & SO_RCVTIMEO)
        {
            struct timeval *time = (struct timeval *)optval;
            int tmo = time->tv_sec * 1000 + time->tv_usec / 1000;
            sock_wait_set(s,tmo,SOCK_RECV_WAIT);
        }
        if(optname & SO_SNDTIMEO)
        {
            struct timeval *time = (struct timeval *)optval;
            int tmo = time->tv_sec * 1000 + time->tv_usec / 1000;
            sock_wait_set(s,tmo,SOCK_SEND_WAIT);
        }
        
    default:
        dbg_error("unkown setsockopt optname\r\n");
        return -1;
    }
    return 0;
}

static int raw_close(sock_t *sock)
{
    raw_t *raw = (raw_t *)sock;
    list_node_t *cur_node = raw->recv_list.first;
    while (cur_node)
    {
        list_node_t *next = cur_node->next;
        list_remove(&raw->recv_list, cur_node);
        pkg_t *cur_pkg = list_node_parent(cur_node, pkg_t, node);
        package_collect(cur_pkg);
        cur_node = next;
    }
    list_remove(&raw_list, &raw->node);
    memset(raw, 0, sizeof(raw_t));
    mempool_free_blk(&raw_pool, raw);
    return 0;
}
static const sock_ops_t raw_ops = {
    .sendto = raw_sendto,
    .recvfrom = raw_recvfrom,
    .setsockopt = raw_setsockopt,
    .close = raw_close,
};
sock_t *raw_create(int family, int protocol)
{
    int ret;
    raw_t *raw = mempool_alloc_blk(&raw_pool, -1);
    if (!raw)
    {
        dbg_error("raw pool out of memory\r\n");
        return NULL;
    }
    memset(raw, 0, sizeof(raw_t));
    int proto = protocol == 0 ? RAW_DEFAULT_PROTOCAL : protocol;
    sock_init((sock_t *)raw, family, proto, &raw_ops);
    list_insert_last(&raw_list, &raw->node);
    return (sock_t *)raw;
}

int raw_in(netif_t *netif, pkg_t *package)
{
    // dbg_info("raw in ...\r\n");
    bool flag = false;
    ipv4_header_t *ipv4_head = package_data(package, sizeof(ipv4_header_t), 0);
    // 解析ipv4头，看看把数据包加到哪
    ipv4_head_parse_t parse;
    parse_ipv4_header(ipv4_head, &parse);
    // 这里可能有好几个raw，有些socket用着同样的协议和目的ip
    list_node_t *cur = raw_list.first;
    while (cur)
    {
        raw_t *cur_raw = list_node_parent(cur, raw_t, node);
        if (cur_raw->base.target_ip.q_addr == parse.src_ip.q_addr && cur_raw->base.protocal == parse.protocol)
        {
            flag = true;
            // 每找到一个符合条件的raw
            if (cur_raw->netif != netif)
            {
                cur_raw->netif = netif;
                cur_raw->base.host_ip.q_addr = netif->info.ipaddr.q_addr;
            }
            pkg_t *clone_pkg = package_clone(package);
            list_insert_last(&cur_raw->recv_list, &clone_pkg->node);
            sock_wait_notify(&cur_raw->base.recv_wait,NET_ERR_OK);
        }
        cur = cur->next;
    }
    if (!flag)
    {
        return -1;
    }
    package_collect(package);
    return 0;
}

void raw_init(void)
{
    list_init(&raw_list);
    mempool_init(&raw_pool, raw_buf, RAW_MAX_NR, sizeof(raw_t));
}