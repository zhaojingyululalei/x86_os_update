#ifndef __NET_SYSCALL_H
#define __NET_SYSCALL_H
#include "netif.h"


int sys_netif_create(const char *if_name, netif_type_t type);
int sys_netif_open(const char *if_name);
int sys_netif_active(const char *if_name);
int sys_netif_deactive(const char *if_name);
int sys_netif_close(const char *if_name);
int sys_netif_set_ip(const char* if_name,const char* ip_str);
int sys_netif_set_gateway(const char* if_name,const char* ip_str);
int sys_netif_set_mask(const char* if_name,const char* ip_str);
void sys_netif_show(void);




uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);
int inet_pton(int family, const char *strptr, void *addrptr);
const char * inet_ntop(int family, const void *addrptr, char *strptr, int len);
#endif
