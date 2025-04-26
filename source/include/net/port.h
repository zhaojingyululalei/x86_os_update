#ifndef __PORT_H
#define __PORT_H
#include "socket.h"
typedef enum _port_state_t
{
    PROT_STATE_FREE,
    PROT_STATE_USED,
}prot_state_t;

typedef struct _net_port_t
{
    prot_state_t state;
}net_port_t;

extern bool mul_port;
port_t net_port_dynamic_alloc(void);
int net_port_free(port_t port);
int net_port_use(port_t port);
void net_port_init(void);
int net_port_is_free(port_t port);
int net_port_is_used(port_t port);
#endif