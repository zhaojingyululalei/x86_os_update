#ifndef __NETIF_H
#define __NETIF_H
#include "net_tools/msgQ.h"
#include "net_tools/package.h"
#include "ipaddr.h"
#define NETIF_NAME_STR_MAX_LEN 128
#define NETIF_MAX_NR 3
typedef enum _netif_type_t
{
    NETIF_TYPE_NONE,
    NETIF_TYPE_LOOP,
    NETIF_TYPE_ETH,
    NETIF_TYPE_VETH,//以太网对
    NETIF_TYPE_BRIDGE,//桥接
    NETIF_TYPE_TAP, //TAP接口

    NETIF_TYPE_SIZE
} netif_type_t;
/**
 * 网卡基本信息
 */
typedef struct _netif_info_t
{
    netif_type_t type;
    char name[NETIF_NAME_STR_MAX_LEN];
    ipaddr_t ipaddr;
    ipaddr_t mask;
    ipaddr_t gateway;
} netif_info_t;

/**
 * 1.不同类型网卡的驱动程序接口,如rtl8139 ,veth...
 * */
struct _netif_t;
typedef struct _netif_ops_t
{
    // 不同驱动可能有些不同参数
    int (*open)(struct _netif_t *netif, void *ex_data); 
    int (*close)(struct _netif_t *netif);
    //从网卡的out_q取一个发送，因此参数没有数据
    int (*send)(struct _netif_t *netif);
    //接受一般用中断接受，然后自动放到网卡的in_q中，因此没有接受驱动
} netif_ops_t;

/**
 *  链路层处理接口，如以太网协议，wifi...
 */
typedef struct _link_layer_t
{
    netif_type_t type;

    int (*open)(struct _netif_t *netif);
    void (*close)(struct _netif_t *netif);
    int (*in)(struct _netif_t *netif, pkg_t *package);
    int (*out)(struct _netif_t *netif, ipaddr_t *dest, pkg_t *package);
} link_layer_t;
typedef struct _netif_t
{

    netif_info_t info;
    enum
    {
        NETIF_STATE_CLOSE,
        NETIF_STATE_OPEN,
        NETIF_STATE_ACTIVE,
        NETIF_STATE_DIE
    } state;

    int id;

    hwaddr_t hwaddr;
    int mtu;
    list_node_t node;

    /**
     * 接受发送，数据包缓存，每个网卡
     * */
    msgQ_t in_q;
    pkg_t *in_q_buf[NETIF_PKG_CACHE_CAPACITY];
    msgQ_t out_q;
    pkg_t *out_q_buf[NETIF_PKG_CACHE_CAPACITY];
    
    netif_ops_t *ops; // 驱动
    void *ex_data;    // 驱动参数
    link_layer_t* link_ops; //链路层处理
    
    int send_pkg_cnt;
    int send_fail_cnt;
    int recv_pkg_cnt;
    int recv_fail_cnt;

} netif_t;

extern netif_t* netif_rtl8139;
void netif_init(void);
void netif_destory(void);
netif_t* netif_alloc(void);
int netif_free(netif_t *netif);
int netif_insert(netif_t *netif);
netif_t *netif_remove(netif_t *netif);
netif_t *netif_first();
netif_t *netif_next(netif_t *netif);

pkg_t* netif_get_pkg_from_outq(netif_t* netif,int timeout);
pkg_t* netif_get_pkg_from_inq(netif_t* netif,int timeout);
int netif_put_pkg_into_outq(netif_t* netif,pkg_t* package,int timeout);
int netif_put_pkg_into_inq(netif_t* netif,pkg_t* package,int timeout);
int netif_out(netif_t* netif,ipaddr_t* dest_ip,pkg_t* package);

netif_t *find_netif_by_name(const char *if_name);
void netif_show_info(netif_t* netif);
void netif_show_list(void);


int netif_set_ip(netif_t *target_if,const char* ip_str);
int netif_set_gateway(netif_t *target_if,const char* ip_str);
int netif_set_mask(netif_t *target_if,const char* ip_str);
netif_t* netif_get_8139(void);
#include "printk.h"

#ifdef NETIF_DBG
    #define NETIF_DBG_PRINT(fmt, ...) dbg_info(fmt, ##__VA_ARGS__)
#else
    #define NETIF_DBG_PRINT(fmt, ...) do { } while(0)
#endif
#endif
