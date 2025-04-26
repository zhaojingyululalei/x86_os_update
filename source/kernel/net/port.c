
#include "port.h"
#include "string.h"
#include "socket.h"
#include "printk.h"
#include "net_cfg.h"
#include "algrithem.h"
#include "_time.h"
#include "net_cfg.h"
bool mul_port =false ;
static net_port_t ports[PORT_MAX_NR];

port_t net_port_dynamic_alloc(void)
{
    port_t ret = 0;

#ifdef TEST_FOR_PORT_RANDOM
    while (1)
    {
        uint32_t timeseed = get_time_seed();
        uint32_t rand = random(timeseed);
        rand = rand % 1000 + 1; // 产生一个1~1000的随机数
        int port_idx = PORT_DYNAMIC_ALLOC_LOWER_BOUND + rand;
        if (ports[port_idx].state == PROT_STATE_FREE)
        {
            ports[port_idx].state = PROT_STATE_USED;
            return port_idx;
        }
    }
#else
    for (int i = PORT_DYNAMIC_ALLOC_LOWER_BOUND; i <= PORT_DYNAMIC_ALLOC_UP_BOUND; i++)
    {
        if (ports[i].state == PROT_STATE_FREE)
        {
            ports[i].state = PROT_STATE_USED;
            ret = i;
            break;
        }
    }

#endif

    return ret;
}

int net_port_free(port_t port)
{
    if (port <= 0 || port > PORT_MAX_NR)
    {
        dbg_error("port out of boundry\r\n");
        return -1;
    }
    ports[port].state = PROT_STATE_FREE;
}

int net_port_use(port_t port)
{
    if (port <= 0 || port > PORT_MAX_NR)
    {
        dbg_error("port out of boundry\r\n");
        return -1;
    }
    if (ports[port].state == PROT_STATE_USED)
    {
        dbg_error("The port is occupied\r\n");
        return -2;
    }
    ports[port].state = PROT_STATE_USED;
    return 0;
}

void net_port_init(void)
{
    memset(ports, 0, sizeof(net_port_t) * PORT_MAX_NR);
}

int net_port_is_free(port_t port)
{
    if (ports[port].state == PROT_STATE_FREE)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
int net_port_is_used(port_t port)
{
    if (ports[port].state == PROT_STATE_USED)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
