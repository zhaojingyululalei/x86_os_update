#include "tcp.h"
static tcp_send_reset_for_keepalive(tcp_t* tcp){
    int ret;
    pkg_t *package = package_alloc(sizeof(tcp_head_t));
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    tcp_parse_t parse;
    memset(&parse, 0, sizeof(tcp_parse_t));
    parse.src_port = tcp->base.host_port;
    parse.dest_port = tcp->base.target_port;
    parse.seq_num = tcp->snd.next;
    parse.ack_num = tcp->rcv.next;
    parse.win_size = TCP_SND_WIN_SIZE;
    // seq_num没有，因为没收到ack_num
    parse.head_len = sizeof(tcp_head_t); // 没选项字段
    parse.rst = true;
    parse.ack = true;

    // 设置包头
    tcp_set(package, &parse);
    return tcp_send_out(package, &tcp->base.host_ip, &tcp->base.target_ip);
}

static void* tcp_keepintv_handle(void* arg){
    tcp_t* tcp= (tcp_t*)arg;
    //发送探测报文
    tcp_send_keepalive(tcp);
    if(tcp->conn.k_retry == ++tcp->conn.k_count){
        //发送复位包
        tcp_send_reset_for_keepalive(tcp);
    }
}
static void* tcp_keepidle_handle(void* arg){
    tcp_t* tcp=(tcp_t*)arg;
    //idle时间到，发送keepalive探测报文
    tcp_send_keepalive(tcp);
    //将idle定时器卸下，换成invt定时器
    soft_timer_remove(&tcp->conn.timer);
    soft_timer_add(&tcp->conn.timer,
        SOFT_TIMER_TYPE_PERIOD,
        tcp->conn.k_intv*1000,
        "keep_invt",
        tcp_keepintv_handle,
        tcp,
        NULL
        );
    
}
int tcp_keepalive_restart(tcp_t* tcp){
    if(!tcp->conn.k_enable){
        return -1;
    }
    soft_timer_remove(&tcp->conn.timer);
    soft_timer_add(&tcp->conn.timer,
                    SOFT_TIMER_TYPE_ONCE,
                    tcp->conn.k_idle*1000,
                    "keep_idle",
                    tcp_keepidle_handle,
                    tcp,
                    NULL
                    );
}


int tcp_keepalive_start(tcp_t* tcp){
    if(!tcp->conn.k_enable){
        return -1;
    }
    soft_timer_add(&tcp->conn.timer,
                    SOFT_TIMER_TYPE_ONCE,
                    tcp->conn.k_idle*1000,
                    "keep_idle",
                    tcp_keepidle_handle,
                    tcp,
                    NULL
                    );
    

}