#include "ipv4.h"
#include "ipaddr.h"
#include "algrithem.h"
#include "protocal.h"
#include "loop.h"
#include "icmpv4.h"
#include "net_tools/net_mmpool.h"
#include "net_tools/soft_timer.h"
#include "raw.h"
#include "udp.h"
#include "tcp.h"
static uint16_t ipv4_frag_global_id = 1;

/*ip route table*/
static uint8_t ip_route_entry_buf[IP_ROUTE_ENTRY_MAX_NR * (sizeof(list_node_t) + sizeof(ip_route_entry_t))];
static mempool_t ip_route_entry_pool;
static list_t ip_route_table;

ip_route_entry_t *ip_route_entry_alloc(void)
{
    ip_route_entry_t *entry = mempool_alloc_blk(&ip_route_entry_pool, -1);
    if (!entry)
        return NULL;
    memset(entry, 0, sizeof(ip_route_entry_t));
    return entry;
}
void ip_route_entry_free(ip_route_entry_t *entry)
{
    memset(entry, 0, sizeof(ip_route_entry_t));
    mempool_free_blk(&ip_route_entry_pool, entry);
}

ip_route_entry_t *ip_route_entry_find(ip_route_find_type_t type, void *data)
{
    switch (type)
    {
    case IP_ROUTE_FIND_TARGET:
        ipaddr_t *target = (ipaddr_t *)data;
        list_node_t *cur = ip_route_table.first;
        while (cur)
        {
            ip_route_entry_t *entry = list_node_parent(cur, ip_route_entry_t, node);
            if (entry->target.q_addr == target->q_addr)
            {
                return entry;
            }
            cur = cur->next;
        }
        break;
    case IP_ROUTE_FIND_NETIF:
        netif_t *netif = (netif_t *)data;
        list_node_t *xcur = ip_route_table.first;
        while (cur)
        {
            ip_route_entry_t *entry = list_node_parent(xcur, ip_route_entry_t, node);
            if (entry->netif == netif)
            {
                return entry;
            }
            xcur = xcur->next;
        }
        break;

    default:
        break;
    }
    return NULL;
}
int ip_route_entry_add(ip_route_entry_t *entry)
{
    // 找到第一个目标比他大的，插它前面
    list_node_t *cur = ip_route_table.first;
    while (cur)
    {
        ip_route_entry_t *find = list_node_parent(cur, ip_route_entry_t, node);
        if (find->target.q_addr > entry->target.q_addr)
        {
            list_insert_front(&ip_route_table, &find->node, &entry->node);
            break;
        }
        else if (find->target.q_addr == entry->target.q_addr)
        {
            if (find->metric > entry->metric)
            {
                list_insert_front(&ip_route_table, &find->node, &entry->node);
            }
            else
            {
                list_insert_behind(&ip_route_table, &find->node, &entry->node);
            }
            break;
        }
        cur = cur->next;
    }
    // 啥都没找到插最后面
    list_insert_last(&ip_route_table, &entry->node);
    return 0;
}
int ip_route_entry_delete(ip_route_entry_t *entry)
{
    list_remove(&ip_route_table, &entry->node);
}

int ip_route_entry_set(const char *target, const char *gateway, const char *mask, int metric, const char *netif_name)
{
    ip_route_entry_t *entry = ip_route_entry_alloc();
    ipaddr_s2n(target, &entry->target);
    ipaddr_s2n(gateway, &entry->gateway);
    ipaddr_s2n(mask, &entry->mask);
    entry->metric = metric;
    entry->netif = find_netif_by_name(netif_name);
    ip_route_entry_add(entry);
    return 0;
}

static void ip_route_table_init(void)
{
    list_init(&ip_route_table);
    mempool_init(&ip_route_entry_pool, ip_route_entry_buf, IP_ROUTE_ENTRY_MAX_NR, sizeof(ip_route_entry_t));
}

