#include "tcp.h"
#include "net_tools/msgQ.h"
#include "socket_param.h"

int tcp_connQ_init(tcp_t *tcp)
{
    int capacity = 0;
    if (!tcp->conn.backlog)
    {
        capacity = TCP_CONN_QUEUE_MAX_SIZE;
    }
    else
    {
        capacity = tcp->conn.backlog;
    }
    if (capacity > TCP_CONN_QUEUE_MAX_SIZE)
    {
        dbg_error("backlog over\r\n");
        return -1;
    }
    msgQ_init(&tcp->conn.cmplt_conn_q, tcp->conn.cmplt_conn_buf, capacity);
}

int tcp_connQ_enqueue(tcp_t* client_tcp,tcp_t* listen_tcp){
    return msgQ_enqueue(&listen_tcp->conn.cmplt_conn_q,client_tcp,-1);
}
/**由accpect调用 */
tcp_t* tcp_connQ_dequeue(tcp_t* listen_tcp){
    //默认阻塞等
    return (tcp_t*)msgQ_dequeue(&listen_tcp->conn.cmplt_conn_q,listen_tcp->base.conn_wait.tmo);
}
/**在listen态下，收到了syn包，把该包的解析结果传入 */
tcp_t* create_client_tcp(tcp_parse_t *recv_parse, tcp_t *listen_tcp)
{
    /*创建一个新的client_tcp*/
    sock_create_param_t param;
    param.family = listen_tcp->base.family;
    param.type = listen_tcp->base.type;
    param.protocal = listen_tcp->base.protocal;

    int fd = sock_create(&param);
    socket_t *socket = socket_from_index(fd);
    if (!socket)
    {
        dbg_error("invalid sockfd\r\n");
        return -1;
    }
    sock_t *sock = socket->sock;
    if (!sock)
    {
        dbg_error("socket create error,socket->sock is null\r\n");
        return -2;
    }

    tcp_t *client_tcp = (tcp_t *)sock;
    client_tcp->base.host_port = recv_parse->dest_port;
    client_tcp->base.target_port = recv_parse->src_port;
    client_tcp->base.host_ip.q_addr = listen_tcp->base.host_ip.q_addr;
    client_tcp->base.target_ip.q_addr = listen_tcp->base.target_ip.q_addr;
    
    tcp_init_connect(client_tcp);
    
    return client_tcp;
}
