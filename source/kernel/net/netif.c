#include "netif.h"
#include "net_tools/net_mmpool.h"
#include "net_tools/package.h"
#include "net_tools/msgQ.h"

netif_t *netif_rtl8139;

static mempool_t netif_pool;
static uint8_t netif_buf[NETIF_MAX_NR * (sizeof(netif_t) + sizeof(list_node_t))];
static list_t netif_list;
static lock_t netif_list_locker;

#include "task/pid.h"
static pidalloc_t netid_allocer;
void netif_init(void)
{
    pidalloc_init(&netid_allocer);
    mempool_init(&netif_pool, netif_buf, NETIF_MAX_NR, sizeof(netif_t));
    list_init(&netif_list);
    lock_init(&netif_list_locker);
}

void netif_destory(void)
{
    mempool_destroy(&netif_pool);
    lock_destory(&netif_list_locker);
    list_destory(&netif_list);
}

netif_t *netif_alloc(void)
{
    netif_t *netif = (netif_t *)mempool_alloc_blk(&netif_pool, -1);
    if (netif)
    {
        memset(netif, 0, sizeof(netif_t));
        netif->id = pid_alloc(&netid_allocer);
        return netif;
    }
    return NULL;
}
int netif_free(netif_t *netif)
{
    int ret;
    pid_free(&netid_allocer, netif->id);

    while (!msgQ_is_empty(&netif->in_q))
    {
        pkg_t *package = msgQ_dequeue(&netif->in_q, -1);
        package_collect(package);
    }
    while (!msgQ_is_empty(&netif->out_q))
    {
        pkg_t *package = msgQ_dequeue(&netif->out_q, -1);
        package_collect(package);
    }
    msgQ_destory(&netif->in_q);
    msgQ_destory(&netif->out_q);
    memset(netif, 0, sizeof(netif_t));
    ret = mempool_free_blk(&netif_pool, netif);
    if (ret < 0)
    {
        dbg_error("netif free fail\r\n");
        return ret;
    }
    return 0;
}
int netif_insert(netif_t *netif)
{
    lock(&netif_list_locker);
    list_insert_last(&netif_list, &netif->node);
    unlock(&netif_list_locker);
    return 0;
}

netif_t *netif_remove(netif_t *netif)
{
    lock(&netif_list_locker);
    list_t *list = &netif_list;
    list_node_t *cur = list->first;
    while (cur)
    {
        if (cur == &netif->node)
        {
            list_remove(list, cur);
            unlock(&netif_list_locker);
            return netif;
        }
        cur = cur->next;
    }
    unlock(&netif_list_locker);
    return NULL;
}
netif_t *netif_first()
{
    list_t *list = &netif_list;
    list_node_t *fnode = list->first;
    netif_t *ret = list_node_parent(fnode, netif_t, node);
    return ret;
}
netif_t *netif_next(netif_t *netif)
{
    if (!netif)
    {
        dbg_error("netif is NULL,not have next\r\n");
        return NULL;
    }
    list_node_t *next = netif->node.next;
    if (!next)
    {
        return NULL;
    }
    else
    {
        return list_node_parent(next, netif_t, node);
    }
}

pkg_t *netif_get_pkg_from_outq(netif_t *netif, int timeout)
{
    pkg_t *package = (pkg_t *)msgQ_dequeue(&netif->out_q, timeout);
    if (!package)
    {
        return NULL;
    }
    package_lseek(package, 0);
    return package;
}

pkg_t *netif_get_pkg_from_inq(netif_t *netif, int timeout)
{
    pkg_t *package = (pkg_t *)msgQ_dequeue(&netif->in_q, timeout);
    if (!package)
    {
        return NULL;
    }
    package_lseek(package, 0);
    return package;
}

int netif_put_pkg_into_outq(netif_t *netif, pkg_t *package, int timeout)
{
    package_lseek(package, 0);
    return msgQ_enqueue(&netif->out_q, (void *)package, timeout);
}