ip_route_entry_t *ip_route(ipaddr_t *dest)
{

    int metric_min = 1000000;
    ip_route_entry_t *entry_ret = NULL;
    // 遍历一遍路由表，网络部分一样就选中，有好几个，比较metric，哪个小选哪个
    list_node_t *cur = ip_route_table.first;
    while (cur)
    {
        ip_route_entry_t *entry = list_node_parent(cur, ip_route_entry_t, node);
        uint32_t dest_net = ipaddr_get_net(dest, &entry->mask);
        uint32_t entry_net = ipaddr_get_net(&entry->target, &entry->mask);
        if (dest_net == entry_net)
        {
            // 属于同一个网段
            int metric = entry->metric;
            if (metric < metric_min)
            {
                metric_min = metric;
                entry_ret = entry;
            }
        }
        cur = cur->next;
    }
    return entry_ret;
}

static bool ipv4_is_ok(ipv4_header_t *ip_head, ipv4_head_parse_t *parse)
{
    if (parse->version != IPV4_HEAD_VERSION)
    {
        return false;
    }
    if (parse->head_len < IPV4_HEAD_MIN_SIZE || parse->head_len > IPV4_HEAD_MAX_SIZE)
    {
        return false;
    }
    if (parse->head_len != sizeof(ipv4_header_t))
    {
        // 有额外数据的ip包暂时不处理
        return false;
    }
    if (parse->total_len < parse->head_len)
    {
        return false;
    }
    // 校验和为0时，即为不需要检查检验和
    if (parse->checksum)
    {
        uint16_t c = checksum16(0, (uint16_t *)ip_head, parse->head_len, 0, 1);
        if (c != 0)
        {
            dbg_warning("Bad checksum: %0x(correct is: %0x)\n", parse->checksum, c);
            return 0;
        }
    }
    return true;
}
static bool ipv4_is_match(netif_t *netif, ipaddr_t *dest_ip)
{
    // 本机ip，局部广播,255.255.255.255 都是匹配的

    // 是否是255.255.255.255
    if (is_global_boradcast(dest_ip))
    {
        return true;
    }
    // 看看主机部分是否为全1
    if (is_local_boradcast(&netif->info.ipaddr, &netif->info.mask, dest_ip))
    {
        return true;
    }
    if (netif->info.ipaddr.q_addr == dest_ip->q_addr)
    {
        return true;
    }
    return false;
}

/*无需分片的包*/
static int ipv4_normal_in(netif_t *netif, pkg_t *package, ipv4_head_parse_t *parse)
{
    int ret;
    ipv4_header_t *head = (ipv4_header_t *)package_data(package, sizeof(ipv4_header_t), 0);
    // 保存ipv4头
    ipv4_header_t ipv4_head;
    package_read_pos(package, &ipv4_head, sizeof(ipv4_header_t), 0);
    // 除去数据包中的ipv4头
    package_shrank_front(package, sizeof(ipv4_header_t));
    switch (parse->protocol)
    {
    case PROTOCAL_TYPE_ICMPV4:

        return icmpv4_in(netif, &parse->src_ip, &netif->info.ipaddr, package, &ipv4_head);
        break;
    case PROTOCAL_TYPE_UDP:
        ret = udp_in(package, &parse->src_ip, &parse->dest_ip);
        if (ret < 0)
        {
            if (ret == -3)
            {
                icmpv4_send_unreach(&parse->src_ip, package, ICMPv4_UNREACH_PORT);
            }
            else
            {
                return ret;
            }
        }

        break;
    case PROTOCAL_TYPE_TCP:
        ret = tcp_in(package,&parse->src_ip,&parse->dest_ip);
        if(ret < 0){
            return ret;
        }
        else{
            return 0;
        }
        break;
    default:
        ret = raw_in(netif, package);
        if (ret < 0)
        {
            return -1;
        }
        break;
    }
    return 0;
}

