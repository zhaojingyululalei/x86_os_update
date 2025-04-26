
#include "net/netif.h"
#include "rtl8139.h"
#include "irq/irq.h"
#include "printk.h"
#include "net/net_tools/package.h"
#include "cpu_instr.h"
static rtl8139_priv_t *priv;
struct _netif_t *netif;

// 读取PCI控制字
uint32_t pci_read_config_dword(int busno, int devno, int funcno, int addr)
{
    outl(PCI_CONFIG_ADDR, ((uint32_t)0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
    return inl(PCI_CONFIG_DATA);
}

// 写PCI控制字
void pci_write_config_dword(int busno, int devno, int funcno, int addr, uint32_t value)
{
    outl(PCI_CONFIG_ADDR, ((uint32_t)0x80000000 | (busno << 16) | (devno << 11) | (funcno << 8) | addr));
    outl(PCI_CONFIG_DATA, value);
}

// 开始PCI设备
void pci_enable_busmastering(int busno, int devno, int funcno)
{
    uint32_t value;

    value = pci_read_config_dword(busno, devno, funcno, PCI_CONFIG_CMD_STAT);
    value |= 0x00000004;
    pci_write_config_dword(busno, devno, funcno, PCI_CONFIG_CMD_STAT, value);
}

/**
 * @brief 硬件复位RTL8139
 */
static void rtl8139_reset(void)
{
    outb(priv->ioaddr + CR, CR_RST); // 复位命令
    for (int i = 100000; i > 0; i--)
    {
        if (!((inb(priv->ioaddr + CR) & CR_RST)))
        {
            NET_DRIVE_DBG_PRINT("rtl8139 reset complete\r\n");
            return;
        }
    }
}

/**
 * 发送配置
 */
static void rtl8139_setup_tx(void)
{
    uint8_t cr = inb(priv->ioaddr + CR);

    // 打开发送允许位
    outb(priv->ioaddr + CR, cr | CR_TE);

    // 配置发送相关的模型，960ns用于100mbps，添加CRC，DMA 1024B
    outl(priv->ioaddr + TCR, TCR_IFG_96 | TCR_MXDMA_1024);

    // 配置发送缓存,共4个缓冲区，并设备各个缓冲区首地址TSAD0-TSAD3
    for (int i = 0; i < R8139DN_TX_DESC_NB; i++)
    {
        uint32_t addr = (uint32_t)(priv->tx.buffer + i * R8139DN_TX_DESC_SIZE);
        priv->tx.data[i] = (uint8_t *)addr;
        outl(priv->ioaddr + TSAD0 + i * TSAD_GAP, addr);
    }

    // 指向第0个缓存包
    priv->tx.curr = priv->tx.dirty = 0;
    priv->tx.free_count = R8139DN_TX_DESC_NB;
}

/**
 * 接收配置
 */
static void rtl8139_setup_rx(void)
{
    //  先禁止接收
    uint8_t cr = inb(priv->ioaddr + CR);
    outb(priv->ioaddr + CR, cr & ~CR_RE);

    priv->rx.curr = 0;

    // 设置接收缓存
    priv->rx.data = priv->rx.buffer;

    // Multiple Interrupt Select Register?
    outw(priv->ioaddr + MULINT, 0);

    // 接收缓存起始地址
    outl(priv->ioaddr + RBSTART, (uint32_t)priv->rx.data);

    // 接收配置：广播+单播匹配、DMA bust 1024？接收缓存长16KB+16字节
    outl(priv->ioaddr + RCR, RCR_MXDMA_1024 | RCR_APM | RCR_AB | RCR_RBLEN_16K | RCR_WRAP);

    // 允许接收数据
    cr = inb(priv->ioaddr + CR);
    outb(priv->ioaddr + CR, cr | CR_RE);
}

/**
 * @brief 遍历RTL8139
 * 参考资料：https://wiki.osdev.org/PCI
 */
static int rtl8139_probe(int index)
{
    int total_found = 0;

    // 遍历256个bus
    for (int bus = 0; bus != 0xff; bus++)
    {
        // 遍历每条bus上最多32个设备
        for (int device = 0; device < 32; device++)
        {
            // 遍历每个设备上的8个功能号
            for (int func = 0; func < 8; func++)
            {

                // read the first dword (index 0) from the PCI configuration space
                // dword 0 contains the vendor and device values from the PCI configuration space
                uint32_t data = pci_read_config_dword(bus, device, func, PCI_CONFIG_ID);
                if (data == 0xffffffff)
                {
                    continue;
                }

                // 取厂商和设备号，不是RTL8139则跳过
                uint16_t device_value = (data >> 16);
                uint16_t vendor = data & 0xFFFF;
                if ((vendor != 0x10ec) || (device_value != 0x8139))
                {
                    continue;
                }

                // 取指定的序号
                if (total_found++ != index)
                {
                    continue;
                }

                // 取中断相关的配置, 测试读的是值为IRQ11(0xB)
                data = pci_read_config_dword(bus, device, func, PCI_CONFIG_INT);

                // 获取IO地址
                priv->bus = bus;
                priv->device = device;
                priv->func = func;
                priv->irq = (data & 0xF) + IRQ0_BASE;
                priv->ioaddr = pci_read_config_dword(bus, device, func, PCI_CONFIG_BASE_ADDR) & ~3;
                NET_DRIVE_DBG_PRINT("RTL8139 found! bus: %d, device: %d, func: %d, ioaddr: %x\r\n",
                           bus, device, func, priv->ioaddr);
                return 1;
            }
        }
    }
    return 0;
}

/**
 * 网络设备打开
 */
static int rtl8139_open(struct _netif_t *p_netif, void *ops_data)
{
    static int idx = 0;

    priv = (rtl8139_priv_t *)ops_data;
    netif = p_netif;

    // 遍历查找网络接口
    int find = rtl8139_probe(idx);
    if (find == 0)
    {
        NET_DRIVE_DBG_PRINT("open netdev%d failed: rtl8139\r\n", idx);
        return -1;
    }

    // 启动PCI总线，以便DMA能够传输网卡数据
    pci_enable_busmastering(priv->bus, priv->device, priv->func);

    // 给网卡通电
    outb(priv->ioaddr + CONFIG1, 0);

    // 开始8139, 使LWAKE + LWPTN变高
    outb(priv->ioaddr + CONFIG1, 0x0);
    NET_DRIVE_DBG_PRINT("enable rtl8139\r\n");

    // 复位网卡
    rtl8139_reset();

    // 配置复位
    rtl8139_setup_tx();
    rtl8139_setup_rx();

    // 读取mac地址。这里读取的值应当和qemu中网卡中配置的相同
    netif->hwaddr.addr[0] = inb(priv->ioaddr + 0);
    netif->hwaddr.addr[1] = inb(priv->ioaddr + 1);
    netif->hwaddr.addr[2] = inb(priv->ioaddr + 2);
    netif->hwaddr.addr[3] = inb(priv->ioaddr + 3);
    netif->hwaddr.addr[4] = inb(priv->ioaddr + 4);
    netif->hwaddr.addr[5] = inb(priv->ioaddr + 5);

    netif->info.type = NETIF_TYPE_ETH;
    netif->mtu = ETHER_MTU_DEFAULT;

    // 系统还没初始化完毕, 启用所有与接收和发送相关的中断
    outw(priv->ioaddr + IMR, 0xFFFF);
    interupt_install(priv->irq, exception_handler_rtl8139);
    irq_enable(priv->irq);

    return 0;
open_error:
    NET_DRIVE_DBG_PRINT("rtl8139 open error\r\n");
    return -3;
}

/**
 * 启动发送
 */
static int rtl8139_send(struct _netif_t *netif)
{
    // 如果发送缓存已经满，即写入比较快，退出，让中断自行处理发送过程
    if (priv->tx.free_count <= 0)
    {
        dbg_warning("tx free_cnt == 0\r\n");
        return 0;
    }

    // 取数据包
    pkg_t *buf = netif_get_pkg_from_outq(netif, -1);
    if(!buf)
    {
        return -1;
    }
    NET_DRIVE_DBG_PRINT("rtl8139 send packeet: %d\r\n", buf->total);

    // 禁止响应中断，以便进行一定程序的保护
    irq_state_t state = irq_enter_protection();

    // 写入发送缓存，不必调整大小，上面已经调整好
    int len = buf->total;
    package_read(buf,priv->tx.data[priv->tx.curr],len);
    //package_collect(buf);

    // 启动整个发送过程, 只需要写TSD，TSAD(数据地址和长度)已经在发送时完成
    // 104 bytes of early TX threshold，104字节才开始发送??，以及发送的包的字节长度，清除OWN位
    uint32_t tx_flags = (1 << TSD_ERTXTH_SHIFT) | len;
    outl(priv->ioaddr + TSD0 + priv->tx.curr * TSD_GAP, tx_flags);

    // 取下一个描述符f地址
    priv->tx.curr += 1;
    if (priv->tx.curr >= R8139DN_TX_DESC_NB)
    {
        priv->tx.curr = 0;
    }

    // 减小空闲计数
    priv->tx.free_count--;

    irq_leave_protection(state);
    return 0;
}
static int rtl8139_close(netif_t* netif)
{
    irq_state_t state = irq_enter_protection();
    //  禁止接收
    uint8_t cr = inb(priv->ioaddr + CR);
    outb(priv->ioaddr + CR, cr & ~CR_RE);

    //禁止发送
    cr = inb(priv->ioaddr + CR);
    outb(priv->ioaddr + CR, cr & ~CR_TE);

    irq_disable(priv->irq);
    irq_leave_protection(state);
}
/**
 * @brief RTL8139中断处理
 * 如果在中断处理中区分当前所用的是哪个网卡？
 */
void do_handler_rtl8139(void)
{
    NET_DRIVE_DBG_PRINT( "rtl8139 interrupt in\r\n");

    // 先读出所有中断，然后清除中断寄存器的状态
    uint16_t isr = inw(priv->ioaddr + ISR);
    outw(priv->ioaddr + ISR, 0xFFFF);

    // 先处理发送完成和错误
    if ((isr & INT_TOK) || (isr & INT_TER))
    {
        NET_DRIVE_DBG_PRINT( "tx interrupt\r\n");

        // 尽可能取出多的数据包，然后将缓存填满
        while (priv->tx.free_count < 4)
        {
            // 取dirty位置的发送状态, 如果发送未完成，即无法写了，那么就退出
            uint32_t tsd = inl(priv->ioaddr + TSD0 + (priv->tx.dirty * TSD_GAP));
            if (!(tsd & (TSD_TOK | TSD_TABT | TSD_TUN)))
            {
                NET_DRIVE_DBG_PRINT( "send error, tsd: %d\r\n", tsd);
                break;
            }

            // 增加计数
            priv->tx.free_count++;
            if (++priv->tx.dirty >= R8139DN_TX_DESC_NB)
            {
                priv->tx.dirty = 0;
            }

            // 尽可能取出多的数据包，然后将缓存填满
            pkg_t* buf = netif_get_pkg_from_outq(netif,-1);
            if (buf)
            {
                priv->tx.free_count--;

                // 写入发送缓存，不必调整大小，上面已经调整好
                int len = buf->total;
                package_read(buf,priv->tx.data[priv->tx.curr],len);
                //package_collect(buf);

                // 启动整个发送过程, 只需要写TSD，TSAD已经在发送时完成
                // 104 bytes of early TX threshold，104字节才开始发送??，以及发送的包的字节长度，清除OWN位
                uint32_t tx_flags = (1 << TSD_ERTXTH_SHIFT) | len;
                outl(priv->ioaddr + TSD0 + priv->tx.curr * TSD_GAP, tx_flags);

                // 取下一个描述符f地址
                priv->tx.curr += 1;
                if (priv->tx.curr >= R8139DN_TX_DESC_NB)
                {
                    priv->tx.curr = 0;
                }
            }
        }
    }

    // 对于接收的测试，可以ping本地广播来测试。否则windows上，ping有时会不发包，而是要过一段时间，难得等
    if (isr & INT_ROK)
    {
        NET_DRIVE_DBG_PRINT("rtl8139 recv ok\r\n");

        // 循环读取，直到缓存为空
        while ((inb(priv->ioaddr + CR) & CR_BUFE) == 0)
        {
            rtl8139_rpkthdr_t *hdr = (rtl8139_rpkthdr_t *)(priv->rx.data + priv->rx.curr);
            int pkt_size = hdr->size - CRC_LEN;

            // 进行一些简单的错误检查，有问题的包就不要写入接收队列了
            if (hdr->FAE || hdr->CRC || hdr->LONG || hdr->RUNT || hdr->ISE)
            {
                dbg_warning("%s: rcv pkt error: \r\n", netif->info.name);
                // 重新对接收进行设置，重启。还是忽略这个包，还是忽略吧
                rtl8139_setup_rx();
            }
            else if ((pkt_size < 0) || (pkt_size >= R8139DN_MAX_ETH_SIZE))
            {
                dbg_warning("%s: size error: \r\n", netif->info.name);
                // 重新对接收进行设置，重启。还是忽略这个包，还是忽略吧
                rtl8139_setup_rx();
            }
            else
            {
                // 分配包缓存空间
                pkg_t* buf = package_alloc(pkt_size);
                if (buf)
                {
                    // 这里要考虑跨包拷贝的情况
                    if (priv->rx.curr + hdr->size > R8139DN_RX_DMA_SIZE)
                    {
                        // 分割了两块空间
                        int count = R8139DN_RX_DMA_SIZE - priv->rx.curr - sizeof(rtl8139_rpkthdr_t);
                        package_write(buf, priv->rx.data + priv->rx.curr + sizeof(rtl8139_rpkthdr_t), count); // 第一部分到尾部
                        package_write(buf, priv->rx.data, pkt_size - count);
                        NET_DRIVE_DBG_PRINT( "copy twice, rx.curr: %d\r\n", priv->rx.curr);
                    }
                    else
                    {
                        // 跳过开头的4个字节的接收状态头
                        package_write(buf,priv->rx.data + priv->rx.curr + sizeof(rtl8139_rpkthdr_t), pkt_size);
                    }

                    // 将收到的包发送队列
                    netif_put_pkg_into_inq(netif,buf,-1);
                }
                else
                {
                    // 包不够则丢弃
                    dbg_warning("no buffer\r\n");
                }

                // 移至下一个包
                priv->rx.curr = (priv->rx.curr + hdr->size + sizeof(rtl8139_rpkthdr_t) + 3) & ~3; // 4字节对齐
                NET_DRIVE_DBG_PRINT( "recv end, rx.curr: %d\r\n", priv->rx.curr);

                // 调整curr在整个有效的缓存空间内
                if (priv->rx.curr > R8139DN_RX_BUFLEN)
                {
                    NET_DRIVE_DBG_PRINT( "priv->rx.curr >= R8139DN_RX_DMA_SIZE\r\n");
                    priv->rx.curr -= R8139DN_RX_BUFLEN;
                }

                // 更新接收地址，通知网卡可以继续接收数据
                outw(priv->ioaddr + CAPR, priv->rx.curr - 16);
            }
        }
    }

    // 别忘了这个，不然中断不会响应
    pic_send_eoi(priv->irq);
}

/**
 * pcap驱动结构
 */
const netif_ops_t netdev8139_ops = {
    .open = rtl8139_open,
    .send = rtl8139_send,
    .close = rtl8139_close,
};
