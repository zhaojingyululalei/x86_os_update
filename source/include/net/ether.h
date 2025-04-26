#ifndef __ETHER_H
#define __ETHER_H

#include "netif.h"
#include "protocal.h"
#include "ipaddr.h"
#include "net_tools/package.h"
#pragma pack(1)
typedef struct _ether_header_t
{
    uint8_t dest[MACADDR_ARRAY_LEN];
    uint8_t src[MACADDR_ARRAY_LEN];
    uint16_t protocal;
}ether_header_t;

typedef struct _ether_pkg_t
{
    ether_header_t head;
    uint8_t data; //数据区46~1500字节，这里只取第一个字节，方便得到数据首地址
}ether_pkg_t;
#pragma pack()

int ether_raw_out(netif_t *netif, protocal_type_t type, const hwaddr_t* mac_dest, pkg_t *pkg);
extern const link_layer_t ether_ops;
#ifdef ETHER_DBG
    #define ETHER_DBG_PRINT(fmt, ...) dbg_info(fmt, ##__VA_ARGS__)
#else
    #define ETHER_DBG_PRINT(fmt, ...) do { } while(0)
#endif

#endif