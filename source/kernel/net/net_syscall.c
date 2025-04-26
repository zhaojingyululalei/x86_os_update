#include "net_syscall.h"
#include "dev/chr/rtl8139.h"
#include "ipaddr.h"
#include "ether.h"
#include "loop.h"
static int netif_id = 0;
/*普通用户能干的事情*/
static rtl8139_priv_t priv;
/**
 * 创建netif结构
 */
int sys_netif_create(const char *if_name, netif_type_t type)
{
    int ret;
    netif_t *target;
    target = netif_alloc();
    if (target)
    {
        
        target->info.type = type;
        target->state = NETIF_STATE_CLOSE;
        strcpy(target->info.name, if_name);
        

        // 装驱动 并初始化相应硬件(调用open初始化)
        switch (type)
        {
        case NETIF_TYPE_ETH:
            target->ops = &netdev8139_ops; // 安装网卡驱动
            target->ex_data = &priv;

            target->link_ops = &ether_ops; //安装链路层驱动
            target->mtu = ETHER_MTU_DEFAULT;
            netif_rtl8139 = target;
            break;
        case NETIF_TYPE_LOOP:
            netif_loop = target;
        default:
            dbg_error("Unrecognized network card type\r\n");
            goto create_fail;
            break;
        }

        /*加入netif队列*/
        netif_insert(target);
    }
    else
    {
        dbg_error("The number of network cards has reached the maximum limit,can not create one more\r\n");
        goto create_fail;
    }
    return 0;
create_fail:
    netif_free(target);
    return -1;
}
/**
 * 初始化netif拥有的资源
 * 初始化netif消息队列
 */
int sys_netif_open(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (!target_if)
    {
        dbg_error("netif_open fail:there is no netif name:%s\r\n", if_name);
        return -1;
    }
    else
    {
        if(target_if->state!=NETIF_STATE_CLOSE)
        {
            dbg_error("netif_open fail:netif state is not close,can not open\r\n");
        }
        // 找到了接口  打开接口
        target_if->state = NETIF_STATE_OPEN;
        // 初始化netif接口的所有资源
        msgQ_init(&target_if->in_q, target_if->in_q_buf, NETIF_PKG_CACHE_CAPACITY);
        msgQ_init(&target_if->out_q, target_if->out_q_buf, NETIF_PKG_CACHE_CAPACITY);
        
    }
    return 0;
}
#include "ipv4.h"
/**
 * 激活网卡
 * 网卡中断开始工作
 */
int sys_netif_active(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (target_if)
    {
        if(target_if->state != NETIF_STATE_OPEN)
        {
            dbg_error("active fail:netif is not in the open state and cannot be activated\r\n");
            return -3;
        }
        ret = target_if->ops->open(target_if, &priv); // 初始化8139网卡，中断开始工作
        if (ret < 0)
        {
            dbg_error("active fail:target_if->ops->open failed\r\n");
            return -1;
        }
        if(!target_if->link_ops)
        {
            dbg_error("active fail:link drive not installed\r\n");
            return -2;
        }
        //链路层的一些前戏，比如发送arp anounce包，通知自己的网卡ip对应的mac
        ret = target_if->link_ops->open(target_if); 
        if(ret < 0)
        {
            dbg_error("active fail:target_if->link_ops->open fail\r\n");
            target_if->ops->close(target_if); //关闭网卡
            return -3;
        }
        //安装路由表
        ip_route_entry_set("0.0.0.0","192.168.169.40","0.0.0.0",21010,"eth0");
        ip_route_entry_set("192.168.169.0","0.0.0.0","255.255.255.0",0,"eth0");

        target_if->state = NETIF_STATE_ACTIVE;

    }
    else
    {
        return -2;
    }
    return 0;
}


int sys_netif_deactive(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (target_if)
    {
        if(target_if->state != NETIF_STATE_ACTIVE)
        {
            dbg_error("8139 is not in the active state and cannot be die\r\n");
            return -3;
        }
        ret = target_if->ops->close(target_if); // 初始化8139网卡，中断开始工作
        if (ret < 0)
        {
            dbg_error("8139 failed to function properly, initialization failed\r\n");
            return -1;
        }

        target_if->state = NETIF_STATE_DIE;

    }
    else
    {
        return -2;
    }
    return 0;
}

int sys_netif_close(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (target_if)
    {
        if(target_if->state != NETIF_STATE_DIE)
        {
            dbg_error("8139 is still working,not in the die state and cannot be close\r\n");
            return -3;
        }
        
        target_if->state = NETIF_STATE_CLOSE;
        //释放网卡资源
        //队列中所有的数据包取出来并释放掉
        netif_remove(target_if);
        netif_free(target_if);

    }
    else
    {
        return -2;
    }
    return 0;
}



void sys_netif_show(void)
{
    netif_show_list();
}

/*用户设置了新ip，还需要重启协议栈之类的工作，记得加*/
int sys_netif_set_ip(const char* if_name,const char* ip_str)
{
    NETIF_DBG_PRINT("new ip set,restart the protocol stack\r\n");
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if(!target_if)
    {
        return -1;
    }

    return netif_set_ip(target_if,ip_str);


}
int sys_netif_set_gateway(const char* if_name,const char* ip_str)
{
    NETIF_DBG_PRINT("new ip set,restart the protocol stack\r\n");
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if(!target_if)
    {
        return -1;
    }
    return netif_set_gateway(target_if,ip_str);

    
}
int sys_netif_set_mask(const char* if_name,const char* ip_str)
{
    NETIF_DBG_PRINT("new ip set,restart the protocol stack\r\n");
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if(!target_if)
    {
        return -1;
    }

    return netif_set_mask(target_if,ip_str);
}


#include "socket.h"
#include "ipaddr.h"
#include "string.h"
int inet_pton(int family, const char *strptr, void *addrptr)
{
    // 仅支持IPv4地址类型
    if ((family != AF_INET) || !strptr || !addrptr) {
        return -1;
    }
    struct in_addr * addr = (struct in_addr *)addrptr;
    
    ipaddr_t dest;
    ipaddr_s2n(strptr,&dest);
    
    addr->s_addr = dest.q_addr;
    return 1;
}
const char * inet_ntop(int family, const void *addrptr, char *strptr, int len)
{
    if ((family != AF_INET) || !addrptr || !strptr || !len) {
        return (const char *)0;
    }

    struct in_addr * addr = (struct in_addr *)addrptr;
    char buf[20];
    sprintf(buf, "%d.%d.%d.%d", addr->addr0, addr->addr1, addr->addr2, addr->addr3);
    strncpy(strptr, buf, len - 1);
    strptr[len - 1] = '\0';
    return strptr;
}





