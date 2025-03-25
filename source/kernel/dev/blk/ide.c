#include "dev/ide.h"
#include "irq/irq.h"
#include "cpu_instr.h"
#include "dev/pci.h"
#include "printk.h"
#include "dev/ide.h"
#include "mem/kmalloc.h"
#include "cpu_cfg.h"
#include "string.h"
#include "task/task.h"
#include "time/clock.h"
#include "errno.h"
#define IDE_TIMEOUT 2000

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

typedef struct ide_params_t
{
    uint16_t config;                 // 0 General configuration bits
    uint16_t cylinders;              // 01 cylinders
    uint16_t RESERVED_1;             // 02
    uint16_t heads;                  // 03 heads
    uint16_t RESERVED_2[5 - 3];      // 05
    uint16_t sectors;                // 06 sectors per track
    uint16_t RESERVED_3[9 - 6];      // 09
    uint8_t serial[20];              // 10 ~ 19 序列号
    uint16_t RESERVED_4[22 - 19];    // 10 ~ 22
    uint8_t firmware[8];             // 23 ~ 26 固件版本
    uint8_t model[40];               // 27 ~ 46 模型数
    uint8_t drq_sectors;             // 47 扇区数量
    uint8_t RESERVED_5[3];           // 48
    uint16_t capabilities;           // 49 能力
    uint16_t RESERVED_6[59 - 49];    // 50 ~ 59
    uint32_t total_lba;              // 60 ~ 61
    uint16_t RESERVED;               // 62
    uint16_t mdma_mode;              // 63
    uint8_t RESERVED_11;             // 64
    uint8_t pio_mode;                // 64
    uint16_t RESERVED_7[79 - 64];    // 65 ~ 79 参见 ATA specification
    uint16_t major_version;          // 80 主版本
    uint16_t minor_version;          // 81 副版本
    uint16_t commmand_sets[87 - 81]; // 82 ~ 87 支持的命令集
    uint16_t RESERVED_8[118 - 87];   // 88 ~ 118
    uint16_t support_settings;       // 119
    uint16_t enable_settings;        // 120
    uint16_t RESERVED_9[221 - 120];  // 221
    uint16_t transport_major;        // 222
    uint16_t transport_minor;        // 223
    uint16_t RESERVED_10[254 - 223]; // 254
    uint16_t integrity;              // 校验和
} __attribute__((packed)) ide_params_t;

ide_ctrl_t controllers[IDE_CTRL_NR];
void do_handler_ide(exception_frame_t *frame)
{
    int vector = frame->num;
    pic_send_eoi(vector);

    // 得到中断向量对应的控制器
    ide_ctrl_t *ctrl = &controllers[vector - IRQ_HARDDISK - 0x20];

    // 读取常规状态寄存器，表示中断处理结束
    uint8_t state = inb(ctrl->iobase + IDE_STATUS);
    dbg_info("harddisk interrupt vector %d state 0x%x\r\n", vector, state);
    sys_sem_notify(&ctrl->sem);
}
void do_handler_ide_prim(exception_frame_t *frame)
{
    do_handler_ide(frame);
}
void do_handler_ide_slav(exception_frame_t *frame)
{
    do_handler_ide(frame);
}
/**
 * @brief 硬盘延时
 */
static void ide_delay()
{
    sys_sleep(25);
}

// 选择磁盘
static void ide_select_drive(ide_disk_t *disk)
{
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->active = disk;
}
// 选择扇区
static void ide_select_sector(ide_disk_t *disk, uint32_t lba, uint8_t count)
{
    // 输出功能，可省略
    outb(disk->ctrl->iobase + IDE_FEATURE, 0);

    // 读写扇区数量
    outb(disk->ctrl->iobase + IDE_SECTOR, count);

    // LBA 低字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);
    // LBA 中字节
    outb(disk->ctrl->iobase + IDE_LBA_MID, (lba >> 8) & 0xff);
    // LBA 高字节
    outb(disk->ctrl->iobase + IDE_LBA_HIGH, (lba >> 16) & 0xff);

    // LBA 最高四位 + 磁盘选择
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf) | disk->selector);
    disk->ctrl->active = disk;
}

