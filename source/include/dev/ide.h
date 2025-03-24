#ifndef __IDE_H
#define __IDE_H
#include "types.h"
#include "ipc/mutex.h"
#include "ipc/semaphore.h"

#define SECTOR_SIZE 512     // 扇区大小

#define IDE_CTRL_NR 2 // 控制器数量，固定为 2
#define IDE_DISK_NR 2 // 每个控制器可挂磁盘数量，固定为 2



typedef struct part_entry_t
{
    uint8_t bootable;             // 引导标志
    uint8_t start_head;           // 分区起始磁头号
    uint8_t start_sector : 6;     // 分区起始扇区号
    uint16_t start_cylinder : 10; // 分区起始柱面号
    uint8_t system;               // 分区类型字节
    uint8_t end_head;             // 分区的结束磁头号
    uint8_t end_sector : 6;       // 分区结束扇区号
    uint16_t end_cylinder : 10;   // 分区结束柱面号
    uint32_t start;               // 分区起始物理扇区号 LBA
    uint32_t count;               // 分区占用的扇区数
} __attribute__((packed)) part_entry_t;

typedef struct boot_sector_t
{
    uint8_t code[446];
    part_entry_t entry[4];
    uint16_t signature;
} __attribute__((packed)) boot_sector_t;

struct ide_disk_t;
typedef struct ide_part_t
{
    char name[8];            // 分区名称
    struct ide_disk_t *disk; // 磁盘指针
    uint32_t system;              // 分区类型
    uint32_t start;               // 分区起始物理扇区号 LBA
    uint32_t count;               // 分区占用的扇区数
} ide_part_t;

//PRDT表项
typedef struct ide_prd_t
{
    uint32_t addr;
    uint32_t len;
} ide_prd_t;

// IDE 磁盘
struct ide_ctrl_t;
typedef struct ide_disk_t
{
    char name[8];                  // 磁盘名称
    struct ide_ctrl_t *ctrl;       // 控制器指针
    uint8_t selector;                   // 磁盘选择
    bool master;                   // 主盘 从盘
    uint32_t total_lba;                 // 可用扇区数量
    uint32_t cylinders;                 // 柱面数
    uint32_t heads;                     // 磁头数
    uint32_t sectors;                   // 扇区数
    uint32_t interface;                 // 磁盘类型
    uint32_t sector_size;               // 扇区大小
    ide_part_t *parts;                  // 硬盘分区
} ide_disk_t;

// IDE 控制器
typedef struct ide_ctrl_t
{
    char name[8];                  // 控制器名称
    mutex_t mutex;                   // 控制器锁
    sem_t   sem;
    enum{
        IDE_TYPE_PIO,
        IDE_TYPE_UDMA
    } iotype;                    // 设备 IO 类型
    uint16_t iobase;                    // IO 寄存器基址
    uint16_t bmbase;                    // PCI 总线主控寄存器基地址
    ide_disk_t disks[IDE_DISK_NR]; // 磁盘
    ide_disk_t *active;            // 当前选择的磁盘
    uint8_t control;                    // 控制字节
    ide_prd_t prd;                 // Physical Region Descriptor
} ide_ctrl_t;

int ide_pio_write(ide_disk_t *disk, void *buf, uint8_t count, uint32_t lba);
int ide_pio_read(ide_disk_t *disk, void *buf, uint8_t count, uint32_t lba);
#endif