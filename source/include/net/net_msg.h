#ifndef __NET_MSG_H
#define __NET_MSG_H

#include "net_tools/package.h"
#include "netif.h"
#include "net_submit.h"
#define NET_MSG_MAX_NR 100
typedef enum _net_msg_type_t
{
    NET_MSG_TYPE_NONE,
    NET_MSG_TYPE_PKG,      // 消息类型是数据包
    NET_MSG_TYPE_NET_FUNC, // 消息类型是app请求
} net_msg_type_t;

typedef struct _net_msg_t
{
    netif_t* netif;
    net_msg_type_t type; // 消息类型

    union
    {
        pkg_t *package;
        net_app_task_t* task;
    };

} net_msg_t;

void net_msg_init(void);
net_msg_t *net_msg_create(netif_t* netif,net_msg_type_t type, void *data);
void net_msg_free(net_msg_t *message);
#endif