// 从磁盘读取一个扇区到 buf
static void ide_pio_read_sector(ide_disk_t *disk, uint16_t *buf)
{
    for (size_t i = 0; i < (disk->sector_size / 2); i++)
    {
        buf[i] = inw(disk->ctrl->iobase + IDE_DATA);
    }
}

// 从 buf 写入一个扇区到磁盘
static void ide_pio_write_sector(ide_disk_t *disk, uint16_t *buf)
{
    for (size_t i = 0; i < (disk->sector_size / 2); i++)
    {
        outw(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}
static int ide_busy_wait(ide_ctrl_t *ctrl, uint8_t mask, int timeout_ms);
/**
 * @brief 重置硬盘控制器
 */
static int ide_reset_controller(ide_ctrl_t *ctrl)
{
    outb(ctrl->iobase + IDE_CONTROL, IDE_CTRL_SRST);
    ide_delay();
    outb(ctrl->iobase + IDE_CONTROL, ctrl->control);
    return ide_busy_wait(ctrl, IDE_SR_NULL, IDE_TIMEOUT);
}
static void ide_error(ide_ctrl_t *ctrl)
{
    uint8_t error = inb(ctrl->iobase + IDE_ERR);
    if (error & IDE_ER_BBK)
        dbg_warning("bad block\r\n");
    if (error & IDE_ER_UNC)
        dbg_warning("uncorrectable data\r\n");
    if (error & IDE_ER_MC)
        dbg_warning("media change\r\n");
    if (error & IDE_ER_IDNF)
        dbg_warning("id not found\r\n");
    if (error & IDE_ER_MCR)
        dbg_warning("media change requested\r\n");
    if (error & IDE_ER_ABRT)
        dbg_warning("abort\r\n");
    if (error & IDE_ER_TK0NF)
        dbg_warning("track 0 not found\r\n");
    if (error & IDE_ER_AMNF)
        dbg_warning("address mark not found\r\n");
}

static int ide_busy_wait(ide_ctrl_t *ctrl, uint8_t mask, int timeout_ms)
{
    int expires = get_systick() + timeout_ms;
    while (true)
    {
        int cur_systick = get_systick();
        // 超时
        if (timeout_ms > 0 && cur_systick - expires > 0)
        {
            return -ETIMEOUT;
        }

        // 从备用状态寄存器中读状态
        uint8_t state = inb(ctrl->iobase + IDE_ALT_STATUS);
        if (state & IDE_SR_ERR) // 有错误
        {
            ide_error(ctrl);
            ide_reset_controller(ctrl);
            return -EIO;
        }
        if (state & IDE_SR_BSY) // 驱动器忙
        {
            ide_delay();
            continue;
        }
        if ((state & mask) == mask) // 等待某个的状态完成
            return EOK;
    }
}

/**
 * @brief 磁盘探测
 */
static int ide_probe_device(ide_disk_t *disk)
{
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector & IDE_SEL_MASK);
    ide_delay();

    outb(disk->ctrl->iobase + IDE_SECTOR, 0x55);
    outb(disk->ctrl->iobase + IDE_CHS_SECTOR, 0xAA);

    outb(disk->ctrl->iobase + IDE_SECTOR, 0xAA);
    outb(disk->ctrl->iobase + IDE_CHS_SECTOR, 0x55);

    outb(disk->ctrl->iobase + IDE_SECTOR, 0x55);
    outb(disk->ctrl->iobase + IDE_CHS_SECTOR, 0xAA);

    uint8_t sector_count = inb(disk->ctrl->iobase + IDE_SECTOR);
    uint8_t sector_index = inb(disk->ctrl->iobase + IDE_CHS_SECTOR);

    if (sector_count == 0x55 && sector_index == 0xAA)
        return 0;
    return -1;
}

/**
 * @brief 探测磁盘接口类型
 */
static int ide_interface_type(ide_disk_t *disk)
{
    outb(disk->ctrl->iobase + IDE_COMMAND, IDE_CMD_DIAGNOSTIC);
    if (ide_busy_wait(disk->ctrl, IDE_SR_NULL, IDE_TIMEOUT) < 0)
        return IDE_INTERFACE_UNKNOWN;

    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector & IDE_SEL_MASK);
    ide_delay();

    uint8_t sector_count = inb(disk->ctrl->iobase + IDE_SECTOR);
    uint8_t sector_index = inb(disk->ctrl->iobase + IDE_LBA_LOW);
    if (sector_count != 1 || sector_index != 1)
        return IDE_INTERFACE_UNKNOWN;

    uint8_t cylinder_low = inb(disk->ctrl->iobase + IDE_CHS_CYL);
    uint8_t cylinder_high = inb(disk->ctrl->iobase + IDE_CHS_CYH);
    uint8_t state = inb(disk->ctrl->iobase + IDE_STATUS);

    if (cylinder_low == 0x14 && cylinder_high == 0xeb)
        return IDE_INTERFACE_ATAPI;

    if (cylinder_low == 0 && cylinder_high == 0 && state != 0)
        return IDE_INTERFACE_ATA;

    return IDE_INTERFACE_UNKNOWN;
}
// 转换字节序
static void ide_fixstrings(char *buf, uint32_t len)
{
    for (size_t i = 0; i < len; i += 2)
    {
        register char ch = buf[i];
        buf[i] = buf[i + 1];
        buf[i + 1] = ch;
    }
    buf[len - 1] = '\0';
}

