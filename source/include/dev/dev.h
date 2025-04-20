#ifndef __DEV_H
#define __DEV_H

#include "tools/rb_tree.h"
#include "tools/list.h"
#include "fs/devfs/devfs.h"
#define DEV_NAME_SIZE               32      // 设备名称长度

// 设备类型
typedef enum _device_type_t
{
    DEV_TYPE_NULL,  // 空设备
    DEV_TYPE_CHAR,  // 字符设备
    DEV_TYPE_BLOCK, // 块设备
    DEV_TYPE_NET,   // 网络设备
    DEV_TYPE_MAX
}device_type_t;

// 设备子类型
enum device_subtype_t
{
    DEV_MAJOR_NULL,
    DEV_MAJOR_CONSOLE = 1, // 控制台
    DEV_MAJOR_KEYBOARD,    // 键盘
    DEV_MAJOR_SERIAL,      // 串口
    DEV_MAJOR_TTY,         // TTY 设备
    DEV_MAJOR_SB16,        // 声霸卡
    DEV_MAJOR_IDE_DISK,    // IDE 磁盘
    DEV_MAJOR_IDE_PART,    // IDE 磁盘分区
    DEV_MAJOR_IDE_CD,      // IDE 光盘
    DEV_MAJOR_RAMDISK,     // 虚拟磁盘
    DEV_MAJOR_FLOPPY,      // 软盘
    DEV_MAJOR_NETIF,       // 网卡
    DEV_MAJOR_MAX
};

enum dev_cmd_t
{
    DEV_CMD_INVALID,
    DEV_CMD_GETINFO=64,
    DEV_CMD_COMMENTS,
};


typedef struct _dev_ops_t{
    int (*open) (dev_t devfd) ;
    int (*read) (dev_t devfd,  char * buf, int size,int* pos);
    int (*write) (dev_t devfd,  char * buf, int size,int* pos);
    int (*control) (dev_t devfd, int cmd, int arg0, int arg1);
    int (*lseek)(dev_t devfd,int* pos,int offset,int whence);
    void (*close) (dev_t devfd);
}dev_ops_t;


/**
 * @brief 设备文件描述符
 */
typedef struct _dev_desc_t {
    device_type_t type;
    char name[DEV_NAME_SIZE];           // 设备名称
    int major;                          // 主设备号
    int minor;                          // 次设备号
    void * data;                        // 设备参数
    int open_count;  
    
}dev_desc_t;
typedef struct _device_t {
    dev_t devfd; //存放device_t地址
    dev_desc_t  desc;      // 设备特性描述符
    dev_ops_t* ops;
    rb_node_t rbnode;
    list_node_t lnode;
}device_t;


int dev_open (dev_t devfd);
int dev_read (dev_t devfd,  char * buf, int size,int *pos);
int dev_write (dev_t devfd,  char * buf, int size,int *pos);
int dev_control (dev_t devfd, int cmd, int arg0, int arg1);
void dev_close (dev_t devfd);
int dev_install(device_type_t type,int major,int minor,const char* name,void* data,dev_ops_t* ops);
void dev_show_all(void);

#endif