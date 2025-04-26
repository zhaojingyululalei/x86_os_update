
#include "net/netif.h"
#include "net_tools/net_threads.h"
#include "net_msg.h"
#include "net_msg_queue.h"
#include "ipv4.h"
/**
 * 数据包进入协议栈进行处理
 */
static int netif_in(netif_t *netif, pkg_t *package)
{
    int ret;
    netif->recv_pkg_cnt++;
    if (!netif->link_ops)
    {
        if (netif->info.type == NETIF_TYPE_LOOP)
        {
            // loop类型的接口，没有链路层，mac地址为全0
            // 因此直接交给ipv4层处理
            return ipv4_in(netif, package);
        }
        else
        {
            dbg_error("no link layer ops:ether? wifi?\r\n");
            return -1;
        }
    }

    // 调用链路层处理，例如ether层
    ret = netif->link_ops->in(netif, package);
    if (ret < 0)
    {
        dbg_warning("link layer ops handler a pkg fail\r\n");
        return -2;
    }

    return 0;
}
static void do_net_func(net_app_task_t* task)
{
   task->ret = (void*)task->func(task->arg);
   semaphore_post(&task->sem);
}
/**
 * 处理消息队列里的消息，包括数据包和应用请求
 */
static void handle(void)
{
    int ret;
    // 依次取出msgQ中的数据进行处理
    while (1)
    {
        // 取消息，最多等第一个定时器的时间
        net_msg_t *msg = net_msg_dequeue(-1);
        if (!msg)
        {
            return; // 直到取不出来为止，取不出来说明队列空了，返回
        }

        netif_t *netif = msg->netif;

        switch (msg->type)
        {
        case NET_MSG_TYPE_PKG:
            pkg_t *package = msg->package;
            net_msg_free(msg);
            ret = netif_in(netif, package);
            if (ret < 0)
            {
                netif->recv_fail_cnt++;
                package_collect(msg->package);
            }
            break;
        case NET_MSG_TYPE_NET_FUNC:
            do_net_func(msg->task);
            net_msg_free(msg);
            break;
        default:
            dbg_error("unkown msg type\r\n");
            break;
        }
    }
}
DEFINE_PROCESS_FUNC(worker)
{
    int ret;

    while (1)
    {
        netif_t *netif = netif_first();
        uint32_t before = get_cur_time_ms();
        // 遍历每一块网卡,把所有数据放入msgQ中
        while (netif)
        {
            // 从网卡的inq中取package放入msgQ
            pkg_t *package = netif_get_pkg_from_inq(netif, -1);
            if (!package)
            {
                // 没取到包，就说明这块网卡的数据取完了，找下一块
                netif = netif_next(netif);
                continue;
            }
            net_msg_t *pkg_msg = net_msg_create(netif, NET_MSG_TYPE_PKG, package);
            if (!pkg_msg)
            {
                package_collect(package);
                continue;
            }
            ret = net_msg_enqueue(pkg_msg, -1);
            if (ret < 0)
            {
                dbg_warning("net msgQ full,drop one pkg\r\n");
                // 内部会释放package
                package_collect(pkg_msg->package);
                net_msg_free(pkg_msg);
            }
            netif = netif_next(netif);
        }

        // 处理消息队列的数据
        handle();
        uint32_t after = get_cur_time_ms();
        soft_timer_scan_list(after - before);
    }
}

void net_worker_init(void)
{
    // 注意：这创建的是内核线程，没有堆区。
    thread_create(worker);
}