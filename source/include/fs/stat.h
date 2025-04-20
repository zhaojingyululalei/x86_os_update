#ifndef __STAT_H
#define __STAT_H
#include "types.h"
#include "time/time.h"
typedef struct _file_stat_t
{
    int major;
    int minor;
    int nr;     // 文件 i 节点号
    uint16_t mode;     // 文件类型和属性
    uint8_t nlinks;    // 指定文件的连接数
    uint16_t uid;      // 文件的用户(标识)号
    uint8_t gid;       // 文件的组号
    size_t size;  // 文件大小（字节数）（如果文件是常规文件）
    time_t atime; // 上次（最后）访问时间
    time_t mtime; // 最后修改时间
    time_t ctime; // 最后节点修改时间
} file_stat_t;

#endif