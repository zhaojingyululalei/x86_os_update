#include "arp.h"
#include "ether.h"
#include "net_tools/soft_timer.h"
arp_cache_table_t arp_cache_table;

static int arp_show_cache_entry(arp_entry_t *entry)
{

    char ip_buf[20] = {0};
    char mac_buf[20] = {0};
    char *state;
    ipaddr_n2s(&entry->ipaddr, ip_buf, 20);
    mac_n2s(&entry->hwaddr, mac_buf);

    switch (entry->state)
    {
    case ARP_ENTRY_STATE_NONE:
        state = "NONE";
        break;
    case ARP_ENTRY_STATE_WAITING:
        state = "waiting";
        break;
    case ARP_ENTRY_STATE_RESOLVED:
        state = "resolved";
        break;
    default:
        dbg_error("unkown state\r\n");
        return -1;
    }
    ARP_DBG_PRINT("-----\r\n");
    ARP_DBG_PRINT("cache entry ip:%s\r\n", ip_buf);
    ARP_DBG_PRINT("cache entry mac:%s\r\n", mac_buf);
    ARP_DBG_PRINT("cache entry state:%s\r\n", state);
    ARP_DBG_PRINT("cache entry netif:%s\r\n", entry->netif->info.name);
    ARP_DBG_PRINT("cache entry cache (%d) pkgs\r\n", entry->pkg_list.count);
    ARP_DBG_PRINT("-----\r\n");

    return 0;
}
static arp_entry_t *arp_cache_first(arp_cache_table_t *table)
{
    list_t *entry_list = &table->entry_list;
    arp_entry_t *first_entry = list_node_parent(entry_list->first, arp_entry_t, node);
    return first_entry;
}
static arp_entry_t *arp_cache_next(arp_entry_t *entry)
{
    list_node_t *cur = &entry->node;
    list_node_t *next = cur->next;
    arp_entry_t *ret = list_node_parent(next, arp_entry_t, node);
    return ret;
}
static int arp_is_ok(arp_parse_t *parse)
{
    if (parse->hard_type != ARP_HARD_TYPE_ETHER)
    {
        return 0;
    }
    if (parse->protocal_type != ARP_PROTOCAL_TYPE_IPV4)
    {
        return 0;
    }
    if (parse->hard_len != MACADDR_ARRAY_LEN)
    {
        return 0;
    }
    if (parse->protocal_len != IPADDR_ARRY_LEN)
    {
        return 0;
    }
    return 1;
}
static void arp_entry_send_all(arp_entry_t *entry)
{
    // 释放pkg缓存
    list_node_t *cur = entry->pkg_list.first;
    while (cur)
    {
        list_node_t *next = cur->next;
        list_remove(&entry->pkg_list, cur);
        pkg_t *pkg = list_node_parent(cur, pkg_t, node);
        ether_raw_out(entry->netif, PROTOCAL_TYPE_IPV4, &entry->hwaddr, pkg);
        cur = next;
    }
}
void arp_show_cache_list(void)
{
#ifdef ARP_DBG
    list_t *list = &arp_cache_table.entry_list;
    list_node_t *cur = list->first;

    ARP_DBG_PRINT("++++++++++++++++++++++++++show arp cache list++++++++++++++++\r\n");
    ARP_DBG_PRINT("list count is:%d\r\n", list->count);
    while (cur)
    {
        arp_entry_t *entry = list_node_parent(cur, arp_entry_t, node);
        arp_show_cache_entry(entry);
        cur = cur->next;
    }
    ARP_DBG_PRINT("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
#endif
}
void arp_cache_table_init(arp_cache_table_t *table)
{
    memset(table, 0, sizeof(arp_cache_table_t));
    list_init(&table->entry_list);
    mempool_init(&table->entry_pool, table->entry_buff, ARP_ENTRY_MAX_SIZE, sizeof(arp_entry_t));
}

int arp_cache_entry_cnt(arp_cache_table_t *table)
{
    return table->entry_list.count;
}

arp_entry_t *arp_cache_alloc_entry(arp_cache_table_t *table, int force)
{
    if (!table)
    {
        dbg_error("param false\r\n");
        return NULL;
    }
    arp_entry_t *entry = mempool_alloc_blk(&table->entry_pool, -1);
    if (!entry)
    {
        if (force)
        {
            // 如果必须要分配,把最后那个移除掉
            list_node_t *lastnode = list_remove(&table->entry_list, table->entry_list.last);
            arp_entry_t *new_entry = list_node_parent(lastnode, arp_entry_t, node);
            memset(new_entry, 0, sizeof(arp_entry_t));
            return new_entry;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        memset(entry, 0, sizeof(arp_entry_t));
        return entry;
    }
    return NULL;
}
void arp_cache_free_entry(arp_cache_table_t *table, arp_entry_t *entry)
{
    if (!table || !entry)
    {
        dbg_error("param false\r\n");
        return;
    }
    // 释放pkg缓存
    list_node_t *cur = entry->pkg_list.first;
    while (cur)
    {
        list_node_t *next = cur->next;
        list_remove(&entry->pkg_list, cur);
        pkg_t *pkg = list_node_parent(cur, pkg_t, node);
        memset(pkg, 0, sizeof(pkg_t));
        package_collect(pkg);
        entry->netif->send_fail_cnt++;
        cur = next;
    }
    // 释放entry
    mempool_free_blk(&table->entry_pool, entry);
}
/**清空arp缓存 */
void arp_cache_clear(arp_cache_table_t *table)
{
    arp_entry_t *cur = arp_cache_first(table);
    while (cur)
    {
        arp_entry_t *next = arp_cache_next(cur);
        arp_cache_remove_entry(table, cur);
        arp_cache_free_entry(table, cur);
        cur = next;
    }
}
arp_entry_t *arp_cache_remove_entry(arp_cache_table_t *table, arp_entry_t *entry)
{
    if (!table || !entry)
    {
        dbg_error("param false\r\n");
        return NULL;
    }

    list_node_t *rm_node = list_remove(&table->entry_list, &entry->node);
    arp_entry_t *ret;
    if (rm_node)
    {
        ret = list_node_parent(rm_node, arp_entry_t, node);
        return ret;
    }
    else
    {
        return NULL;
    }
}
int arp_cache_insert_entry(arp_cache_table_t *table, arp_entry_t *entry)
{
    if (!table || !entry)
    {
        dbg_error("param false\r\n");
        return -1;
    }

    list_insert_first(&table->entry_list, &entry->node);
    return 0;
}
arp_entry_t *arp_cache_find(arp_cache_table_t *table, ipaddr_t *ip)
{
    arp_entry_t *ret;
    list_node_t *cur = table->entry_list.first;
    while (cur)
    {
        arp_entry_t *cur_e = list_node_parent(cur, arp_entry_t, node);
        if (cur_e->ipaddr.q_addr == ip->q_addr)
        {
            ret = cur_e;
            arp_cache_remove_entry(table, cur_e);
            arp_cache_insert_entry(table, cur_e);
            return ret;
        }
        cur = cur->next;
    }
    return NULL;
}

int arp_entry_insert_pkg(arp_entry_t *entry, pkg_t *pkg)
{
    if (!entry || !pkg)
    {
        dbg_error("param false\r\n");
        return -1;
    }
    if (entry->pkg_list.count < ARP_ENTRY_PKG_CACHE_MAX_NR)
    {
        list_insert_last(&entry->pkg_list, &pkg->node);
        return 0;
    }
    else
    {
        return -2;
    }
}

arp_entry_t* arp_cache_add_entry(netif_t *netif, ipaddr_t *ip, hwaddr_t *hwaddr, int force)
{

    arp_entry_t *new_entry;
    // 创建该表项
    new_entry = arp_cache_alloc_entry(&arp_cache_table, force);
    if (new_entry)
    {
        new_entry->ipaddr.q_addr = ip->q_addr;
        new_entry->netif = netif;
        new_entry->retry = ARP_ENTRY_RETRY;
        if (hwaddr)
        {
            new_entry->state = ARP_ENTRY_STATE_RESOLVED;
            memcpy(new_entry->hwaddr.addr, hwaddr->addr, MACADDR_ARRAY_LEN);
            new_entry->tmo = ARP_ENTRY_TMO_STABLE;
        }
        else
        {
            new_entry->state = ARP_ENTRY_STATE_WAITING;
            new_entry->tmo = ARP_ENTRY_TMO_RESOLVING;
        }
        arp_cache_insert_entry(&arp_cache_table, new_entry);
        return new_entry;
    }
    else
    {
        return NULL;
    }
}
pkg_t *arp_entry_remove_pkg(arp_entry_t *entry, pkg_t *pkg)
{
    if (!entry || !pkg)
    {
        dbg_error("param false\r\n");
        return NULL;
    }

    list_remove(&entry->pkg_list, &pkg->node);
    return pkg;
}

void arp_show_arp(arp_t *arp)
{
#ifdef ARP_DBG
    arp_parse_t parse;
    arp_parse_pkg(arp, &parse);
    ARP_DBG_PRINT("-----\r\n");
    ARP_DBG_PRINT("hard type:0x%04x\r\n", parse.hard_type);
    ARP_DBG_PRINT("proto type:0x%04x\r\n", parse.protocal_type);
    ARP_DBG_PRINT("hard len:%d\r\n", parse.hard_len);
    ARP_DBG_PRINT("proto len:%d\r\n", parse.protocal_len);
    ARP_DBG_PRINT("opcode:0x%04x\r\n", parse.opcode);
    ARP_DBG_PRINT("src mac:%s\r\n", parse.src_mac_str);
    ARP_DBG_PRINT("src ip:%s\r\n", parse.src_ip_str);
    ARP_DBG_PRINT("dest mac:%s\r\n", parse.dest_mac_str);
    ARP_DBG_PRINT("dest ip:%s\r\n", parse.dest_ip_str);
#endif
}
int arp_parse_pkg(arp_t *arp, arp_parse_t *parse)
{
    hwaddr_t mac_src, mac_dest;
    memcpy(mac_src.addr, arp->src_mac, MACADDR_ARRAY_LEN);
    memcpy(mac_dest.addr, arp->dest_mac, MACADDR_ARRAY_LEN);

    parse->hard_type = ntohs(arp->hard_type);
    parse->protocal_type = ntohs(arp->protocal_type);
    parse->hard_len = arp->hard_len;
    parse->protocal_len = arp->protocal_len;
    parse->opcode = ntohs(arp->opcode);
    mac_n2s(&mac_src, parse->src_mac_str);
    mac_n2s(&mac_dest, parse->dest_mac_str);

    parse->src_ip.q_addr = ntohl(arp->src_ip);
    parse->dest_ip.q_addr = ntohl(arp->dest_ip);

    ipaddr_n2s(&parse->src_ip, parse->src_ip_str, 20);
    ipaddr_n2s(&parse->dest_ip, parse->dest_ip_str, 20);
    memcpy(parse->dest_mac.addr,arp->dest_mac,MACADDR_ARRAY_LEN);
    memcpy(parse->src_mac.addr,arp->src_mac,MACADDR_ARRAY_LEN);
    return 0;
}
/**
 * 发送arp请求包
 */
int arp_send_request(netif_t *netif, const ipaddr_t *dest_ip)
{
    int ret;

    pkg_t *pkg = package_alloc(sizeof(arp_t));
    if (!pkg)
    {
        dbg_warning("package cache exhausted\r\n");
        return -1;
    }
    arp_t *arp = (arp_t *)package_data(pkg, sizeof(arp_t), 0);
    arp->hard_type = htons(ARP_HARD_TYPE_ETHER);
    arp->protocal_type = htons(ARP_PROTOCAL_TYPE_IPV4);
    arp->hard_len = MACADDR_ARRAY_LEN;
    arp->protocal_len = IPADDR_ARRY_LEN;
    arp->opcode = htons(ARP_OPCODE_REQUEST);
    memcpy(arp->src_mac, netif->hwaddr.addr, MACADDR_ARRAY_LEN);
    arp->src_ip = htonl(netif->info.ipaddr.q_addr);
    memcpy(arp->dest_mac, get_mac_empty()->addr, MACADDR_ARRAY_LEN);
    arp->dest_ip = htonl(dest_ip->q_addr);

    // arp_show_arp(arp);
    ret = ether_raw_out(netif, PROTOCAL_TYPE_ARP, get_mac_broadcast(), pkg);
    if (ret < 0)
    {

        package_collect(pkg);
        return -2;
    }
    return 0;
}
/*发送免费包*/
int arp_send_free(netif_t *netif)
{
    return arp_send_request(netif, &netif->info.ipaddr);
}
int arp_send_reply(netif_t *netif, pkg_t *package)
{
    hwaddr_t dest_hw;
    arp_t *arp = package_data(package, sizeof(arp_t), 0);
    arp->opcode = htons(ARP_OPCODE_REPLY);
    memcpy(arp->dest_mac, arp->src_mac, MACADDR_ARRAY_LEN);
    arp->dest_ip = arp->src_ip;
    memcpy(arp->src_mac, netif->hwaddr.addr, MACADDR_ARRAY_LEN);
    arp->src_ip = htonl(netif->info.ipaddr.q_addr);

    memcpy(dest_hw.addr, arp->dest_mac, MACADDR_ARRAY_LEN);
    return ether_raw_out(netif, PROTOCAL_TYPE_ARP, &dest_hw, package);
}

int arp_in(netif_t *netif, pkg_t *package)
{
    int ret;
    arp_parse_t parse;
    arp_entry_t *entry;
    hwaddr_t hw;
    arp_t *arp = package_data(package, sizeof(arp_t), 0);
    // arp_show_arp(arp);
    arp_parse_pkg(arp, &parse);
    if (!arp_is_ok(&parse))
    {
        dbg_warning("recv a wrong format arp pkg\r\n");
        return -1;
    }

    switch (parse.opcode)
    {
    case ARP_OPCODE_REQUEST:
        // arp请求包都是广播包，如果确实问的是我的ip对应的mac地址，就发送回应包
        if (parse.dest_ip.q_addr == netif->info.ipaddr.q_addr)
        {
            ret = arp_send_reply(netif, package); //发送完成释放掉了
            if (ret < 0)
                return -1;
        }
        else
        {
            // 问的不是我的，但是包我也接收到了
            return -2;
        }
        entry = arp_cache_find(&arp_cache_table, &parse.src_ip);
        if (!entry)
        {
            // 添加新页表
            memcpy(hw.addr, parse.src_mac.addr, MACADDR_ARRAY_LEN);
            arp_cache_add_entry(netif, &parse.src_ip, &hw, 0);
            
        }
        else
        {
            if (entry->state == ARP_ENTRY_STATE_WAITING)
            {
                entry->state = ARP_ENTRY_STATE_RESOLVED;
                memcpy(entry->hwaddr.addr, arp->src_mac, MACADDR_ARRAY_LEN);
                arp_entry_send_all(entry);
            }
        }

        break;
    case ARP_OPCODE_REPLY: // 这个不是广播包
        entry = arp_cache_find(&arp_cache_table, &parse.src_ip);
        if (!entry)
        {
            // 添加新表项
            memcpy(hw.addr, arp->src_mac, MACADDR_ARRAY_LEN);
            arp_cache_add_entry(netif, &parse.src_ip, &hw, 1);
        }
        else
        {
            if (entry->state == ARP_ENTRY_STATE_WAITING)
            {
                entry->state = ARP_ENTRY_STATE_RESOLVED;
                entry->tmo = ARP_ENTRY_TMO_STABLE;
                memcpy(entry->hwaddr.addr, arp->src_mac, MACADDR_ARRAY_LEN);
                arp_entry_send_all(entry);
            }
        }
        break;
    default:
        dbg_error("unkown arp opcode type\r\n");
        return -2;
        break;
    }

    package_collect(package); // 成功处理后，回收
    return 0;
}

int arp_reslove(netif_t *netif, ipaddr_t *dest_ip, pkg_t *package)
{
    int ret;
    arp_entry_t *entry = arp_cache_find(&arp_cache_table, dest_ip);
    if (entry)
    {
        if (entry->state == ARP_ENTRY_STATE_RESOLVED)
        {
            return ether_raw_out(entry->netif, PROTOCAL_TYPE_IPV4, &entry->hwaddr, package);
        }
        else
        {

            return arp_entry_insert_pkg(entry, package);
        }
    }
    else
    {
        // 创建该表项
        arp_entry_t *new_entry = arp_cache_add_entry(netif, dest_ip, NULL, 1);
        //把包放进去
        arp_entry_insert_pkg(new_entry,package);
        // 发送arp请求
        arp_send_request(netif, dest_ip);
    }
    return 0;
}

void arp_cache_scan_period(void *arg)
{
    list_node_t *cur = arp_cache_table.entry_list.first;
    while (cur)
    {
        list_node_t *next = cur->next;
        arp_entry_t *entry = list_node_parent(cur, arp_entry_t, node);
        if (--entry->tmo == 0)
        {
            switch (entry->state)
            {
            case ARP_ENTRY_STATE_RESOLVED:
                ipaddr_t dest_ip;
                dest_ip.q_addr = entry->ipaddr.q_addr;
                entry->tmo = ARP_ENTRY_TMO_RESOLVING;
                entry->retry = ARP_ENTRY_RETRY;
                entry->state = ARP_ENTRY_STATE_WAITING;
                // 重新请求mac地址
                arp_send_request(entry->netif, &dest_ip);
                break;
            case ARP_ENTRY_STATE_WAITING:
                if (--entry->retry == 0)
                {
                    arp_cache_remove_entry(&arp_cache_table, entry);
                    arp_cache_free_entry(&arp_cache_table, entry);
                }
                else
                {
                    ipaddr_t dest_ip;
                    dest_ip.q_addr = entry->ipaddr.q_addr;
                    entry->tmo = ARP_ENTRY_TMO_RESOLVING;
                    entry->state = ARP_ENTRY_STATE_WAITING;
                    // 重新请求mac地址
                    arp_send_request(entry->netif, &dest_ip);
                }
                break;
            default:
                dbg_error("unkown entry state\r\n");
                break;
            }
        }

        cur = next;
    }
}
soft_timer_t arp_timer ;
void arp_init(void)
{
    arp_cache_table_init(&arp_cache_table);
    
    soft_timer_add(&arp_timer, SOFT_TIMER_TYPE_PERIOD, 1000, "arp timer", arp_cache_scan_period, NULL, NULL);
    
}
