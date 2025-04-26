#ifndef __IPADDR_H
#define __IPADDR_H
#include "types.h"
#include "net_cfg.h"
/**
 * @brief IP地址
 */
typedef struct _ipaddr_t
{

    union
    {
        uint32_t q_addr;                 // 32位整体描述
        uint8_t a_addr[IPADDR_ARRY_LEN]; // 数组描述
    };
    enum
    {
        IPADDR_V4,
    } type; // 地址类型
} ipaddr_t;

typedef struct _hwaddr_t
{

    uint8_t addr[MACADDR_ARRAY_LEN];

} hwaddr_t;

int ipaddr_n2s(const ipaddr_t *ip_addr, char *ip_str, int str_len);
int ipaddr_s2n(const char *ip_str, ipaddr_t *ip_addr);
int mac_s2n( const char* mac_str,hwaddr_t* mac);
int mac_n2s(const hwaddr_t* mac, char* mac_str);
// 判断当前系统是否是小端,0xff在低字节还是高字节
#define IS_LITTLE_ENDIAN (*(uint16_t *)"\0\xff" > 0x100)
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

hwaddr_t* get_mac_broadcast(void);
hwaddr_t* get_mac_empty(void);

uint32_t ipaddr_get_host(ipaddr_t *ip,ipaddr_t *mask);
uint32_t ipaddr_get_net(ipaddr_t * ip,ipaddr_t *mask);

int is_local_boradcast(ipaddr_t* host_ip,ipaddr_t *host_mask, ipaddr_t *ip);
int is_global_boradcast(ipaddr_t *ip);
#endif