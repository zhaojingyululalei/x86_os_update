
#ifndef __8259A_H
#define __8259A_H

// PIC控制器相关的寄存器及位配置
#define PIC0_ICW1			0x20
#define PIC0_ICW2			0x21
#define PIC0_ICW3			0x21
#define PIC0_ICW4			0x21
#define PIC0_OCW2			0x20
#define PIC0_IMR			0x21

#define PIC1_ICW1			0xa0
#define PIC1_ICW2			0xa1
#define PIC1_ICW3			0xa1
#define PIC1_ICW4			0xa1
#define PIC1_OCW2			0xa0
#define PIC1_IMR			0xa1

#define PIC_ICW1_ICW4		(1 << 0)		// 1 - 需要初始化ICW4
#define PIC_ICW1_ALWAYS_1	(1 << 4)		// 总为1的位
#define PIC_ICW4_8086	    (1 << 0)        // 8086工作模式
#define PIC_ICW4_SNFM       (1 << 4)

#define PIC_OCW2_EOI		(1 << 5)		// 1 - 非特殊结束中断EOI命令

#define IRQ_PIC_START		0x20			// PIC中断起始号

void init_pic(void);

void pic_send_eoi(int irq_num);

void irq_enable(int irq_num);

void irq_disable(int irq_num) ;

#endif
