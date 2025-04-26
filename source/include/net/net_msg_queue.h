#ifndef _NET_MSG_QUEUE_H
#define _NET_MSG_QUEUE_H

#include "net_msg.h"
#define NET_MQ_CAPACITY 100


void net_mq_init(void);

int net_msg_enqueue(net_msg_t* message,int timeout);

net_msg_t* net_msg_dequeue(int timeout);
#endif