#ifndef __ARP_H
#define __ARP_H
#include "printk.h"
#include "net_cfg.h"
#include "ipaddr.h"
#include "net_tools/package.h"
#include "netif.h"
#include "net_tools/net_mmpool.h"
#include "net_tools/soft_timer.h"


#define ARP_HARD_TYPE_ETHER 1
#define ARP_PROTOCAL_TYPE_IPV4  0x0800
#define ARP_OPCODE_REQUEST  1
#define ARP_OPCODE_REPLY    2
typedef struct _arp_entry_t
{
    ipaddr_t ipaddr;
    hwaddr_t hwaddr;

    enum{
        ARP_ENTRY_STATE_NONE,
        ARP_ENTRY_STATE_WAITING, //正在查询ip对应的mac地址
        ARP_ENTRY_STATE_RESOLVED //查到了
    }state;

    netif_t* netif;
    list_t pkg_list;

    int tmo;   //每隔多久更新一次arp表
    int retry; //处于waiting的时间最长维持多久

    list_node_t node;
}arp_entry_t;

typedef struct _arp_cache_table_t
{
    list_t entry_list;
    mempool_t entry_pool;//内存池，分配entry_t用的
    uint8_t entry_buff[ARP_ENTRY_MAX_SIZE*(sizeof(arp_entry_t)+sizeof(list_node_t))];
}arp_cache_table_t;

/*网络序，数据包中的东西统一使用网络序*/
#pragma pack(1)
typedef struct _arp_t
{
    uint16_t hard_type;
    uint16_t protocal_type;
    uint8_t hard_len;
    uint8_t protocal_len;
    uint16_t opcode;
    uint8_t src_mac[MACADDR_ARRAY_LEN];
    uint32_t src_ip;
    uint8_t dest_mac[MACADDR_ARRAY_LEN];
    uint32_t dest_ip;
}arp_t;
#pragma pack(0)
typedef struct _arp_parse_t
{
    uint16_t hard_type;
    uint16_t protocal_type;
    uint8_t hard_len;
    uint8_t protocal_len;
    uint16_t opcode;
    hwaddr_t src_mac;
    char src_mac_str[20];
    ipaddr_t src_ip;
    char src_ip_str[20];
    hwaddr_t dest_mac;
    char dest_mac_str[20];
    ipaddr_t dest_ip;
    char dest_ip_str[20];
}arp_parse_t;

extern soft_timer_t arp_timer ;
void arp_init(void);

/*arp cache table操作*/
extern arp_cache_table_t arp_cache_table;
int arp_cache_entry_cnt(arp_cache_table_t* table);
arp_entry_t* arp_cache_alloc_entry(arp_cache_table_t *table,int force);
void arp_cache_free_entry(arp_cache_table_t *table,arp_entry_t* entry);
void arp_cache_clear(arp_cache_table_t *table);
arp_entry_t* arp_cache_remove_entry(arp_cache_table_t *table,arp_entry_t* entry);
int arp_cache_insert_entry(arp_cache_table_t *table,arp_entry_t* entry);
arp_entry_t* arp_cache_add_entry(netif_t *netif, ipaddr_t *ip, hwaddr_t *hwaddr, int force);
arp_entry_t* arp_cache_find(arp_cache_table_t* table,ipaddr_t* ip);
int arp_entry_insert_pkg(arp_entry_t* entry,pkg_t* pkg);
pkg_t* arp_entry_remove_pkg(arp_entry_t* entry,pkg_t* pkg);

/*arp*/
int arp_parse_pkg(arp_t* arp,arp_parse_t* parse);

int arp_send_request(netif_t* netif, const ipaddr_t* dest_ip);
int arp_send_free(netif_t *netif);
int arp_send_reply(netif_t *netif, pkg_t *package);


int arp_in(netif_t* netif,pkg_t* package);
int arp_reslove(netif_t* netif,ipaddr_t* dest_ip,pkg_t* package);


/*dbg*/
void arp_show_cache_list(void);
void arp_show_arp(arp_t* arp);
#ifdef ARP_DBG
    #define ARP_DBG_PRINT(fmt, ...) dbg_info(fmt, ##__VA_ARGS__)
#else
    #define ARP_DBG_PRINT(fmt, ...) do { } while(0)
#endif

#endif
