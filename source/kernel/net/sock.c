#include "sock.h"
#include "printk.h"
#include "string.h"
#include "socket_param.h"
#include "socket.h"
#include "raw.h"
#include "udp.h"
#include "tcp.h"

#define SOCKET_MAX_NR    (TCP_BUF_MAX_NR + UDP_BUF_MAX_NR + RAW_MAX_NR)
/*工作线程所要执行的函数*/

static socket_t socket_tbl[SOCKET_MAX_NR] = {0};

int socket_get_index(socket_t *s)
{
    return (int)(s - socket_tbl);
}
socket_t *socket_from_index(int idx)
{
    if (idx < 0 || idx > SOCKET_MAX_NR)
    {
        dbg_error("socket out of boundry\r\n");
    }
    return socket_tbl + idx;
}
static socket_t *socket_alloc(void)
{
    socket_t *ret = NULL;
    for (int i = 0; i < SOCKET_MAX_NR; i++)
    {
        if (socket_tbl[i].state == SOCKET_STATE_NONE)
        {
            ret = &socket_tbl[i];
            break;
        }
    }
    ret->state = SOCKET_STATE_PRESSION;
    return ret;
}
static void socket_free(socket_t *s)
{
    s->state == SOCKET_STATE_NONE;
    s->sock = NULL;
}
void socket_init(void)
{
    memset(socket_tbl, 0, sizeof(socket_t) * SOCKET_MAX_NR);
}

void sock_init(sock_t *sock, int family, int protocal, const sock_ops_t *ops)
{
    memset(sock, 0, sizeof(sock_t));
    sock->family = family;
    sock->protocal = protocal;
    sock->ops = ops;
    sock_wait_init(&sock->send_wait);
    sock_wait_init(&sock->recv_wait);
    sock_wait_init(&sock->conn_wait);
}
static const struct
{
    sock_t *(*create)(int family, int protocol);
} ops_create_tbl[] = {
    [SOCK_RAW] = {.create = raw_create},
    [SOCK_DGRAM] = {.create = udp_create},
    [SOCK_STREAM] = {.create = tcp_create}
};

int sock_create(void *arg)
{
    sock_create_param_t *param = (sock_create_param_t *)arg;
    if (param->family != AF_INET)
    {
        dbg_error("socket param fault:family is not AF_INET\r\n");
        return -1;
    }
    socket_t *s = socket_alloc();
    s->sock->type = param->type;
    if (!s)
    {
        dbg_error("socket pool out of memory\r\n");
        return -1;
    }

    // 根据不同的类型，调用不同的create函数，RAW DGRAM STREAM类型
    sock_t *sock = ops_create_tbl[param->type].create(param->family, param->protocal);
    if (!sock)
    {
        socket_free(s);
        return -1;
    }

    s->sock = sock;
    return socket_get_index(s);
}

int sock_sendto(void *arg)
{
    sock_sendto_param_t *param = (sock_sendto_param_t *)arg;
    if (!param->addr || param->addr_len != sizeof(struct sockaddr) || param->sockfd < 0)
    {
        return -1;
    }

    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    return sock->ops->sendto(sock, param->buf, param->buf_len, param->flags, param->addr, param->addr_len);
}

int sock_recvfrom(void *arg)
{
    sock_recvfrom_param_t *param = (sock_recvfrom_param_t *)arg;

    if (param->sockfd < 0)
    {
        dbg_error("sockfd wrong\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    return sock->ops->recvfrom(sock, param->buf, param->buf_len, param->flags, param->addr, param->addr_len);
}

int sock_setsockopt(void *arg)
{
    sock_setsockopt_param_t *param = (sock_setsockopt_param_t *)arg;
    if (param->level != SOL_SOCKET)
    {
        dbg_error("unkown sock opt level\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    return sock->ops->setsockopt(sock, param->level, param->optname, param->optval, param->optlen);
}

int sock_closesocket(void *arg)
{
    int ret;
    int sockfd = (int)arg;
    socket_t *socket = socket_from_index(sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }
    ret = sock->ops->close(sock); // 释放对应类型的所有资源，例如raw的所有资源
    socket_free(socket);          // 回收socket
    return ret;
}

int sock_connect(void *arg)
{
    sock_connect_param_t *param = (sock_connect_param_t *)arg;
    if (param->sockfd < 0)
    {
        dbg_error("unkown sock opt level\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }
    return sock->ops->connect(sock, param->addr, param->len);
}
int sock_send(void *arg)
{
    sock_send_param_t *param = (sock_send_param_t *)arg;
    if (param->sockfd < 0)
    {
        dbg_error("unkown sock opt level\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }
    return sock->ops->send(sock, param->buf, param->len, param->flags);
}
int sock_recv(void *arg)
{
    sock_recv_param_t *param = (sock_recv_param_t *)arg;

    if (param->sockfd < 0)
    {
        dbg_error("sockfd wrong\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    return sock->ops->recv(sock, param->buf, param->len, param->flags);
}

int sock_bind(void*arg)
{
    sock_bind_param_t* param = (sock_bind_param_t *)arg;
    if (param->sockfd < 0)
    {
        dbg_error("sockfd wrong\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    return sock->ops->bind(sock,param->addr,param->addrlen);
}

int sock_listen(void* arg){
    sock_listen_param_t* param = (sock_listen_param_t *)arg;
    if (param->sockfd < 0)
    {
        dbg_error("sockfd wrong\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    return sock->ops->listen(sock,param->backlog);
}

int sock_accept(void* arg){
    sock_accept_param_t* param = (sock_accept_param_t *)arg;
    if (param->sockfd < 0)
    {
        dbg_error("sockfd wrong\r\n");
        return -1;
    }
    socket_t *socket = socket_from_index(param->sockfd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    return sock->ops->accept(sock,param->addr,param->len);
}
void sock_wait_init(sock_wait_t *wait)
{

    semaphore_init(&wait->sem, 0);
    wait->tmo = 0;
    wait->err =NET_ERR_OK;
}
int sock_wait_set(sock_t *sock, int tmo, int flag)
{
    sock->wait_flag |= flag;
    switch (flag)
    {
    case SOCK_RECV_WAIT:
        sock->recv_wait.tmo = tmo;
        break;
    case SOCK_SEND_WAIT:
        sock->send_wait.tmo = tmo;
        break;
    case SOCK_CONN_WAIT:
        sock->conn_wait.tmo = tmo;
        break;
    default:
        dbg_error("unkown sock wait type\r\n");
        return -1;
    }

    return 0;
}
int sock_wait_enter(sock_wait_t *wait)
{
    int ret;
    ret = time_wait(&wait->sem, wait->tmo);
    if(ret < 0)
    {
        if(ret == -3){
            wait->err = NET_ERR_TMO;
        }
    }
    return wait->err;
}
/*是因为啥唤醒的*/
void sock_wait_notify(sock_wait_t *wait,net_err_t err)
{
    wait->err = err;
    semaphore_post(&wait->sem);
}
void sock_wait_destory(sock_wait_t *wait){
    semaphore_destory(&wait->sem);
}