/**
 * @brief 识别磁盘
 */
static int ide_identify(ide_disk_t *disk, uint16_t *buf)
{
    dbg_info("identifing disk %s...\r\n", disk->name);
    sys_mutex_lock(&disk->ctrl->mutex);
    ide_select_drive(disk);

    // ide_select_sector(disk, 0, 0);
    uint8_t cmd = IDE_CMD_IDENTIFY;
    if (disk->interface == IDE_INTERFACE_ATAPI)
    {
        cmd = IDE_CMD_PIDENTIFY;
    }

    outb(disk->ctrl->iobase + IDE_COMMAND, cmd);

    int ret = -1;
    if ((ret = ide_busy_wait(disk->ctrl, IDE_SR_NULL, IDE_TIMEOUT)) < EOK)
        goto rollback;

    ide_params_t *params = (ide_params_t *)buf;

    ide_pio_read_sector(disk, buf);

    ide_fixstrings(params->serial, sizeof(params->serial));
    dbg_info("disk %s serial number %s\r\n", disk->name, params->serial);

    ide_fixstrings(params->firmware, sizeof(params->firmware));
    dbg_info("disk %s firmware version %s\r\n", disk->name, params->firmware);

    ide_fixstrings(params->model, sizeof(params->model));
    dbg_info("disk %s model number %s\r\n", disk->name, params->model);

    if (disk->interface == IDE_INTERFACE_ATAPI)
    {
        ret = EOK;
        goto rollback;
    }

    if (params->total_lba == 0)
    {
        ret = -EIO;
        goto rollback;
    }
    dbg_info("disk %s total lba %d\r\n", disk->name, params->total_lba);

    disk->total_lba = params->total_lba;
    disk->cylinders = params->cylinders;
    disk->heads = params->heads;
    disk->sectors = params->sectors;
    ret = EOK;

rollback:
    sys_mutex_unlock(&disk->ctrl->mutex);
    return ret;
}
static void ide_controler_init(void)
{
    int iotype = IDE_TYPE_PIO;
    int bmbase = 0;
    // 通过classnode找到设备
    pci_device_t *device = pci_find_device_by_class(PCI_CLASS_STORAGE_IDE);
    if (device)
    {
        pci_bar_t bar;
        int ret = pci_find_bar(device, &bar, PCI_BAR_TYPE_IO);
        ASSERT(ret == 0);

        dbg_info("find dev 0x%x bar iobase 0x%x iosize %d\r\n",
                 device, bar.iobase, bar.size);

        // 允许dma
        pci_enable_busmastering(device);

        iotype = IDE_TYPE_UDMA;
        bmbase = bar.iobase;
    }

    uint16_t *buf = (uint16_t *)kmalloc(MEM_PAGE_SIZE);
    // 遍历每个通道上的每个磁盘
    for (int c = 0; c < IDE_CTRL_NR; c++)
    {
        ide_ctrl_t *ctrl = &controllers[c];
        sprintf(ctrl->name, "ide%u", c);
        sys_mutex_init(&ctrl->mutex);
        sys_sem_init(&ctrl->sem, 0);
        list_init(&ctrl->scatter_list.list);
        ctrl->active = NULL;
        ctrl->iotype = iotype;
        ctrl->bmbase = bmbase + c * 8;
        if (c) // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        }
        else // 主通道
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }
        // 读取控制字节，以后重置时使用
        ctrl->control = inb(ctrl->iobase + IDE_CONTROL);
        for (int d = 0; d < IDE_DISK_NR; d++)
        {
            ide_disk_t *disk = &ctrl->disks[d];
            // sda sdb sdc sdd
            sprintf(disk->name, "sd%c", 'a' + c * 2 + d);
            disk->ctrl = ctrl;
            if (d) // 从盘
            {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            }
            else // 主盘
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
            if (ide_probe_device(disk) < 0)
            {
                dbg_info("IDE device %s not exists...\r\n", disk->name);
                continue;
            }
            disk->interface = ide_interface_type(disk);
            dbg_info("IDE device %s type %d...\r\n", disk->name, disk->interface);
            if (disk->interface == IDE_INTERFACE_UNKNOWN)
                continue;
            if (disk->interface == IDE_INTERFACE_ATA)
            {
                disk->sector_size = SECTOR_SIZE;
                if (ide_identify(disk, buf) == EOK)
                {
                    // ide_part_init(disk, buf);
                    dbg_info("disk %s identify ok\r\n", disk->name);
                }
            }
        }
    }
    kfree(buf);
}

