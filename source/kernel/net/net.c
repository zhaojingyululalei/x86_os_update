#include "net_msg_queue.h"
#include "net_msg.h"
#include "net_tools/package.h"
#include "net_worker.h"
#include "netif.h"
#include "arp.h"
#include "net_tools/soft_timer.h"
#include "ipv4.h"
#include "icmpv4.h"
#include "net_submit.h"
#include "sock.h"
#include "raw.h"
#include  "udp.h"
#include "tcp.h"
void net_init(void)
{
    //数据包分配结构
    package_pool_init();
    soft_timer_init();
    net_submit_init();
    //核心的消息队列 以及消息分配结构
    net_mq_init();
    net_msg_init();
    netif_init();
    net_worker_init();
    arp_init();
    ipv4_init();
    icmpv4_init();
    socket_init();
    raw_init();
    udp_init();
    tcp_init();
}