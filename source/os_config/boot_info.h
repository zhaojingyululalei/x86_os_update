#ifndef __BOOT_INFO_H
#define __BOOT_INFO_H

#include "types.h"
#define BOOT_RAM_REGION_MAX			10
/**
 * 启动信息参数
 */
#pragma pack(1)
typedef struct _boot_info_t {
    uint32_t gdt_base_addr;
    uint32_t kernel_size;
    struct {
        uint32_t start;
        uint32_t size;
    }ram_region_cfg[BOOT_RAM_REGION_MAX];
    int ram_region_count;
}boot_info_t;

typedef struct _ARDS_t {
    uint32_t BaseAddrLow;   // 内存区域的起始地址（低32位）
    uint32_t BaseAddrHigh;  // 内存区域的起始地址（高32位）
    uint32_t LengthLow;     // 内存区域的长度（低32位）
    uint32_t LengthHigh;    // 内存区域的长度（高32位）
    uint32_t Type;          // 内存区域的类型
}ARDS_t;

#pragma pack()

#endif