// PIO 方式读取磁盘
int ide_pio_read(ide_disk_t *disk, void *buf, uint8_t count, uint32_t lba)
{
    ASSERT(count > 0);

    ide_ctrl_t *ctrl = disk->ctrl;

    sys_mutex_lock(&ctrl->mutex);

    int ret = -EIO;

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    if ((ret = ide_busy_wait(ctrl, IDE_SR_DRDY, IDE_TIMEOUT)) < EOK)
    {
        sys_mutex_unlock(&ctrl->mutex);
        return ret;
    }

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送读命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0; i < count; i++)
    {
        // 阻塞自己等待中断的到来，等待磁盘准备数据
        sys_mutex_unlock(&ctrl->mutex);
        ret = sys_sem_timewait(&ctrl->sem, IDE_TIMEOUT / 1000);
        if (ret < 0)
        {
            return -ETIMEOUT; // 等了半天，数据也没准备好
        }
        sys_mutex_lock(&ctrl->mutex);

        if ((ret = ide_busy_wait(ctrl, IDE_SR_DRQ, IDE_TIMEOUT)) < EOK)
        {
            sys_mutex_unlock(&ctrl->mutex);
            return ret;
        }

        uint32_t offset = ((uint32_t)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (uint16_t *)offset);
    }
    ret = EOK;
    sys_mutex_unlock(&ctrl->mutex);
    return ret;
}

// PIO 方式写磁盘
int ide_pio_write(ide_disk_t *disk, void *buf, uint8_t count, uint32_t lba)
{
    ASSERT(count > 0);

    ide_ctrl_t *ctrl = disk->ctrl;

    sys_mutex_lock(&ctrl->mutex);

    int ret = EOK;

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    if ((ret = ide_busy_wait(ctrl, IDE_SR_DRDY, IDE_TIMEOUT)) < EOK)
    {
        sys_mutex_unlock(&ctrl->mutex);
        return ret;
    }

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送写命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0; i < count; i++)
    {
        uint32_t offset = ((uint32_t)buf + i * SECTOR_SIZE);
        ide_pio_write_sector(disk, (uint16_t *)offset);

        // 阻塞自己等待中断的到来，等待磁盘准备数据
        sys_mutex_unlock(&ctrl->mutex);
        ret = sys_sem_timewait(&ctrl->sem, IDE_TIMEOUT / 1000);
        if (ret < 0)
        {
            return -ETIMEOUT; // 等了半天，数据也没准备好
        }
        sys_mutex_lock(&ctrl->mutex);

        if ((ret = ide_busy_wait(ctrl, IDE_SR_NULL, IDE_TIMEOUT)) < EOK)
        {
            sys_mutex_unlock(&ctrl->mutex);
            return ret;
        }
    }
    ret = EOK;
    sys_mutex_unlock(&ctrl->mutex);
    return ret;
}



