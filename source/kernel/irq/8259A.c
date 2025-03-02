
#include "cpu_instr.h"
#include "irq/8259A.h"

/**
 * @brief 初始化中断控制器
 */
void init_pic(void) {
    /*主片配置*/
    // 边缘触发，级联，需要配置icw4, 8086模式
    outb(PIC0_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    // 对应的中断号起始序号0x20
    outb(PIC0_ICW2, IRQ_PIC_START);
    // 主片IRQ2有从片
    outb(PIC0_ICW3, 1 << 2);
    // 普通全嵌套、非缓冲、非自动结束、8086模式
    outb(PIC0_ICW4, PIC_ICW4_8086 | PIC_ICW4_SNFM);

    /*从片配置*/
    // 边缘触发，级联，需要配置icw4, 8086模式
    outb(PIC1_ICW1, PIC_ICW1_ICW4 | PIC_ICW1_ALWAYS_1);
    // 起始中断序号，要加上8
    outb(PIC1_ICW2, IRQ_PIC_START + 8);
    // 没有从片，连接到主片的IRQ2上
    outb(PIC1_ICW3, 2);
    // “一般嵌套”、非缓冲、非自动结束、8086模式
    outb(PIC1_ICW4, PIC_ICW4_8086);
    // 禁止所有中断, 允许从PIC1传来的中断
    outb(PIC0_IMR, 0xFF & ~(1 << 2));
    outb(PIC1_IMR, 0xFF);
}

/**
 * @brief 发送eoi命令
 * @param irq_num 中断号
 */
void pic_send_eoi(int irq_num) {
    irq_num -= IRQ_PIC_START;

    // 从片也可能需要发送EOI
    if (irq_num >= 8) {
        outb(PIC1_OCW2, PIC_OCW2_EOI);
    }

    outb(PIC0_OCW2, PIC_OCW2_EOI);
}

/**
 * @brief 中断允许，操作8259屏蔽字
 */
void irq_enable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START;
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) & ~(1 << irq_num);
        outb(PIC0_IMR, mask);
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) & ~(1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}
/**
 * @brief 中断关闭 8259屏蔽字
 */
void irq_disable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START;
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) | (1 << irq_num);
        outb(PIC0_IMR, mask);
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) | (1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}