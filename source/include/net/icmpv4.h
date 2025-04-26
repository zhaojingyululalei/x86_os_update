#ifndef __ICMPV4_H
#define __ICMPV4_H
#include "net_cfg.h"
#include "types.h"
#include "net_tools/package.h"
#include "ipaddr.h"
#include "ipv4.h"
/**
 * @brief ICMP包类型
 */
typedef enum _icmp_type_t {
    ICMPv4_ECHO_REPLY = 0,                  // 回送响应
    ICMPv4_ECHO_REQUEST = 8,                // 回送请求
    ICMPv4_UNREACH = 3,                     // 不可达
}icmp_type_t;

/**
 * @brief ICMP包代码
 */
typedef enum _icmp_code_t {
    ICMPv4_ECHO = 0,                        // echo的响应码
    ICMPv4_UNREACH_PRO = 2,                 // 协议不可达
    ICMPv4_UNREACH_PORT = 3,                // 端口不可达
}icmp_code_t;


#pragma pack(1)
typedef struct _icmpv4_head_t {
    uint8_t type;           // 类型
    uint8_t code;			// 代码
    uint16_t checksum;	    // ICMP报文的校验和
}icmpv4_head_t;

/**
 * ICMP报文
 */
typedef struct _icmpv4_pkt_t {
    icmpv4_head_t head;            // 头和数据区
    union {
        uint32_t reverse;       // 不同应用 字段不同
        struct{
            uint16_t Identifier;
            uint16_t Sequence ;
        };
    };
}icmpv4_t;
#pragma pack()
int icmpv4_send_unreach(ipaddr_t *dest, pkg_t *package, int code);
int icmpv4_in(netif_t *netif,ipaddr_t* visitor_ip,ipaddr_t* host_ip,pkg_t* package,ipv4_header_t* ipv4_head);
int icmpv4_out(ipaddr_t* dest,pkg_t* package);
void icmpv4_init(void);

#ifdef ICMPV4_DBG
    #define ICMPV4_DBG_PRINT(fmt, ...) dbg_info(fmt, ##__VA_ARGS__)
#else
    #define ICMPV4_DBG_PRINT(fmt, ...) do { } while(0)
#endif
#endif