// 修改后的DMA设置函数
static int ide_setup_dma(ide_ctrl_t *ctrl, int cmd, void *buf, uint32_t len) {
    // 1. 创建分散列表映射
    int ret = scatter_list_map(&ctrl->scatter_list, (vm_addr_t)buf, len);
    if (ret < 0) {
        return ret;
    }

    // 2. 分配PRD表
    int nents = list_count(&ctrl->scatter_list.list);
    ctrl->prdt = dma_alloc_coherent(sizeof(ide_prd_t) * nents);
    if (!ctrl->prdt) {
        scatter_list_destory(&ctrl->scatter_list);
        return -ENOMEM;
    }

    // 3. 填充PRD表
    int i = 0;
    list_node_t *node = list_first(&ctrl->scatter_list.list);
    while (node) {
        scatter_node_t *snode = list_node_parent(node, scatter_node_t, node);
        ctrl->prdt[i].addr = snode->phstart;
        ctrl->prdt[i].len = snode->len;
        if (i == nents - 1) {
            ctrl->prdt[i].len |= IDE_LAST_PRD;
        }
        i++;
        node = list_node_next(node);
    }

    // 4. 设置PRD表地址
    outl(ctrl->bmbase + BM_PRD_ADDR, (uint32_t)ctrl->prdt);

    // 5. 设置命令和中断
    outb(ctrl->bmbase + BM_COMMAND_REG, cmd | BM_CR_STOP);
    outb(ctrl->bmbase + BM_STATUS_REG, 
         inb(ctrl->bmbase + BM_STATUS_REG) | BM_SR_INT | BM_SR_ERR);

    return 0;
}



// 启动 DMA
static void ide_start_dma(ide_ctrl_t *ctrl)
{
    outb(ctrl->bmbase + BM_COMMAND_REG, inb(ctrl->bmbase + BM_COMMAND_REG) | BM_CR_START);
}

static int ide_stop_dma(ide_ctrl_t *ctrl)
{
    // 停止 DMA
    outb(ctrl->bmbase + BM_COMMAND_REG, inb(ctrl->bmbase + BM_COMMAND_REG) & (~BM_CR_START));

    // 获取 DMA 状态
    uint8_t status = inb(ctrl->bmbase + BM_STATUS_REG);

    // 清除中断和错误位
    outb(ctrl->bmbase + BM_STATUS_REG, status | BM_SR_INT | BM_SR_ERR);

    // 检测错误
    if (status & BM_SR_ERR)
    {
        dbg_error("IDE dma error %02X\n", status);
        return -EIO;
    }
    return EOK;
}
// 修改后的UDMA读取函数
int ide_udma_read(ide_disk_t *disk, void *buf, uint8_t count, uint32_t lba) {
    int ret = 0;
    ide_ctrl_t *ctrl = disk->ctrl;

    sys_mutex_lock(&ctrl->mutex);

    // 设置DMA传输
    ret = ide_setup_dma(ctrl, BM_CR_READ, buf, count * SECTOR_SIZE);
    if (ret < 0) {
        sys_mutex_unlock(&ctrl->mutex);
        return ret;
    }

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 设置UDMA读命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ_UDMA);

    // 启动DMA
    ide_start_dma(ctrl);

    // 等待传输完成
    sys_mutex_unlock(&ctrl->mutex);
    ret = sys_sem_timewait(&ctrl->sem, IDE_TIMEOUT / 1000);
    if (ret < 0) {
        sys_mutex_lock(&ctrl->mutex);
        ide_stop_dma(ctrl);
        sys_mutex_unlock(&ctrl->mutex);
        return -ETIMEOUT;
    }

    sys_mutex_lock(&ctrl->mutex);
    ret = ide_stop_dma(ctrl);
    ASSERT(ret == EOK);
    // 释放资源
    if (ctrl->prdt) {
        dma_free_coherent((ph_addr_t)ctrl->prdt, sizeof(ide_prd_t) * list_count(&ctrl->scatter_list.list));
        ctrl->prdt = NULL;
    }
    scatter_list_destory(&ctrl->scatter_list);

    sys_mutex_unlock(&ctrl->mutex);
    return ret;
}