static int ipv4_frag_in(netif_t *netif, pkg_t *pkg, ipv4_head_parse_t *parse_head)
{
    int ret;
    ipaddr_t src_ip = {
        .type = IPADDR_V4,
        .q_addr = parse_head->src_ip.q_addr};
    ip_frag_t *frag = ipv4_frag_find(&src_ip, parse_head->id);
    if (!frag)
    {
        frag = ipv4_frag_alloc();
        frag->id = parse_head->id;
        frag->ip.q_addr = parse_head->src_ip.q_addr;
        frag->ip.type = IPADDR_V4;
    }

    ret = ipv4_frag_insert_pkg(frag, pkg, parse_head);
    if (ret < 0)
    {
        return ret;
    }
    ipv4_frag_list_print();

    if (ipv4_frag_is_all_arrived(frag))
    {
        // ip头总长度，校验和还没改
        pkg_t *pkg_all = ipv4_frag_join_pkg(frag);
        // 释放frag
        ipv4_frag_free(frag);
        ipv4_header_t *head = package_data(pkg_all, sizeof(ipv4_header_t), 0);
        ipv4_head_parse_t parse;
        parse_ipv4_header(head, &parse);
        ret = ipv4_normal_in(netif, pkg_all, &parse);
        if (ret < 0)
        {
            package_collect(pkg_all); // 大包未正确处理回收大包
            return ret;
        }
    }
    // 小包正确处理由上层释放
    return 0;
}
/**把一个大包分成多片发送 */
static int ipv4_frag_out(netif_t *netif, pkg_t *pkg, uint8_t protocal, ipaddr_t *dest)
{
    int ret;
    ipaddr_t *src = &netif->info.ipaddr;
    int sum_size = 0;
    int remain_size = pkg->total;
    int cur_pos = 0;
    int old_pos = 0;
    pkg_t *frag_pkg = NULL;
    while (remain_size)
    {
        if (remain_size > netif->mtu)
        {
            frag_pkg = package_alloc(netif->mtu);
            if (!frag_pkg)
            {
                dbg_error("alloc pkg fail\r\n");
                return -1;
            }
            int cpy_size = netif->mtu - sizeof(ipv4_header_t);
            ret = package_memcpy(frag_pkg, sizeof(ipv4_header_t), pkg, cur_pos, cpy_size);
            if (ret < 0)
            {
                dbg_error("pkg copy fail\r\n");
                package_collect(frag_pkg);
                return -2;
            }
            // package_print(frag_pkg);
            old_pos = cur_pos;
            cur_pos += cpy_size;
            remain_size -= cpy_size;
        }
        else
        {
            frag_pkg = package_alloc(remain_size + sizeof(ipv4_header_t));
            if (!frag_pkg)
            {
                dbg_error("alloc pkg fail\r\n");
                return -1;
            }
            int cpy_size = remain_size;
            ret = package_memcpy(frag_pkg, sizeof(ipv4_header_t), pkg, cur_pos, cpy_size);
            if (ret < 0)
            {
                dbg_error("pkg copy fail\r\n");
                package_collect(frag_pkg);
                return -2;
            }
            // package_print(frag_pkg);
            old_pos = cur_pos;
            cur_pos += cpy_size;
            remain_size -= cpy_size;
        }

        // 填充头部
        ipv4_header_t *head = package_data(frag_pkg, sizeof(ipv4_header_t), 0);
        package_memset(frag_pkg, 0, 0, sizeof(ipv4_header_t));
        ipv4_head_parse_t parse;
        memset(&parse, 0, sizeof(ipv4_head_parse_t));

        parse.version = IPV4_HEAD_VERSION;
        parse.head_len = sizeof(ipv4_header_t);
        parse.total_len = frag_pkg->total;
        parse.id = ipv4_frag_global_id;
        parse.flags = remain_size > 0 ? IPV4_HEAD_FLAGS_MORE_FRAGMENT : 0;
        parse.frag_offset = old_pos >> 3;

        parse.ttl = IPV4_HEAD_TTL_DEFAULT;
        parse.protocol = protocal;
        parse.checksum = 0;
        parse.src_ip.q_addr = src->q_addr;
        parse.dest_ip.q_addr = dest->q_addr;

        ipv4_set_header(&parse, head);
        uint16_t check_ret = package_checksum16(frag_pkg, 0, sizeof(ipv4_header_t), 0, 1);
        // 这里直接赋值，不要大小端转换  解析的时候，checksum转不转换都行
        head->h_checksum = check_ret;
        int frag_size = frag_pkg->total;
        // package_print(frag_pkg);
        ip_route_entry_t *entry = ip_route(dest);
        if (!entry)
        {
            dbg_warning("ip route fail\r\n");
            package_collect(frag_pkg);
            return -4;
        }
        if (entry->gateway.q_addr != 0)
        {
            dest->q_addr = entry->gateway.q_addr;
        }
        ret = netif_out(netif, dest, frag_pkg); // 成功发送ether层回收
        if (ret < 0)
        {
            dbg_warning("the frag_pkg send fail\r\n");
            package_collect(frag_pkg);
            return -3;
        }
        else
        {
            sum_size += (frag_size - sizeof(ipv4_header_t));
        }
    }
    ipv4_frag_global_id++;
    package_collect(pkg); // 回收大包，小包发送成功或者失败都已经处理好了
    return sum_size;
}
static int ipv4_normal_out(netif_t *netif, pkg_t *package, protocal_type_t protocal, ipaddr_t *dest)
{
    // package_print(package,0);
    int ret;
    int pkg_total = package->total;
    package_add_headspace(package, sizeof(ipv4_header_t));
    ipv4_header_t *head = package_data(package, sizeof(ipv4_header_t), 0);

    ipv4_head_parse_t parse;
    memset(&parse, 0, sizeof(ipv4_head_parse_t));

    parse.version = IPV4_HEAD_VERSION;
    parse.head_len = sizeof(ipv4_header_t);
    parse.total_len = package->total;
    parse.flags = IPV4_HEAD_FLAGS_NOT_FRAGMENT;
    parse.ttl = IPV4_HEAD_TTL_DEFAULT;
    parse.protocol = protocal;
    parse.checksum = 0;
    parse.src_ip.q_addr = netif->info.ipaddr.q_addr;
    parse.dest_ip.q_addr = dest->q_addr;

    ipv4_set_header(&parse, head);
    ipv4_show_head(head);
    uint16_t check_ret = package_checksum16(package, 0, sizeof(ipv4_header_t), 0, 1);
    head->h_checksum = check_ret;
    // package_print(package,0);
    ip_route_entry_t *entry = ip_route(dest);
    if (!entry)
    {
        dbg_warning("ip route fail\r\n");
        return -4;
    }
    if (entry->gateway.q_addr != 0)
    {
        dest->q_addr = entry->gateway.q_addr;
    }
    ret = netif_out(netif, dest, package);
    if (ret < 0)
    {
        return ret;
    }
    else
    {
        return pkg_total;
    }
}
void parse_ipv4_header(const ipv4_header_t *ip_head, ipv4_head_parse_t *parsed)
{
    // 解析字段
    parsed->version = (ip_head->version_and_ihl >> 4) & 0x0F;
    parsed->head_len = (ip_head->version_and_ihl & 0x0F) * 4;

    parsed->dscp = (ip_head->DSCP6_and_ENC2 >> 2) & 0x3F;
    parsed->enc = ip_head->DSCP6_and_ENC2 & 0x03;

    parsed->total_len = ntohs(ip_head->total_len);
    parsed->id = ntohs(ip_head->id);

    uint16_t frag_flags_and_offset_host = 0;
    // 将网络字节序转换为主机字节序
    frag_flags_and_offset_host = ntohs(ip_head->frag_flags_and_offset);
    // 提取 flags 和 frag_offset
    parsed->flags = (frag_flags_and_offset_host >> 13) & 0x07;       // 高3位为 flags
    parsed->frag_offset = (frag_flags_and_offset_host & 0x1FFF) * 8; // 低13位为 frag_offset

    parsed->ttl = ip_head->ttl;
    parsed->protocol = ip_head->protocal;

    parsed->checksum = ntohs(ip_head->h_checksum);
    parsed->src_ip.q_addr = ntohl(ip_head->src_ip);
    parsed->dest_ip.q_addr = ntohl(ip_head->dest_ip);
}
void ipv4_set_header(const ipv4_head_parse_t *parsed, ipv4_header_t *head)
{
    // 将 version 和 head_len 合并到 version_and_ihl
    head->version_and_ihl = ((parsed->version & 0x0F) << 4) | ((parsed->head_len / 4) & 0x0F);

    // DSCP 和 ENC2
    head->DSCP6_and_ENC2 = ((parsed->dscp & 0x3F) << 2) | (parsed->enc & 0x03);

    // 总长度（转换为网络字节序）
    head->total_len = htons(parsed->total_len);

    // 标识字段（转换为网络字节序）
    head->id = htons(parsed->id);

    // flags 和 fragment offset 合并后转换为网络字节序
    uint16_t frag_flags_and_offset_host = ((parsed->flags & 0x07) << 13) | (parsed->frag_offset & 0x1FFF);
    head->frag_flags_and_offset = htons(frag_flags_and_offset_host);

    // TTL 和协议
    head->ttl = parsed->ttl;
    head->protocal = parsed->protocol;

    // 校验和（转换为网络字节序）
    head->h_checksum = htons(parsed->checksum);

    // 源地址和目标地址（转换为网络字节序）
    head->src_ip = htonl(parsed->src_ip.q_addr);
    head->dest_ip = htonl(parsed->dest_ip.q_addr);
}
int ipv4_in(netif_t *netif, pkg_t *package)
{
    int ret;
    // 解析包头
    ipv4_header_t *ip_head = (ipv4_header_t *)package_data(package, sizeof(ipv4_header_t), 0);
    ipv4_head_parse_t parse_head;
    parse_ipv4_header(ip_head, &parse_head);
    ipv4_show_pkg(&parse_head);
    if (!ipv4_is_ok(ip_head, &parse_head))
    {
        dbg_warning("recv a wrong format ipv4 package\r\n");
        return -1;
    }
    if (!ipv4_is_match(netif, &parse_head.dest_ip))
    {
        dbg_warning("routing error:recv a pkg dest_ip not the host\r\n");
        return -2;
    }
    if (package->total > parse_head.total_len)
    {
        // ether 最小字节数46，可能自动填充了一些0
        package_shrank_last(package, package->total - parse_head.total_len);
    }

    if (parse_head.flags & 0x1 || parse_head.frag_offset)
    {
        // 没接收，让上次释放 ； 接受了也不能释放，在分片列表中存着
        return ipv4_frag_in(netif, package, &parse_head);
    }
    else
    {
        ret = ipv4_normal_in(netif, package, &parse_head);
        if (ret < 0)
        {
            return -4;
        }
        // 成功收到ip包，也不能释放，存到socket的recvlist中了
        // 应用层取走释放
    }
}

