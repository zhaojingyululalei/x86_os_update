#ifndef __TCP_H
#define __TCP_H
#include "sock.h"
#include "net_tools/package.h"
#include "net_tools/soft_timer.h"
#include "net_tools/msgQ.h"
#define TCP_BUF_MAX_NR 50
#define TCP_DEFAULT_PROTOCAL    IPPROTO_TCP
#define TCP_HASH_SIZE 128
#define TCP_SND_BUF_MAX_SIZE 2048
#define TCP_RCV_BUF_MAX_SIZE 2048
#define TCP_SND_WIN_SIZE  1024
#define TCP_RCV_WIN_SIZE  1024  

#define TCP_CONN_QUEUE_MAX_SIZE 128

#define TCP_FIN_POS (1<<0)
#define TCP_SYN_POS (1<<1)
#define TCP_RST_POS (1<<2)
#define TCP_PSH_POS (1<<3)
#define TCP_ACK_POS (1<<4)
#define TCP_URG_POS (1<<5)

#define TCP_OPTION_END  0
#define TCP_OPTION_NOP  1
#define TCP_OPTION_MSS  2

#define TCP_DEFAULT_MSS 536

#define TCP_KEEP_IDLE_DEFAULT   5
#define TCP_KEEP_INTV_DEFAULT   1
#define TCP_KEEP_RETRY_DEFAULT    10

#define TCP_TIME_WAIT_MSL   60

typedef struct _tcp_data_t
{
    pkg_t* package;
    ipaddr_t remote_ip;
    port_t remote_port;
    ipaddr_t host_ip;
    port_t host_port;
}tcp_data_t;

typedef struct _tcp_buf_t{
    uint8_t* data;
    int capacity;
    int size;
    int tail;
    int head;
}tcp_buf_t;

typedef enum _tcp_state_t{
    TCP_STATE_CLOSED,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SEND,
    TCP_STATE_SYN_RECVD,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT1,
    TCP_STATE_FIN_WAIT2,
    TCP_STATE_CLOSING,
    TCP_STATE_TIME_WAIT,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_LAST_ACK,

    TCP_STATE_MAX,
}tcp_state_t;

typedef enum _tcp_in_pkg_type_t{
    TCP_PKG_TYPE_SYN_ACK, //收到这种类型数据包，只能遍历链表找符合条件的tcp结构

}tcp_in_pkg_type_t;

typedef struct _tcp_t {
    sock_t base;
    int mss;
    tcp_state_t state;
    struct 
    {
        uint32_t init_seq;
        uint32_t next;
        uint32_t win_size;
        uint32_t win;
        uint8_t data_buf[TCP_RCV_BUF_MAX_SIZE];
        tcp_buf_t buf;
        //sock_wait_t wait;
    }rcv;

    struct 
    {
        uint32_t init_seq;
        uint32_t unack;//已发送，但未确认的数据
        uint32_t next;//下一次要发送的数据
        uint32_t win_size;
        uint8_t data_buf[TCP_SND_BUF_MAX_SIZE];
        tcp_buf_t buf;
        //sock_wait_t wait;
    }snd;

    struct
    {
        //链接的保活配置
        bool k_enable;
        int k_idle;
        int k_intv;
        int k_retry;
        int k_count;
        soft_timer_t timer;

        //全连接队列和半链接队列（listen & accept）
        int backlog;
        struct _tcp_t* listen_tcp;
        struct _tcp_t* cmplt_conn_buf[TCP_CONN_QUEUE_MAX_SIZE];
        queue_t cmplt_conn_q;

    }conn;
    soft_timer_t time_wait_timer;

    sock_wait_t close_wait;
    list_node_t hash_node;
    list_node_t node;
}tcp_t;


#pragma pack(1)
typedef struct _tcp_head_t{
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t hlen_and_flags;
    uint16_t win_size;
    uint16_t checksum;
    uint16_t urg_ptr;
}tcp_head_t;//注：还有个32位选项，可能有或没有
#pragma pack()
typedef struct _tcp_flag_t{
    bool urg;
    bool psh;
    bool rst;
    bool syn;
    bool fin;
}tcp_flag_t;
typedef struct _tcp_parse_t{
    port_t src_port;
    port_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t head_len;
    bool urg;
    bool ack;
    bool psh;
    bool rst;
    bool syn;
    bool fin;
    uint16_t win_size;
    uint16_t checksum;
    uint16_t urgptr;
    uint8_t* options;
    uint16_t option_len;
}tcp_parse_t;


typedef struct _tcp_hash_entry_t{
    list_t value_list;
}tcp_hash_entry_t;

