#include "ipaddr.h"
#include "string.h"
int ipaddr_s2n(const char *ip_str, ipaddr_t *ip_addr)
{
    if (!ip_str || !ip_addr)
    {
        return -1;
    }
    ip_addr->type = IPADDR_V4;
    uint8_t bytes[IPADDR_ARRY_LEN] = {0};
    int byte = 0, index = 0;

    while (*ip_str)
    {
        if (*ip_str >= '0' && *ip_str <= '9')
        {
            byte = byte * 10 + (*ip_str - '0');
            if (byte > 255)
            {
                return -1;
            }
        }
        else if (*ip_str == '.')
        {
            if (index >= IPADDR_ARRY_LEN - 1)
            {
                return -1;
            }
            bytes[index++] = byte;
            byte = 0;
        }
        else
        {
            return -1;
        }
        ip_str++;
    }

    if (index != IPADDR_ARRY_LEN - 1)
    {
        return -1;
    }
    bytes[index] = byte;

    for (int i = 0; i < IPADDR_ARRY_LEN; ++i)
    {
        ip_addr->a_addr[i] = bytes[i];
    }

    ip_addr->q_addr = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

    return 0;
}

int ipaddr_n2s(const ipaddr_t *ip_addr, char *ip_str, int str_len)
{
    if (!ip_addr || !ip_str || str_len < 16)
    {
        return -1;
    }

    
    ip_str[str_len - 1] = '\0';

    sprintf(ip_str, "%d.%d.%d.%d",
            ip_addr->a_addr[3], ip_addr->a_addr[2],
            ip_addr->a_addr[1], ip_addr->a_addr[0]);

    return 0;
}

int mac_n2s(const hwaddr_t *mac, char *mac_str)
{
    // 用于存放一个 MAC 地址的字符串表示（格式：xx:xx:xx:xx:xx:xx）
    // 每个字节是两位十六进制数，使用冒号分隔
    for (int i = 0; i < 6; ++i)
    {
        // 每个字节需要转成两位十六进制字符，%02x 表示输出两位，不足补零
        sprintf(mac_str + (i * 3), "%02x", mac->addr[i]);

        // 如果不是最后一个字节，插入冒号
        if (i < 5)
        {
            mac_str[(i * 3) + 2] = ':';
        }
    }
    mac_str[17] = '\0'; // 结束符，17是因为6个字节 * 3 + 1个结束符
    return 0;
}

int mac_s2n(const char *mac_str, hwaddr_t *mac)
{
    int j = 0;

    for (int i = 0; i < 17; i += 3)
    { // 每次读取3个字符，两个十六进制字符和一个冒号
        // 如果是冒号跳过
        if (mac_str[i] == ':')
        {
            continue;
        }

        // 将两个十六进制字符转换为一个字节
        uint8_t byte = 0;
        for (int k = 0; k < 2; ++k)
        {
            byte <<= 4;
            if (mac_str[i + k] >= '0' && mac_str[i + k] <= '9')
            {
                byte |= (mac_str[i + k] - '0');
            }
            else if (mac_str[i + k] >= 'a' && mac_str[i + k] <= 'f')
            {
                byte |= (mac_str[i + k] - 'a' + 10);
            }
            else if (mac_str[i + k] >= 'A' && mac_str[i + k] <= 'F')
            {
                byte |= (mac_str[i + k] - 'A' + 10);
            }
        }

        mac->addr[j++] = byte; // 将字节存入数组
    }

    return 0;
}

// 32位无符号整数转换函数（主机到网络）
uint32_t htonl(uint32_t hostlong)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((hostlong >> 24) & 0x000000ff) |
               ((hostlong >> 8) & 0x0000ff00) |
               ((hostlong << 8) & 0x00ff0000) |
               ((hostlong << 24) & 0xff000000);
    }
    return hostlong; // 如果系统是大端字节序，则无需转换
}

// 16位无符号整数转换函数（主机到网络）
uint16_t htons(uint16_t hostshort)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((hostshort >> 8) & 0x00ff) |
               ((hostshort << 8) & 0xff00);
    }
    return hostshort;
}

// 32位无符号整数转换函数（网络到主机）
uint32_t ntohl(uint32_t netlong)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((netlong >> 24) & 0x000000ff) |
               ((netlong >> 8) & 0x0000ff00) |
               ((netlong << 8) & 0x00ff0000) |
               ((netlong << 24) & 0xff000000);
    }
    return netlong;
}

// 16位无符号整数转换函数（网络到主机）
uint16_t ntohs(uint16_t netshort)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((netshort >> 8) & 0x00ff) |
               ((netshort << 8) & 0xff00);
    }
    return netshort;
}

static hwaddr_t mac_broadcast = {
    .addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
static hwaddr_t mac_empty = {
    .addr = {0, 0, 0, 0, 0, 0}};
hwaddr_t *get_mac_broadcast(void)
{
    return &mac_broadcast;
}
hwaddr_t *get_mac_empty(void)
{
    return &mac_empty;
}

uint32_t ipaddr_get_host(ipaddr_t *ip, ipaddr_t *mask)
{

    uint32_t host;
    uint32_t ip_ = ip->q_addr;
    uint32_t mask_ = mask->q_addr;

    host = (~mask_) & ip_;
    return host;
}
uint32_t ipaddr_get_net(ipaddr_t *ip, ipaddr_t *mask)
{

    uint32_t net;
    uint32_t ip_ = ip->q_addr;
    uint32_t mask_ = mask->q_addr;

    net = ip_ & mask_;
    return net;
}

/*是否是局部广播地址*/
int is_local_boradcast(ipaddr_t *host_ip, ipaddr_t *host_mask, ipaddr_t *ip)
{

    // 查看网络部分是否相同
    uint32_t me = ipaddr_get_net(host_ip, host_mask);
    uint32_t him = ipaddr_get_net(ip, host_mask);
    if (me != him)
    {
        return 0;
    }
    // 如果网络部分相同，继续检查主机部分是否全1
    uint32_t host = ipaddr_get_host(ip, host_mask);

    if (host == ~host_mask->q_addr)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
/*是否是全局广播地址*/
int is_global_boradcast(ipaddr_t *ip)
{

    ipaddr_t ip_broad = {
        .type = IPADDR_V4};
    ipaddr_s2n("255.255.255.255", &ip_broad);
    if (ip->q_addr == ip_broad.q_addr)
    {
        return 1;
    }
    return 0;
}