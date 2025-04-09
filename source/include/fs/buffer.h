#ifndef __BUFFER_H
#define __BUFFER_H

#include "tools/list.h"
#include "tools/rb_tree.h"
#include "ipc/mutex.h"
#include "types.h"
#include "mem/page.h"
#include "cpu_cfg.h"

typedef struct _buffer_t{
    //设备号
    int major;
    int minor;
    //块号
    int blk;  //这个块号指的是文件系统块号 2个扇区 而且是4对齐的起始块号

    page_t* page; //数据在哪个页记录着 
    char* data;  //数据在哪
    list_node_t lnode;
    list_node_t lru_node;
    rb_node_t rbnode; 
    bool dirty; //是否修改（是否和磁盘数据一致）
    bool valid; //是否有效
}buffer_t;
buffer_t* fs_read_to_buffer(int major,int minor,int blk,bool modify);
void fs_contend_buffer(buffer_t* buf);
void fs_sync_buffer(buffer_t *buf);
void sys_sync(void);
#endif