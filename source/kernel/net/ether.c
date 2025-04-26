#include "ether.h"
#include "mem/malloc.h"
#include "protocal.h"
#include "arp.h"
#include "ipv4.h"
static void ether_show_head(ether_header_t *head)
{
#ifdef ETHER_DBG
    ETHER_DBG_PRINT("ether_header----\r\n");
    char src_buf[20] = {0};
    char dest_buf[20] = {0};
    hwaddr_t src_mac;
    hwaddr_t dest_mac;
    memcpy(src_mac.addr, head->src, MACADDR_ARRAY_LEN);
    memcpy(dest_mac.addr, head->dest, MACADDR_ARRAY_LEN);
    mac_n2s(&src_mac, src_buf);
    mac_n2s(&dest_mac, dest_buf);
    uint16_t proto = ntohs(head->protocal);
    ETHER_DBG_PRINT("mac src:%s\r\n", src_buf);
    ETHER_DBG_PRINT("mac dest:%s\r\n", dest_buf);
    ETHER_DBG_PRINT("protoal:0x%04x\r\n", proto);
    ETHER_DBG_PRINT("\r\n");
#endif
}
static void ether_show_pkg(pkg_t *package)
{
#ifdef ETHER_DBG
    ETHER_DBG_PRINT("ether in a package:\r\n");
    ether_header_t *ether_head = package_data(package, sizeof(ether_header_t), 0);
    ether_show_head(ether_head);
    package_print(package, sizeof(ether_header_t));
#endif
}
static int ether_pkg_is_ok(netif_t *netif, ether_header_t *header, int pkg_len)
{
    if (pkg_len > sizeof(ether_header_t) + ETHER_MTU_MAX_SIZE)
    {
        dbg_warning("ether pkg len over size\r\n");
        return -1;
    }

    if (pkg_len < sizeof(ether_header_t))
    {
        dbg_warning("ether pkg len so small\r\n");
        return -2;
    }
    // 既不是本机mac地址，也不是广播地址
    if (memcmp(netif->hwaddr.addr, header->dest, MACADDR_ARRAY_LEN) != 0 && memcmp(get_mac_broadcast()->addr, header->dest, MACADDR_ARRAY_LEN) != 0)
    {
        dbg_warning("ether recv a pkg,dest mac addr wrong\r\n");
        return -3;
    }
    return 0;
}

static int ether_open(netif_t *netif)
{
    int ret;
    ret = arp_send_free(netif);
    return 0;
}
static void ether_close(netif_t *netif)
{
}
static int ether_in(netif_t *netif, pkg_t *package)
{
    int ret;
    ether_header_t *ether_head = package_data(package, sizeof(ether_header_t), 0);
    if (ether_pkg_is_ok(netif, ether_head, package->total) < 0)
    {
        dbg_warning("recv a wrong fomat package in ether layer\r\n");
        return -1;
    }
    uint16_t proto = ntohs(ether_head->protocal);
    switch (proto)
    {
    case PROTOCAL_TYPE_ARP:
        // 去掉以太头
        ret = package_shrank_front(package, sizeof(ether_header_t));
        if (ret < 0)
        {
            dbg_error("package shrank ops fail\r\n");
            return -2;
        }
        return arp_in(netif, package);
        break;
    case PROTOCAL_TYPE_IPV4:
        // 去掉以太头
        ret = package_shrank_front(package, sizeof(ether_header_t));
        if (ret < 0)
        {
            dbg_error("package shrank ops fail\r\n");
            return -2;
        }
        return ipv4_in(netif,package);

    default:
        dbg_error("unkown protocal type\r\n");
        return -2;
        break;
    }
    return 0;
}
static int ether_out(netif_t *netif, ipaddr_t *dest, pkg_t *package)
{
    int ret;
    if (netif->state != NETIF_STATE_ACTIVE)
    {
        dbg_error("netif state not active,can not send pkg\r\n");
        return -1;
    }
    //如果ip地址就是自己，那就不用查arp表来找mac地址了
    if (dest->q_addr == netif->info.ipaddr.q_addr)
    {
        return ether_raw_out(netif, PROTOCAL_TYPE_IPV4, &netif->hwaddr, package);
    }
    else
    {
        return arp_reslove(netif,dest,package);
    }
    return 0;
}

int ether_raw_out(netif_t *netif, protocal_type_t type, const hwaddr_t *mac_dest, pkg_t *pkg)
{
    int ret;
    netif->send_pkg_cnt++;
    int data_size = pkg->total;
    // 不足46字节，调整至46字节
    if (data_size < ETHER_MTU_MIN_SIZE)
    {
        int expand_size = ETHER_MTU_MIN_SIZE - data_size;
        package_expand_last(pkg, expand_size);
        package_memset(pkg, data_size, 0, expand_size);
    }
    ret = package_add_headspace(pkg, sizeof(ether_header_t));
    if (ret < 0)
    {
        dbg_error("pkg add header fail\r\n");
        netif->send_fail_cnt++;
        return ret;
    }
    // 填充以太网数据包头
    ether_header_t *head = package_data(pkg, sizeof(ether_header_t), 0);
    memcpy(head->src, netif->hwaddr.addr, MACADDR_ARRAY_LEN);
    memcpy(head->dest, mac_dest->addr, MACADDR_ARRAY_LEN);
    head->protocal = htons(type);

    //ether_show_head(head); // 显示包头信息

    // 目的mac地址就是自己，直接放入输入队列即可
    if (memcmp(netif->hwaddr.addr, mac_dest->addr, MACADDR_ARRAY_LEN) == 0)
    {
        netif_put_pkg_into_inq(netif, pkg, -1);
        return 0; // 这算是发送成功了
    }

    ret = netif_put_pkg_into_outq(netif, pkg, -1); // 将包放入输出队列
    if (ret < 0)
    {
        // 包都没放进去，pkg可以回收了
        netif->send_fail_cnt++;
        return -2;
    }
    ret = netif->ops->send(netif); // 调用8139驱动输出数据
    if (ret < 0)
        netif->send_fail_cnt++;
    else
        package_collect(pkg);//该包已经成功发送了
    return ret;
}
const link_layer_t ether_ops = {
    .type = NETIF_TYPE_ETH,
    .open = ether_open,
    .close = ether_close,
    .in = ether_in,
    .out = ether_out};