typedef struct _tcp_seq_t{
    uint32_t seq_num;
    int len;
    list_node_t node;
}tcp_seq_t;
extern list_t tcp_list;
typedef int (*tcp_in_handle)(tcp_t* tcp,pkg_t* package,tcp_parse_t* parse,ipaddr_t* remote,ipaddr_t* host);
#define DEFINE_TCP_IN_HANDLE(name)            int name(tcp_t* tcp,pkg_t* package,tcp_parse_t* parse,ipaddr_t* remote,ipaddr_t* host)
/*初始化*/
void tcp_parse(pkg_t *package, tcp_parse_t *parse);
void tcp_set(pkg_t* package,const tcp_parse_t* parse);
void tcp_init(void);
sock_t *tcp_create(int family, int protocol);
void tcp_reset(tcp_t* tcp,net_err_t err);
void tcp_free(tcp_t *tcp);

/*输入*/
int tcp_in(pkg_t* package,ipaddr_t* remote_ip,ipaddr_t* host_ip);
int tcp_read_option(tcp_t* tcp,tcp_parse_t* recv_parse);

/*输出*/
int tcp_send_out(pkg_t *package, ipaddr_t *src, ipaddr_t *dest);
int tcp_send_reset(tcp_parse_t* tcp_recv,ipaddr_t* src,ipaddr_t* dest);
int tcp_send_syn(tcp_t *tcp);
int tcp_send_ack(tcp_t *tcp, tcp_parse_t *tcp_recv);
int tcp_send_syn_ack(tcp_t* tcp,tcp_parse_t* tcp_recv);
int tcp_send_fin(tcp_t* tcp);
int tcp_send_data(tcp_t* tcp,tcp_flag_t *flag);
int tcp_send_keepalive(tcp_t* tcp);
/*状态机*/
char* tcp_state_name(tcp_state_t state);
void tcp_set_state(tcp_t* tcp,tcp_state_t state);


/*hash表*/
uint32_t jenkins_hash(uint32_t a, uint32_t b, uint32_t c, uint32_t d);
tcp_t *find_tcp_connection(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port);
void hash_insert_tcp_connection(tcp_t *tcp);
void hash_delete_tcp_connection(tcp_t *tcp);
void tcp_hash_table_print() ;

/*tcp_buf*/
int tcp_buf_is_full(tcp_buf_t* buf);
int tcp_buf_is_empty(tcp_buf_t* buf);
int tcp_buf_insert(tcp_buf_t* buf, uint8_t* data, int data_size);
int tcp_buf_remove(tcp_buf_t* buf,int max_size) ;
int tcp_buf_expand(tcp_buf_t *buf, int len);
int tcp_buf_read(tcp_buf_t* buf,uint8_t* out_data,int offset,int len);
int tcp_buf_write(tcp_buf_t* buf,int offset,int data_len,uint8_t* data);

void tcp_buf_print(tcp_buf_t* buf);
void tcp_buf_free(tcp_buf_t* buf);
void tcp_buf_init(tcp_buf_t* buf, uint8_t *data,int capacity);

int tcp_write_snd_buf(tcp_t* tcp,const uint8_t* data,int len);
int tcp_read_snd_buf(tcp_t* tcp,pkg_t* tcp_pkg,int pkg_offset,int buf_offset,int len);
int tcp_write_rcv_buf(tcp_t *tcp, int offset, const uint8_t *data, int data_len);

/*seq*/
void tcp_seq_init(void);
tcp_seq_t* tcp_seq_alloc(void);
void tcp_seq_free(tcp_seq_t* seq);
int seq_compare(uint32_t a,uint32_t b);
int seq_check(tcp_t* tcp,tcp_parse_t* recv_parse,pkg_t* recv_pkg);
int seq_handle(tcp_t *tcp, tcp_parse_t *recv_parse, pkg_t *recv_pkg);
int tcp_seq_remove(int size);

/**keep_alive */
int tcp_keepalive_start(tcp_t* tcp);
int tcp_keepalive_restart(tcp_t* tcp);

/**conn */
int tcp_init_connect(tcp_t *tcp);
int tcp_connQ_init(tcp_t* tcp);
tcp_t* create_client_tcp(tcp_parse_t *recv_parse, tcp_t *listen_tcp);
int tcp_connQ_enqueue(tcp_t* client_tcp,tcp_t* listen_tcp);
/**由accpect调用 */
tcp_t* tcp_connQ_dequeue(tcp_t* listen_tcp);
/*dbg*/
// TCP 报文信息打印函数
void tcp_show(const tcp_parse_t* tcp);
#include "net_cfg.h"

#ifdef TCP_DBG
    #define TCP_DBG_PRINT(fmt, ...) dbg_info(fmt, ##__VA_ARGS__)
#else
    #define TCP_DBG_PRINT(fmt, ...) do { } while(0)
#endif

#endif