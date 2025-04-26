#include "icmpv4.h"
#include "netif.h"
#include "protocal.h"
#include "ipv4.h"
#include "ipaddr.h"
#include "raw.h"
static bool is_icmpv4_ok(pkg_t* package)
{
    uint16_t checksum = package_checksum16(package, 0, package->total, 0, 1);
    if (checksum != 0)
    {
        dbg_warning("icmp format fault:checksum\r\n");
        return false;
    }
    return true;
}
static int icmpv4_send_reply(ipaddr_t* dest,pkg_t* package)
{
    int ret;
    icmpv4_t* icmp = package_data(package,sizeof(icmpv4_t),0);
    icmp->head.checksum = 0;
    icmp->head.type = ICMPv4_ECHO_REPLY;

    return icmpv4_out(dest,package);
}

int icmpv4_send_unreach(ipaddr_t *dest, pkg_t *package, int code)
{
    int ret;
    package_add_headspace(package,sizeof(icmpv4_t));
    icmpv4_t* icmp_head = package_data(package,sizeof(icmpv4_t),0);
    icmp_head->head.type = ICMPv4_UNREACH;
    icmp_head->head.code = code;
    icmp_head->head.checksum = 0;
    icmp_head->reverse = 0;

    ret = icmpv4_out(dest,package);
    return ret;
}
int icmpv4_in(netif_t *netif,ipaddr_t* visitor_ip,ipaddr_t* host_ip,pkg_t* package,ipv4_header_t* ipv4_head)
{
    int ret;
    //获取icmp包头
    icmpv4_t *icmpv4_pkg = (icmpv4_t *)package_data(package, sizeof(icmpv4_t), 0);

    if(!is_icmpv4_ok(package))
    {
        dbg_warning("recv a wrong format icmp pkg\r\n");
        return -1;
    }

    switch (icmpv4_pkg->head.type)
    {
    case ICMPv4_ECHO_REQUEST:
        return icmpv4_send_reply(visitor_ip,package);
        break;
    case ICMPv4_ECHO_REPLY:
        //添加ipv4头
        package_add_header(package,ipv4_head,sizeof(ipv4_header_t));
        ret = raw_in(netif,package);
        if(ret < 0)
        {
            return  -2;
        }
        break;
    default:
        break;
    }


    return 0;   
}
int icmpv4_out(ipaddr_t* dest,pkg_t* package)
{
    int ret;
    icmpv4_t* icmp = package_data(package,sizeof(icmpv4_t),0);
    icmp->head.checksum = package_checksum16(package,0,package->total,0,1);
    return ipv4_out(package,PROTOCAL_TYPE_ICMPV4,dest);
}







void icmpv4_init(void)
{
    return;
}