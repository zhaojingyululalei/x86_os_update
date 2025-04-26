#include "tcp.h"
#include "printk.h"
#include "net_cfg.h"
/*hash表里存的都是成功建立连接的tcp，不成功的别存*/
static tcp_hash_entry_t tcp_hash_tbl[TCP_HASH_SIZE]={0};

// Jenkins 哈希函数（用于计算四元组哈希值）
uint32_t jenkins_hash(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint32_t hash = 0;
    hash += a;
    hash += hash << 10;
    hash ^= hash >> 6;
    hash += b;
    hash += hash << 10;
    hash ^= hash >> 6;
    hash += c;
    hash += hash << 10;
    hash ^= hash >> 6;
    hash += d;
    hash += hash << 10;
    hash ^= hash >> 6;
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash % TCP_HASH_SIZE;
}
// 查找 TCP 连接
tcp_t *find_tcp_connection(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port)
{
    uint32_t hash_index = jenkins_hash(local_ip, local_port, remote_ip, remote_port);
    tcp_hash_entry_t *entry = &tcp_hash_tbl[hash_index];
    list_t *value = &entry->value_list;
    list_node_t *cur = value->first;
    while (cur)
    {
        tcp_t *tcp = list_node_parent(cur, tcp_t, hash_node);
        if (tcp->base.host_ip.q_addr == local_ip &&
            tcp->base.host_port == local_port &&
            tcp->base.target_ip.q_addr == remote_ip &&
            tcp->base.target_port == remote_port)
        {
            return tcp; // 找到了
        }
        cur = cur->next;
    }
    return NULL; // 没找到
}

// 插入新的 TCP 连接
void hash_insert_tcp_connection(tcp_t *tcp)
{
    uint32_t local_ip = tcp->base.host_ip.q_addr;
    uint16_t local_port = tcp->base.host_port;
    uint32_t remote_ip = tcp->base.target_ip.q_addr;
    uint16_t remote_port = tcp->base.target_port;
    if (find_tcp_connection(local_ip, local_port, remote_ip, remote_port))
    {
        dbg_warning("连接已存在，插入失败！\n");
        return;
    }
    uint32_t hash_index = jenkins_hash(local_ip, local_port, remote_ip, remote_port);
    list_t *list = &(tcp_hash_tbl[hash_index].value_list);
    list_insert_first(list, &tcp->hash_node);
}

// 删除 TCP 连接
void hash_delete_tcp_connection(tcp_t *tcp)
{
    uint32_t local_ip = tcp->base.host_ip.q_addr;
    uint16_t local_port = tcp->base.host_port;
    uint32_t remote_ip = tcp->base.target_ip.q_addr;
    uint16_t remote_port = tcp->base.target_port;
    uint32_t hash_index = jenkins_hash(local_ip, local_port, remote_ip, remote_port);
    list_t *list = &(tcp_hash_tbl[hash_index].value_list);
    list_node_t* cur = list->first;
    while (cur)
    {
        tcp_t *tcp = list_node_parent(cur, tcp_t, hash_node);
        if (tcp->base.host_ip.q_addr == local_ip &&
            tcp->base.host_port == local_port &&
            tcp->base.target_ip.q_addr == remote_ip &&
            tcp->base.target_port == remote_port)
        {
            list_remove(list,&tcp->hash_node);
            return;
        }
        cur = cur->next;
    }
    
    //dbg_warning("未找到连接，删除失败。\n");
    return ;
}

// 打印整个哈希表
void tcp_hash_table_print() {
#ifdef TCP_HASH_TBL_DBG
    for (int i = 0; i < TCP_HASH_SIZE; i++) {
        list_t *list = &(tcp_hash_tbl[i].value_list);
        list_node_t* cur = list->first;
        dbg_info("index %d:******************\r\n",i);
        while (cur)
        {
            tcp_t *tcp = list_node_parent(cur, tcp_t, hash_node);
            char ip_buf[20]= {0};
            ipaddr_n2s(&tcp->base.host_ip,ip_buf,20);
            dbg_info("local ip:%s\r\n",ip_buf);

            memset(ip_buf,0,20);

            ipaddr_n2s(&tcp->base.target_ip,ip_buf,20);
            dbg_info("remote ip:%s\r\n",ip_buf);

            dbg_info("local port:%d\r\n",tcp->base.host_port);
            dbg_info("remote port:%d\r\n",tcp->base.target_port);
            cur =cur->next;
        }
        
    }
#endif
}