#include "net_msg_queue.h"

#include "net_tools/msgQ.h"
#include "net_msg.h"

static net_msg_t* net_mq_buf[NET_MQ_CAPACITY];
static msgQ_t net_mq;

void net_mq_init(void)
{
    msgQ_init(&net_mq,(void**)net_mq_buf,NET_MQ_CAPACITY);
}

int net_msg_enqueue(net_msg_t* message,int timeout)
{
    return msgQ_enqueue(&net_mq,message,timeout);
}

net_msg_t* net_msg_dequeue(int timeout)
{
    return (net_msg_t*)msgQ_dequeue(&net_mq,timeout);
}