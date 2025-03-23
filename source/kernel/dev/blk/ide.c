#include "dev/ide.h"

#define IDE_TIMEOUT 60000

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERR 0x0001        // 错误寄存器
#define IDE_FEATURE 0x0001    // 功能寄存器
#define IDE_SECTOR 0x0002     // 扇区数量
#define IDE_LBA_LOW 0x0003    // LBA 低字节
#define IDE_CHS_SECTOR 0x0003 // CHS 扇区位置
#define IDE_LBA_MID 0x0004    // LBA 中字节
#define IDE_CHS_CYL 0x0004    // CHS 柱面低字节
#define IDE_LBA_HIGH 0x0005   // LBA 高字节
#define IDE_CHS_CYH 0x0005    // CHS 柱面高字节
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令

#define IDE_CMD_READ 0x20       // 读命令
#define IDE_CMD_WRITE 0x30      // 写命令
#define IDE_CMD_IDENTIFY 0xEC   // 识别命令
#define IDE_CMD_DIAGNOSTIC 0x90 // 诊断命令

#define IDE_CMD_READ_UDMA 0xC8  // UDMA 读命令
#define IDE_CMD_WRITE_UDMA 0xCA // UDMA 写命令

#define IDE_CMD_PIDENTIFY 0xA1 // 识别 PACKET 命令
#define IDE_CMD_PACKET 0xA0    // PACKET 命令

// ATAPI 命令
#define IDE_ATAPI_CMD_REQUESTSENSE 0x03
#define IDE_ATAPI_CMD_READCAPICITY 0x25
#define IDE_ATAPI_CMD_READ10 0x28

#define IDE_ATAPI_FEATURE_PIO 0
#define IDE_ATAPI_FEATURE_DMA 1

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA
#define IDE_SEL_MASK 0b10110000   // CHS 模式 MASK

#define IDE_INTERFACE_UNKNOWN 0
#define IDE_INTERFACE_ATA 1
#define IDE_INTERFACE_ATAPI 2

// 总线主控寄存器偏移
#define BM_COMMAND_REG 0 // 命令寄存器偏移
#define BM_STATUS_REG 2  // 状态寄存器偏移
#define BM_PRD_ADDR 4    // PRD 地址寄存器偏移

// 总线主控命令寄存器
#define BM_CR_STOP 0x00  // 终止传输
#define BM_CR_START 0x01 // 开始传输
#define BM_CR_WRITE 0x00 // 主控写磁盘
#define BM_CR_READ 0x08  // 主控读磁盘

// 总线主控状态寄存器
#define BM_SR_ACT 0x01     // 激活
#define BM_SR_ERR 0x02     // 错误
#define BM_SR_INT 0x04     // 中断信号生成
#define BM_SR_DRV0 0x20    // 驱动器 0 可以使用 DMA 方式
#define BM_SR_DRV1 0x40    // 驱动器 1 可以使用 DMA 方式
#define BM_SR_SIMPLEX 0x80 // 仅单纯形操作

#define IDE_LAST_PRD 0x80000000 // 最后一个描述符

