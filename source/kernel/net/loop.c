/**
 * loop网口，没有数据链路层，因此最底层的协议只到ipv4层
 * 接收到的包最底层是ipv4层协议
 * 发出的包最底层也是ipv4层
 * 而且不涉及arp包，因为没有mac地址
 */

#include "netif.h"

netif_t* netif_loop;