/**
 * 返回成功发送的字节数
 */
int ipv4_out(pkg_t *package, protocal_type_t protocal, ipaddr_t *dest)
{

    // 看看高位是否是127
    int ret;
    netif_t *netif;
    // 如果是回环接口
    if (dest->a_addr[IPADDR_ARRY_LEN - 1] == 0x7F)
    {
        netif = netif_loop;
    }
    else // 如果不是物理接口
    {
        // 通过路由找到发出数据包的接口
        ip_route_entry_t *entry = ip_route(dest);
        if (!entry)
        {
            dbg_warning("ip route fail\r\n");
            return -4;
        }
        netif = entry->netif;
    }
    if (netif->mtu <= 0)
    {
        dbg_error("netif unset mtu\r\n");
        return -1;
    }
    if (package->total + sizeof(ipv4_header_t) > netif->mtu)
    {
        ret = ipv4_frag_out(netif, package, protocal, dest);
        if (ret < 0)
        {
            package_collect(package);
            return ret;
        }
    }
    else
    {
        ret = ipv4_normal_out(netif, package, protocal, dest);
        if (ret < 0)
        {
            package_collect(package);
            return ret;
        }
    }
    return ret;
}

/**ipfrag */

static uint8_t frag_buff[IPV4_FRAG_MAX * (sizeof(list_node_t) + sizeof(ip_frag_t))];
static mempool_t ip_frag_pool;
static list_t ip_frag_list;
static soft_timer_t ipv4_frag_timer;
static void *frag_tmo_handle(void *arg)
{
    list_t *frag_list = &ip_frag_list;
    list_node_t *cur = frag_list->first;
    while (cur)
    {
        list_node_t *next = cur->next;
        ip_frag_t *frag = list_node_parent(cur, ip_frag_t, node);
        if (--frag->tmo <= 0)
        {
            ipv4_frag_free(frag);
        }
        cur = next;
    }
    return NULL;
}
void ipv4_frag_init(void)
{
    list_init(&ip_frag_list);
    mempool_init(&ip_frag_pool, frag_buff, IPV4_FRAG_MAX, sizeof(ip_frag_t));
    soft_timer_add(&ipv4_frag_timer, SOFT_TIMER_TYPE_PERIOD, IPV4_FRAG_TIMER_SCAN * 1000, "IPV4_FRAG_TIMER", frag_tmo_handle, NULL, NULL);
}
/*释放，而且把包都回收*/
void ipv4_frag_free(ip_frag_t *frag)
{
    if (!frag)
    {
        dbg_error("frag is null\r\n");
        return;
    }
    list_node_t *free_node = &frag->node;
    list_t *pkg_list = &frag->frag_list;
    while (list_count(pkg_list) > 0)
    {
        list_node_t *pkg_node = list_remove_first(pkg_list);
        pkg_t *pkg = list_node_parent(pkg_node, pkg_t, node);
        package_collect(pkg);
    }
    list_remove(&ip_frag_list, free_node);
    mempool_free_blk(&ip_frag_pool, frag);
}
/**分配fragt */
ip_frag_t *ipv4_frag_alloc(void)
{
    ip_frag_t *ret = NULL;
    ip_frag_t *frag = mempool_alloc_blk(&ip_frag_pool, -1);
    if (!frag)
    {
        ip_frag_t *last_frag = list_node_parent(&ip_frag_list.last, ip_frag_t, node);
        // 把数据包都释放掉
        list_t *pkg_list = &last_frag->frag_list;
        while (list_count(pkg_list) > 0)
        {
            list_node_t *pkg_node = list_remove_first(pkg_list);
            pkg_t *pkg = list_node_parent(pkg_node, pkg_t, node);
            package_collect(pkg);
        }
        ret = last_frag;
        list_remove(&ip_frag_list, &ret->node);
    }
    else
    {
        memset(frag, 0, sizeof(ip_frag_t));
        ret = frag;
    }
    // 把刚分配的frag结构插入首部
    frag->tmo = IPV4_FRAG_TMO / IPV4_FRAG_TIMER_SCAN;
    list_insert_first(&ip_frag_list, &ret->node);
    return ret;
}
ip_frag_t *ipv4_frag_find(ipaddr_t *ip, uint16_t id)
{
    ip_frag_t *ret = NULL;
    list_t *list = &ip_frag_list;
    list_node_t *cur = list->first;
    while (cur)
    {
        ip_frag_t *frag = list_node_parent(cur, ip_frag_t, node);
        if (frag->ip.q_addr == ip->q_addr && id == frag->id)
        {
            ret = frag;
            list_remove(&ip_frag_list, &frag->node);
            list_insert_first(&ip_frag_list, &frag->node);
            break;
        }
        cur = cur->next;
    }
    return ret;
}
int ipv4_frag_insert_pkg(ip_frag_t *frag, pkg_t *pkg, ipv4_head_parse_t *parse)
{
    if (!frag || !pkg || !parse)
    {
        return -1;
    }
    list_t *pkg_list = &frag->frag_list;
    list_node_t *cur = pkg_list->first;
    while (cur)
    {
        pkg_t *cur_pkg = list_node_parent(cur, pkg_t, node);
        ipv4_header_t *ip_head = package_data(cur_pkg, sizeof(ipv4_header_t), 0);
        int frag_flags_and_offset_host = 0;
        frag_flags_and_offset_host = ntohs(ip_head->frag_flags_and_offset);
        uint16_t offset = (frag_flags_and_offset_host & 0x1FFF) * 8;
        // 安顺序插入
        if (parse->frag_offset < offset)
        {
            list_insert_front(pkg_list, &cur_pkg->node, &pkg->node);
            break;
        }
        else if (parse->frag_offset == offset)
        {
            break;
        }
        cur = cur->next;
    }
    if (!cur)
    {
        list_insert_last(pkg_list, &pkg->node);
    }
    return 0;
}
int ipv4_frag_is_all_arrived(ip_frag_t *frag)
{
    list_t *pkg_list = &frag->frag_list;
    list_node_t *cur_pkg_node = pkg_list->first;
    if (list_count(pkg_list) <= 1)
    {
        return 0; // 就一个分片，肯定不全
    }
    //
    pkg_t *first_pkg = list_node_parent(cur_pkg_node, pkg_t, node);
    ipv4_header_t *first_ip_head = package_data(first_pkg, sizeof(ipv4_header_t), 0);
    ipv4_head_parse_t first_parse;
    parse_ipv4_header(first_ip_head, &first_parse);
    if (first_parse.frag_offset != 0 || !(first_parse.flags & 0x1))
    {
        return 0; // 第一个分片的偏移量不是0，肯定不全
    }
    while (cur_pkg_node)
    {
        pkg_t *cur_pkg = list_node_parent(cur_pkg_node, pkg_t, node);
        ipv4_header_t *ip_head = package_data(cur_pkg, sizeof(ipv4_header_t), 0);
        uint16_t frag_flags_and_offset_host = 0, frag_offset = 0, total_len = 0;
        uint8_t flags = 0, head_len = 0;
        // 将网络字节序转换为主机字节序
        frag_flags_and_offset_host = ntohs(ip_head->frag_flags_and_offset);
        // 提取 flags 和 frag_offset
        flags = (frag_flags_and_offset_host >> 13) & 0x07;       // 高3位为 flags
        frag_offset = (frag_flags_and_offset_host & 0x1FFF) * 8; // 低13位为 frag_offset
        head_len = (ip_head->version_and_ihl & 0x0F) * 4;
        total_len = ntohs(ip_head->total_len);
        uint16_t data_len = total_len - head_len;
        list_node_t *next = NULL;
        if (flags & 0x1)
        {
            // MF==1,应该有下一个分片
            next = cur_pkg_node->next;
            if (!next)
            {
                return 0; // 分片没有全部到达
            }
            else
            {
                pkg_t *next_pkg = list_node_parent(next, pkg_t, node);
                ipv4_header_t *next_ip_head = package_data(next_pkg, sizeof(ipv4_header_t), 0);
                uint16_t next_flags_and_offset = 0, next_offset = 0;
                next_flags_and_offset = ntohs(next_ip_head->frag_flags_and_offset);
                next_offset = (next_flags_and_offset & 0x1FFF) * 8; // 低13位为 frag_offset
                if (next_offset != data_len + frag_offset)
                {
                    return 0; // 分片没有全部到达
                }
                else
                {
                    cur_pkg_node = next;
                }
            }
        }
        else
        {
            return 1;
        }
    }
    return 1;
}
/**把一个分片链表的所有包合成一个大ip包 */
pkg_t *ipv4_frag_join_pkg(ip_frag_t *frag)
{
    pkg_t *to, *from;
    list_t *pkg_list = &frag->frag_list;
    list_node_t *first_pkg_node = pkg_list->first;
    pkg_t *first_pkg = list_node_parent(first_pkg_node, pkg_t, node);

    list_remove(pkg_list, first_pkg_node);
    to = first_pkg;

    first_pkg_node = pkg_list->first;
    while (first_pkg_node)
    {
        list_node_t *next_pkg_node = first_pkg_node->next;
        from = list_node_parent(first_pkg_node, pkg_t, node);
        package_shrank_front(from, sizeof(ipv4_header_t));
        list_remove(pkg_list, first_pkg_node);
        package_join(from, to);
        first_pkg_node = next_pkg_node;
    }
    return to;
}

