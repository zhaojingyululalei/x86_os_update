#include "net_tools/net_mmpool.h"
#include "net_msg.h"
#include "tools/list.h"
#include "net_tools/msgQ.h"
#include "string.h"
#include "netif.h"
static mempool_t net_msg_pool;
static uint8_t net_msg_buf[(sizeof(net_msg_t) + sizeof(list_node_t)) * NET_MSG_MAX_NR];

void net_msg_init(void)
{
    mempool_init(&net_msg_pool, net_msg_buf, NET_MSG_MAX_NR, sizeof(net_msg_t));
}

net_msg_t *net_msg_create(netif_t *netif, net_msg_type_t type, void *data)
{
    if (!data)
    {
        dbg_error("data is null\r\n");
        return NULL;
    }
    net_msg_t *message = (net_msg_t *)mempool_alloc_blk(&net_msg_pool, -1);
    if (!message)
    {
        // 暂时没有空闲消息块可供使用
        dbg_error("out of memory\r\n");
        return NULL;
    }
    switch (type)
    {
    case NET_MSG_TYPE_PKG:
        pkg_t *pkg = (pkg_t *)data;
        if (pkg->total <= 0)
        {
            return NULL; // 空包不往队列里面放
        }

        message->type = type;
        message->package = pkg;
        message->netif = netif;
        return message;

        break;
    case NET_MSG_TYPE_NET_FUNC:
        message->type = type;
        message->task = (net_app_task_t*)data;
        return message;
        break;

    default:
        dbg_error("unkown net msg type\r\n");
        return NULL;
        break;
    }
    return NULL;
}

void net_msg_free(net_msg_t *message)
{

    memset(message, 0, sizeof(net_msg_t));
    mempool_free_blk(&net_msg_pool, message);
}
