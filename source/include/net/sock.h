#ifndef __SOCK_H
#define __SOCK_H
#include "types.h"
#include "ipaddr.h"
#include "tools/list.h"
#include "socket.h"
#include "net_tools/net_threads.h"


struct _sock_t;
typedef struct _sock_ops_t
{   
    int (*close)(struct _sock_t* s);
	int (*sendto)(struct _sock_t * s, const void* buf, ssize_t len, int flags, 
                        const struct  sockaddr* dest,  socklen_t dest_len);
	int(*recvfrom)(struct _sock_t* s, void* buf, ssize_t len, int flags, 
                        struct  sockaddr* src,  socklen_t * addr_len);
	int (*setsockopt)(struct _sock_t* s,  int level, int optname, const char * optval, int optlen);
	int (*bind)(struct _sock_t* s, const struct  sockaddr* addr,  socklen_t len);
	int (*connect)(struct _sock_t* s, const struct  sockaddr* addr,  socklen_t len);
	int(*send)(struct _sock_t* s, const void* buf, ssize_t len, int flags);
	int(*recv)(struct _sock_t* s, void* buf, ssize_t len, int flags);
    int (*listen) (struct _sock_t *s, int backlog);
    int (*accept)(struct _sock_t *s, struct  sockaddr* addr,  socklen_t* len);
    void (*destroy)(struct _sock_t *s);
}sock_ops_t;

#define SOCK_SEND_WAIT  (1<<0)
#define SOCK_RECV_WAIT  (1<<1)
#define SOCK_CONN_WAIT  (1<<2)
#define SOCK_ALL_WAIT   (SOCK_SEND_WAIT|SOCK_RECV_WAIT|SOCK_CONN_WAIT)
typedef struct _sock_wait_t
{
    
    semaphore_t sem;
    
    int tmo;  //单位ms   0阻塞  >0超时唤醒
    net_err_t err;
}sock_wait_t;

void sock_wait_init(sock_wait_t* wait);
int sock_wait_set(struct _sock_t* sock,int tmo,int flag);
int sock_wait_enter(sock_wait_t* wait);
void sock_wait_notify(sock_wait_t* wait,net_err_t err);
void sock_wait_destory(sock_wait_t *wait);
typedef struct _sock_t
{
    port_t host_port;
    port_t target_port; 
    ipaddr_t host_ip;
    ipaddr_t target_ip;

    int family;
    int protocal;
    int type;

    int wait_flag;
    sock_wait_t send_wait;
    sock_wait_t recv_wait;
    sock_wait_t conn_wait;

    sock_ops_t* ops;

    list_node_t node;

}sock_t;
typedef struct _socket_t
{
    enum{
        SOCKET_STATE_NONE,
        SOCKET_STATE_PRESSION
    }state;

    sock_t* sock;
}socket_t;
int socket_get_index(socket_t *s);
socket_t *socket_from_index(int idx);


void socket_init(void);
void sock_init(sock_t* sock,int family,int protocal,const sock_ops_t* ops);
int sock_create(void* arg);
int sock_sendto(void* arg);
int sock_recvfrom(void* arg);
int sock_setsockopt(void* arg);
int sock_closesocket(void* arg);
int sock_connect(void* arg);
int sock_send(void* arg);
int sock_recv(void* arg);
int sock_bind(void*arg);
int sock_listen(void* arg);
int sock_accept(void* arg);
#endif