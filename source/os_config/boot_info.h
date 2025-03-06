#ifndef __BOOT_INFO_H
#define __BOOT_INFO_H

#include "types.h"

#define BOOT_RAM_REGION_MAX			10
#define CPU_CORES_MAX_CNT   8
/**
 * cpu信息
 */
typedef struct {
    char* manufacture;
    int physical_cores;  // 物理核心数
    int logical_cores;   // 逻辑核心数
    int apic_ids_max;   // 保存每个核心的 APIC ID
    ph_addr_t local_apic_addr;
    ph_addr_t io_apic_addr;
} cpu_info_t;
/**
 * 启动信息参数
 */
typedef struct _boot_info_t {
    uint32_t gdt_base_addr;
    uint32_t kernel_size;
    uint32_t mem_size;
    cpu_info_t cpu_info;
    struct {
        uint32_t start;
        uint32_t size;
    }ram_region_cfg[BOOT_RAM_REGION_MAX];
    int ram_region_count;
}boot_info_t;

#pragma pack(1)
//内存检测信息
typedef struct _ARDS_t {
    uint32_t BaseAddrLow;   // 内存区域的起始地址（低32位）
    uint32_t BaseAddrHigh;  // 内存区域的起始地址（高32位）
    uint32_t LengthLow;     // 内存区域的长度（低32位）
    uint32_t LengthHigh;    // 内存区域的长度（高32位）
    uint32_t Type;          // 内存区域的类型
}ARDS_t;

#pragma pack()

#endif
