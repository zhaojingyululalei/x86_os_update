#ifndef __DEVFS_H
#define __DEVFS_H
#include "types.h"

typedef uint32_t dev_t;
#define MAJOR_MASK  0xfff00000  // 主设备号掩码 (高12位)
#define MINOR_MASK  0x000fffff  // 次设备号掩码 (低20位)
#define MAJOR_SHIFT 20          // 主设备号左移位数

#define MAJOR(dev)  (((dev) & MAJOR_MASK) >> MAJOR_SHIFT)  // 提取主设备号
#define MINOR(dev)  ((dev) & MINOR_MASK)                   // 提取次设备号
#define MKDEV(major, minor)  ((((major) << MAJOR_SHIFT) & MAJOR_MASK) | ((minor) & MINOR_MASK))  // 构造 dev_t

#endif