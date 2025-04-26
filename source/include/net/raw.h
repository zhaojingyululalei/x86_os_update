#ifndef __RAW_H
#define __RAW_H
#include "sock.h"
#include "net_tools/package.h"
#include "netif.h"
#define RAW_MAX_NR 50
#define RAW_DEFAULT_PROTOCAL    IPPROTO_ICMP
typedef struct _raw_t
{
    sock_t base;
    list_node_t node;
    netif_t* netif;
    list_t recv_list;
}raw_t;

void raw_init(void);
sock_t* raw_create(int family, int protocol);
int raw_in(netif_t *netif,pkg_t* package);

#endif