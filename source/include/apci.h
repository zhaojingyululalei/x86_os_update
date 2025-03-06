#ifndef __APCI_H
#define __APCI_H


#include "types.h"

// RSDP 签名
#define RSDP_SIGNATURE "RSD PTR "
// ACPI 表签名
#define ACPI_SIGNATURE_MADT "APIC"
// 最大支持的 RSDT 条目数
#define MAX_RSDT_ENTRIES 32
// 最大支持的 MADT 子结构数
#define MAX_MADT_ENTRIES 64

// 内存范围
#define RSDP_SEARCH_START 0x000E0000
#define RSDP_SEARCH_END   0x000FFFFF

// RSDP 结构（ACPI 1.0 和 ACPI 2.0+）
#pragma pack(1)
typedef struct {
    char signature[8];    // "RSD PTR "
    uint8_t checksum;     // 校验和
    char oemid[6];        // OEM ID
    uint8_t revision;     // 版本号
    uint32_t rsdt_addr;   // RSDT 地址（ACPI 1.0）

    uint32_t length;      // RSDP 长度（ACPI 2.0+）
    uint32_t xsdt_addr_low;   // XSDT 地址（ACPI 2.0+）
    uint32_t xsdt_addr_high;
    uint8_t ext_checksum; // 扩展校验和（ACPI 2.0+）
    uint8_t reserved[3];  // 保留字段
} rsdp_t;



// ACPI 表头结构
typedef struct {
    char signature[4];  // 表签名
    uint32_t length;     // 表长度
    uint8_t revision;    // 表版本
    uint8_t checksum;    // 校验和
    char oemid[6];       // OEM ID
    char oem_table_id[8];// OEM 表 ID
    uint32_t oem_revision;// OEM 版本
    uint32_t creator_id;  // 创建者 ID
    uint32_t creator_revision;// 创建者版本
} acpi_table_header_t;

// RSDT 结构
typedef struct {
    acpi_table_header_t header;  // ACPI 表头
    uint32_t entries[MAX_RSDT_ENTRIES];          // 指向其他 ACPI 表的指针数组
} rsdt_t;

// MADT 结构
typedef struct {
    acpi_table_header_t header;  // ACPI 表头
    uint32_t local_apic_addr;    // 本地 APIC 地址
    uint32_t flags;              // 标志位
    uint8_t entries[MAX_MADT_ENTRIES * 16];           // APIC 子结构
} madt_t;

// MADT 子结构类型
typedef enum {
    MADT_ENTRY_PROCESSOR_LOCAL_APIC = 0,
    MADT_ENTRY_IO_APIC = 1,
    MADT_ENTRY_INTERRUPT_SOURCE_OVERRIDE = 2,
    MADT_ENTRY_NMI_SOURCE = 3,
    MADT_ENTRY_LOCAL_APIC_NMI = 4,
    MADT_ENTRY_UNKNOWN = 255
} madt_entry_type_t;

// MADT 子结构通用头
typedef struct {
    uint8_t type;    // 子结构类型
    uint8_t length;  // 子结构长度
} madt_entry_header_t;

// MADT 子结构：处理器本地 APIC
typedef struct {
    madt_entry_header_t header;  // 子结构头
    uint8_t processor_id;        // 处理器 ID
    uint8_t apic_id;             // APIC ID
    uint32_t flags;              // 标志位
} madt_entry_processor_local_apic_t;

// MADT 子结构：I/O APIC
typedef struct {
    madt_entry_header_t header;  // 子结构头
    uint8_t io_apic_id;          // I/O APIC ID
    uint8_t reserved;            // 保留
    uint32_t io_apic_addr;       // I/O APIC 地址
    uint32_t global_system_interrupt_base; // 全局系统中断基址
} madt_entry_io_apic_t;

// MADT 子结构：中断源覆盖
typedef struct {
    madt_entry_header_t header;  // 子结构头
    uint8_t bus;                 // 总线
    uint8_t source;              // 源
    uint32_t global_system_interrupt; // 全局系统中断
    uint16_t flags;              // 标志位
} madt_entry_interrupt_source_override_t;

// MADT 子结构：非屏蔽中断源
typedef struct {
    madt_entry_header_t header;  // 子结构头
    uint16_t flags;              // 标志位
    uint32_t global_system_interrupt; // 全局系统中断
} madt_entry_nmi_source_t;

// MADT 子结构：本地 APIC NMI
typedef struct {
    madt_entry_header_t header;  // 子结构头
    uint8_t processor_id;        // 处理器 ID
    uint16_t flags;              // 标志位
    uint8_t lint;                // LINT
} madt_entry_local_apic_nmi_t;

// 保存 MADT 子结构的联合体
typedef union {
    madt_entry_header_t header;
    madt_entry_processor_local_apic_t processor_local_apic;
    madt_entry_io_apic_t io_apic;
    madt_entry_interrupt_source_override_t interrupt_source_override;
    madt_entry_nmi_source_t nmi_source;
    madt_entry_local_apic_nmi_t local_apic_nmi;
} madt_entry_t;
#pragma pack()
extern rsdt_t rsdt_static;
extern madt_t madt_static;
extern madt_entry_t madt_entries_static[MAX_MADT_ENTRIES];
extern uint32_t madt_entry_count ;
rsdp_t *find_rsdp() ;
void apci_init(void);
#endif