int ide_udma_write(ide_disk_t *disk, void *buf, uint8_t count, uint32_t lba) {
    int ret = EOK;
    ide_ctrl_t *ctrl = disk->ctrl;
    size_t len = count * SECTOR_SIZE;

    sys_mutex_lock(&ctrl->mutex);

    // 1. 设置DMA传输
    ret = ide_setup_dma(ctrl, BM_CR_WRITE, buf, len);
    if (ret < 0) {
        sys_mutex_unlock(&ctrl->mutex);
        return ret;
    }

    // 2. 选择扇区
    ide_select_sector(disk, lba, count);

    // 3. 设置UDMA写命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE_UDMA);

    // 4. 启动DMA传输
    ide_start_dma(ctrl);

    // 5. 等待传输完成
    sys_mutex_unlock(&ctrl->mutex);
    ret = sys_sem_timewait(&ctrl->sem, IDE_TIMEOUT / 1000);
    if (ret < 0) {
        sys_mutex_lock(&ctrl->mutex);
        ide_stop_dma(ctrl);
        // 清理资源
        if (ctrl->prdt) {
            dma_free_coherent((ph_addr_t)ctrl->prdt, 
                            sizeof(ide_prd_t) * list_count(&ctrl->scatter_list.list));
            ctrl->prdt = NULL;
        }
        scatter_list_destory(&ctrl->scatter_list);
        sys_mutex_unlock(&ctrl->mutex);
        return -ETIMEOUT;
    }

    sys_mutex_lock(&ctrl->mutex);
    
    // 6. 停止DMA并检查状态
    ret = ide_stop_dma(ctrl);
    ASSERT(ret == EOK);
    // 7. 释放资源
    if (ctrl->prdt) {
        dma_free_coherent((ph_addr_t)ctrl->prdt, 
                        sizeof(ide_prd_t) * list_count(&ctrl->scatter_list.list));
        ctrl->prdt = NULL;
    }
    scatter_list_destory(&ctrl->scatter_list);

    sys_mutex_unlock(&ctrl->mutex);
    return ret;
}

static void pio_read_write_test(void)
{
    ide_disk_t *disk = &controllers[1].disks[0];
    uint8_t *write_buf = kmalloc(SECTOR_SIZE * 2);
    uint8_t *read_buf = kmalloc(SECTOR_SIZE * 2);
    for (int i = 0; i < SECTOR_SIZE * 2; i++)
    {
        if (i % 2 == 0)
        {
            write_buf[i] = '0x55';
        }
        else
        {
            write_buf[i] = '0xaa';
        }
    }

    ide_pio_write(disk, write_buf, 2, 0);
    ide_pio_read(disk, read_buf, 2, 0);
    int ret = strncmp(write_buf, read_buf, SECTOR_SIZE * 2);
    if (ret == 0)
    {
        dbg_info("pio read write successful\r\n");
    }
    else
    {
        dbg_info("pio read write failed\r\n");
    }
    kfree(write_buf);
    kfree(read_buf);
}
static void dma_read_write_test(void){
    ide_disk_t *disk = &controllers[1].disks[1];
    int sector_cnt = 16;
    uint8_t *write_buf = kmalloc(SECTOR_SIZE * sector_cnt);
    uint8_t *read_buf = kmalloc(SECTOR_SIZE * sector_cnt);
    for (int i = 0; i < SECTOR_SIZE * 2; i++)
    {
        if (i % 2 == 0)
        {
            write_buf[i] = '0x66';
        }
        else
        {
            write_buf[i] = '0xbb';
        }
    }

    ide_udma_write(disk, write_buf, sector_cnt, 0);
    ide_udma_read(disk, read_buf, sector_cnt, 0);
    int ret = strncmp(write_buf, read_buf, SECTOR_SIZE * sector_cnt);
    if (ret == 0)
    {
        dbg_info("dma read write successful\r\n");
    }
    else
    {
        dbg_info("dma read write failed\r\n");
    }
    kfree(write_buf);
    kfree(read_buf);
}
extern void exception_handler_ide_prim(void);
extern void exception_handler_ide_slav(void);
void ide_init(void)
{
    // 注册硬盘中断
    interupt_install(IRQ14_HARDDISK_PRIMARY, (irq_handler_t)exception_handler_ide_prim);
    irq_enable(IRQ14_HARDDISK_PRIMARY);
    interupt_install(IRQ15_HARDDISK_SLAVE, (irq_handler_t)exception_handler_ide_slav);
    irq_enable(IRQ15_HARDDISK_SLAVE);

    ide_controler_init();
    dma_read_write_test();
}