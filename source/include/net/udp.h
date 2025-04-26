#ifndef __UDP_H
#define __UDP_H
#include "sock.h"
#include "net_tools/package.h"

#define UDP_BUF_MAX_NR  50
#define UDP_DEFAULT_PROTOCAL    IPPROTO_UDP
typedef struct _udp_data_t
{
    ipaddr_t from_ip;
    port_t from_port;
    pkg_t* package;

    list_node_t node;
}udp_data_t;

typedef struct _udp_t
{
    sock_t base;

    list_t recv_list; //udp_data_t
    list_node_t node;
}udp_t;

#pragma pack(1)
typedef struct _udp_head_t
{
    port_t src_port;
    port_t dest_port;
    uint16_t total_len;
    uint16_t checksum;
}udp_head_t;
#pragma pack(0)
typedef struct _udp_parse_t
{
    port_t src_port;
    port_t dest_port;
    uint16_t total_len;
    uint16_t checksum;
}udp_parse_t;
sock_t* udp_create(int family, int protocol);
void udp_init(void);
int udp_out(port_t src_port,port_t dest_port,ipaddr_t* dest_ip,pkg_t* package);
int udp_in(pkg_t* package,ipaddr_t* src_ip,ipaddr_t* dest_ip);
#endif