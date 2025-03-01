#ifndef __NET_CFG_H
#define __NET_CFG_H
typedef enum _net_err_t{
    NET_ERR_OK=0,
    NET_ERR_RECV_RST=-21,
    NET_ERR_TMO=-22,
    NET_ERR_CLOSE = -23,
    NET_ERR_NEED_WAIT = -24,
}net_err_t;
/*addr*/
#define IPADDR_ARRY_LEN 4
#define MACADDR_ARRAY_LEN 6

/*netif*/
#define NETIF_PKG_CACHE_CAPACITY 50


/*ether*/
#define ETHER_MTU_DEFAULT   1500
#define ETHER_MTU_MAX_SIZE ETHER_MTU_DEFAULT
#define ETHER_MTU_MIN_SIZE 46

/*arp*/
#define ARP_ENTRY_MAX_SIZE 20
#define ARP_ENTRY_PKG_CACHE_MAX_NR  5
//单位秒
#define ARP_ENTRY_TMO_STABLE    (20*60)    //已经解析的表项，每隔多久跟新一次
    //以下设置，arp表项处于wating状态最多15秒，否则就删除表项
#define ARP_ENTRY_TMO_RESOLVING 3    //该表项跟新时，每隔多久发一次arp请求
#define ARP_ENTRY_RETRY 5           //发了n次请求，还是没解析出来，就删除该表项

/*ipv4*/
#define IPV4_FRAG_TIMER_SCAN    1   //周期性扫描分片列表,长时间组不成完整包就丢弃
#define IPV4_FRAG_TMO           5   //规定时间内，没有组成完整分片，就释放掉


/*port*/
#define PORT_MAX_NR 2001
#define PORT_DYNAMIC_ALLOC_LOWER_BOUND  1000
#define PORT_DYNAMIC_ALLOC_UP_BOUND 2000

/*dns*/
#define DNS_REQ_SIZE                    10          // DNS请求结构
#define DNS_SERVER_DEFAULT              "180.76.76.76"  // 百度的DNS服务器
#define DNS_SERVER_0                    "8.8.8.8"       // google的DNS服务器
#define DNS_SERVER_1                    "223.6.6.6"     // 阿里云的DNS服务器
#define DNS_SERVER_MAX                  4           // 最大支持的DNS服务器表项数量
#define DNS_ENTRY_SIZE                  10          // DNS表项的数量
#define DNS_DOMAIN_NAME_MAX             64          // DNS中域名的长度
#define DNS_PORT_DEFAULT                53          // 缺省的DNS端口
#define DNS_QUERY_RETRY_TMO             5           // DNS查询超时
#define DNS_QUERY_RETRY_CNT             5           // 查询超时重试次数
#define DNS_WORKING_BUF_SIZE            512         // 工作缓存大小
/*debug */
//这个不要注释掉
#define TEST_FOR_PORT_RANDOM
//#define PKG_DBG
// #define NET_DRIVE_DBG
// #define NETIF_DBG
// #define ETHER_DBG
// #define ARP_DBG
// #define DBG_SOFT_TIMER_PRINT
//#define IPV4_DBG
// #define ICMPV4_DBG
#define TCP_DBG
// #define TCP_HASH_TBL_DBG
#endif