#ifndef __SOCKET_PARAM_H
#define __SOCKET_PARAM_H
#include "sock.h"
typedef struct _sock_create_param_t
{
    int family;
    int type;
    int protocal;
} sock_create_param_t;

typedef struct _sock_sendto_param_t
{
    int sockfd;
    const uint8_t *buf;
    ssize_t buf_len;
    int flags;
    const struct sockaddr *addr;
    socklen_t *addr_len;
} sock_sendto_param_t;

typedef struct _sock_recvfrom_param_t
{
    int sockfd;
    uint8_t *buf;
    ssize_t buf_len;
    int flags;
    struct sockaddr *addr;
    socklen_t *addr_len;
} sock_recvfrom_param_t;

typedef struct _sock_setsockopt_param_t
{
    int sockfd;
    int level;
    int optname;
    const char *optval;
    int optlen;
} sock_setsockopt_param_t;

typedef struct _sock_connect_param_t
{
    int sockfd;
    const struct sockaddr *addr;
    socklen_t len;
} sock_connect_param_t;

typedef struct _sock_send_param_t
{
    int sockfd;
    const void *buf;
    ssize_t len;
    int flags;
} sock_send_param_t;

typedef struct _sock_recv_param_t
{
    int sockfd;
    void *buf;
    ssize_t len;
    int flags;
} sock_recv_param_t;

typedef struct _sock_bind_param_t
{
    int sockfd;
    const struct sockaddr *addr;
    socklen_t addrlen;
} sock_bind_param_t;
typedef struct _sock_listen_param_t
{
    int sockfd;
    int backlog;
} sock_listen_param_t;
typedef struct _sock_accept_param_t
{
    int sockfd;
    struct sockaddr *addr;
    socklen_t *len;
} sock_accept_param_t;
#endif