/*dbg*/
void ip_route_entry_show(ip_route_entry_t *entry)
{
    char ip_buf[20] = {0};
    ipaddr_n2s(&entry->target, ip_buf, 20);
    dbg_info("%s  ", ip_buf);
    memset(ip_buf, 0, 20);

    ipaddr_n2s(&entry->gateway, ip_buf, 20);
    dbg_info("%s  ", ip_buf);
    memset(ip_buf, 0, 20);

    ipaddr_n2s(&entry->mask, ip_buf, 20);
    dbg_info("%s  ", ip_buf);
    memset(ip_buf, 0, 20);

    dbg_info("%d  ", entry->metric);
    dbg_info("%s  ", entry->netif->info.name);
    dbg_info("\r\n");
}
void ip_route_show(void)
{
    dbg_info("target        gateway         netmask         metric      netif\r\n");
    list_node_t *cur = ip_route_table.first;
    while (cur)
    {
        ip_route_entry_t *entry = list_node_parent(cur, ip_route_entry_t, node);
        ip_route_entry_show(entry);
        cur = cur->next;
    }
}
void ipv4_frag_print(ip_frag_t *frag)
{
#ifdef IPV4_DBG
    char ipbuf[20] = {0};
    ipaddr_n2s(&frag->ip, ipbuf, 20);
    dbg_info("the frag ip is:%s\r\n", ipbuf);
    dbg_info("the frag id is%d\r\n", frag->id);
    dbg_info("the pkg list like follow:\r\n");
    list_t *list = &frag->frag_list;
    list_node_t *cur = list->first;
    int count = 0;
    while (cur)
    {
        pkg_t *pkg = list_node_parent(cur, pkg_t, node);
        ipv4_header_t *head = package_data(pkg, sizeof(ipv4_header_t), 0);
        ipv4_head_parse_t parse;
        parse_ipv4_header(head, &parse);
        dbg_info("..................................\r\n");
        dbg_info("pkg_%d:\r\n", count);
        dbg_info("pkg_data_len:%d\r\n", parse.total_len - parse.head_len);
        dbg_info("flags_3,More fragment:%x\r\n", (parse.flags & 0x01) ? 1 : 0);
        dbg_info("frag_offset:%d\r\n", parse.frag_offset);
        dbg_info("..................................\r\n");
        cur = cur->next;
        count++;
    }
#endif
}
void ipv4_frag_list_print(void)
{
#ifdef IPV4_DBG
    list_t *frag_list = &ip_frag_list;
    list_node_t *cur_list = frag_list->first;
    int count = 0;
    while (cur_list)
    {
        dbg_info("print frag list%d++++++++++++\r\n", count);
        ip_frag_t *frag = list_node_parent(cur_list, ip_frag_t, node);
        ipv4_frag_print(frag);
        cur_list = cur_list->next;
        count++;
    }
#endif
}
void ipv4_show_head(ipv4_header_t *head)
{
#ifdef IPV4_DBG
    ipv4_head_parse_t parse;
    parse_ipv4_header(head, &parse);
    ipv4_show_pkg(&parse);
#endif
}
void ipv4_show_pkg(ipv4_head_parse_t *parse)
{
#ifdef IPV4_DBG
    dbg_info("++++++++++++++++++++show ipv4 header++++++++++++++++\r\n");
    dbg_info("version:%d\r\n", parse->version);
    dbg_info("head_len:%d\r\n", parse->head_len);
    dbg_info("dscp:%d\r\n", parse->dscp);
    dbg_info("enc:%d\r\n", parse->enc);
    dbg_info("total_len:%d\r\n", parse->total_len);
    dbg_info("id:0x%04x\r\n", parse->id);
    dbg_info("flags_1,Reserved bit:%x\r\n", (parse->flags & 0x04) ? 1 : 0);
    dbg_info("flags_2,Dont fragment:%x\r\n", (parse->flags & 0x02) ? 1 : 0);
    dbg_info("flags_3,More fragment:%x\r\n", (parse->flags & 0x01) ? 1 : 0);
    dbg_info("frag_offset:%d\r\n", parse->frag_offset);
    dbg_info("ttl:%d\r\n", parse->ttl);
    dbg_info("protocal:0x%02x\r\n", parse->protocol);
    dbg_info("checksum:0x%04x\r\n", parse->checksum);

    char src_buf[20] = {0};
    char dest_buf[20] = {0};
    ipaddr_n2s(&parse->src_ip, src_buf, 20);
    ipaddr_n2s(&parse->dest_ip, dest_buf, 20);
    dbg_info("src_ip:%s\r\n", src_buf);
    dbg_info("dest_ip:%s\r\n", dest_buf);
    dbg_info("++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
#endif
}

void ipv4_init(void)
{
    ipv4_frag_init();
    ip_route_table_init();
    return;
}