int netif_put_pkg_into_inq(netif_t *netif, pkg_t *package, int timeout)
{
    package_lseek(package, 0);
    return msgQ_enqueue(&netif->in_q, (void *)package, timeout);
}

int netif_out(netif_t *netif, ipaddr_t *dest_ip, pkg_t *package)
{
    int ret;
    if (netif->link_ops)
    {
        return netif->link_ops->out(netif, dest_ip, package);
    }
    else
    {
        if (netif->info.type == NETIF_TYPE_LOOP)
        {
            // 如果是回环网口，直接把输出的包放到loop的输入队列中
            return netif_put_pkg_into_inq(netif, package, -1);
        }
        else
        {
            dbg_error("has never been installed link layer dirives\r\n");
            return -3;
        }
    }
    return 0;
}

netif_t *find_netif_by_name(const char *if_name)
{
    /*遍历网络接口链表，找名字为if_name的接口*/
    int name_len = strlen(if_name);
    netif_t *cur_if = netif_first();
    netif_t *target_if = NULL;
    while (cur_if)
    {
        if (strncmp(if_name, cur_if->info.name, name_len) == 0)
        {
            target_if = cur_if;
        }
        cur_if = netif_next(cur_if);
    }
    if (!target_if)
    {
        dbg_error("there is no netif name:%s\r\n", if_name);
        return -1;
    }
    return target_if;
}

void netif_show_info(netif_t *netif)
{
#ifdef NETIF_DBG
    // 类型转换为字符串
    const char *type_str = "Unknown";
    switch (netif->info.type)
    {
    case NETIF_TYPE_NONE:
        type_str = "None";
        break;
    case NETIF_TYPE_LOOP:
        type_str = "Loopback";
        break;
    case NETIF_TYPE_ETH:
        type_str = "Ethernet";
        break;
    }

    // 转换 IP 地址、网关和掩码为字符串
    char ip_str[16], gateway_str[16], mask_str[16];
    ipaddr_n2s(&netif->info.ipaddr, ip_str, sizeof(ip_str));
    ipaddr_n2s(&netif->info.gateway, gateway_str, sizeof(gateway_str));
    ipaddr_n2s(&netif->info.mask, mask_str, sizeof(mask_str));

    // 格式化 MAC 地址为字符串
    char mac_str[18];
    mac_n2s(&netif->hwaddr, mac_str);

    // 打印信息
    dbg_info("netif_id:%d\r\n", netif->id);
    dbg_info("netif_name:%s\r\n", netif->info.name);
    dbg_info("netif_mac:%s\r\n", mac_str);
    dbg_info("netif_ip:%s\r\n", ip_str);
    dbg_info("netif_gateway:%s\r\n", gateway_str);
    dbg_info("netif_mask:%s\r\n", mask_str);
#endif
}
void netif_show_list(void)
{
#ifdef NETIF_DBG
    netif_t *cur = netif_first();
    while (cur)
    {
        netif_show_info(cur);
        cur = netif_next(cur);
    }
#endif
}

int netif_set_ip(netif_t *target_if, const char *ip_str)
{
    int ret;

    ret = ipaddr_s2n(ip_str, &target_if->info.ipaddr);
    if (ret < 0)
    {
        dbg_error("set ip fail\r\n");
        return -2;
    }
    return 0;
}
int netif_set_gateway(netif_t *target_if, const char *ip_str)
{
    int ret;

    ret = ipaddr_s2n(ip_str, &target_if->info.gateway);
    if (ret < 0)
    {
        dbg_error("set ip fail\r\n");
        return -2;
    }
    return 0;
}
int netif_set_mask(netif_t *target_if, const char *ip_str)
{
    int ret;

    ret = ipaddr_s2n(ip_str, &target_if->info.mask);
    if (ret < 0)
    {
        dbg_error("set ip fail\r\n");
        return -2;
    }
    return 0;
}

netif_t *netif_get_8139(void)
{
    return netif_rtl8139;
}