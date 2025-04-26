#ifndef __SOCKET_H
#define __SOCKET_H
#include "types.h"
#include "net_cfg.h"
#include "_time.h"

#define INADDR_ANY 0

#define AF_INET 0      // IPv4协议簇
#define SOCK_RAW 1     // 原始数据报
#define SOCK_DGRAM 2   // 数据报式套接字
#define SOCK_STREAM 3  // 流式套接字
#define IPPROTO_ICMP 1 // ICMP协议
#define IPPROTO_UDP 17 // UDP协议
#define IPPROTO_TCP 6  // TCP协议
#define INADDR_ANY 0   // 任意IP地址，即全0的地址
#define SOL_SOCKET 0
#define SOL_TCP 1

// sock选项

#define SO_RCVTIMEO 1   // 设置用于阻止接收调用的超时（以毫秒为单位）
#define SO_SNDTIMEO 2   // 用于阻止发送调用的超时（以毫秒为单位）。
#define SO_KEEPALIVE 3  // Keepalive选项
#define TCP_KEEPIDLE 4  // Keepalive空闲时间
#define TCP_KEEPINTVL 5 // 超时间隔
#define TCP_KEEPCNT 6   // 重试的次数
#define SO_REUSEPORT 7 //端口重用，

typedef uint16_t port_t;
typedef int socklen_t;
typedef long ssize_t;
#pragma pack(1)

/**
 * @brief 专用于IPv4地址
 */
typedef struct in_addr
{
    union
    {
        struct
        {
            uint8_t addr0;
            uint8_t addr1;
            uint8_t addr2;
            uint8_t addr3;
        };
        uint8_t addr_array[IPADDR_ARRY_LEN];
        uint32_t s_addr;
    };
}in_addr_t;

/**
 * @brief 通用socket结构，与具体的协议簇无关
 */
struct sockaddr
{
    uint8_t sa_len;      // 整个结构的长度，值固定为16
    uint8_t sa_family;   // 地址簇：NET_AF_INET
    uint8_t sa_data[14]; // 数据空间
};

/**
 * @brief 用于存储ipv4的地址和端口
 */
struct sockaddr_in
{
    uint8_t sin_len;         // 整个结构的长度，值固定为16
    uint8_t sin_family;      // 地址簇：AF_INET
    uint16_t sin_port;       // 端口号
    struct in_addr sin_addr; // IP地址
    char sin_zero[8];        // 填充字节，用于ipv6
};
#pragma pack()



int sys_socket(int family,int type,int protocal);
int sys_connect(int sockfd,const struct  sockaddr* addr,  socklen_t len);
int sys_sendto(int sockfd,const void* buf,ssize_t buf_len,int flags,
                        const struct sockaddr* dest,socklen_t dest_len);
                        
int sys_recvfrom(int sockfd, void *buf, ssize_t len, int flags,
                 struct sockaddr *src, socklen_t *addr_len);
int sys_setsockopt(int sockfd,  int level, int optname, const char * optval, int optlen);
int sys_send(int sockfd, const void *buf, ssize_t len, int flags);
int sys_recv(int sockfd, void *buf, ssize_t len, int flags);
int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sys_closesocket(int sockfd);
int sys_listen(int sockfd,int backlog);
int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *len);
#endif