#include "tcp.h"

const static char* state_name_tbl[]={
    [TCP_STATE_CLOSED] = "TCP_STATE_CLOSED",
    [TCP_STATE_LISTEN] = "TCP_STATE_LISTEN",
    [TCP_STATE_SYN_SEND] = "TCP_STATE_SYN_SEND",
    [TCP_STATE_SYN_RECVD] = "TCP_STATE_SYN_RECVD",
    [TCP_STATE_ESTABLISHED] = "TCP_STATE_ESTABLISHED",
    [TCP_STATE_FIN_WAIT1] = "TCP_STATE_FIN_WAIT1",
    [TCP_STATE_FIN_WAIT2] = "TCP_STATE_FIN_WAIT2",
    [TCP_STATE_CLOSING] = "TCP_STATE_CLOSING",
    [TCP_STATE_TIME_WAIT] = "TCP_STATE_TIME_WAIT",
    [TCP_STATE_CLOSE_WAIT] = "TCP_STATE_CLOSE_WAIT",
    [TCP_STATE_LAST_ACK] = "TCP_STATE_LAST_ACK"
};
char* tcp_state_name(tcp_state_t state)
{
    if(state>=TCP_STATE_MAX)
    {
        return NULL;
    }
    return state_name_tbl[state];
}

void tcp_set_state(tcp_t* tcp,tcp_state_t state)
{
    if(state>=TCP_STATE_MAX)
    {
        dbg_error("tcp state err\r\n");
        return;
    }
    tcp->state = state;
    
}