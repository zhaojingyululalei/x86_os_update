
#ifndef __IRQ_H
#define __IRQ_H
#include "types.h"
#include "8259A.h"
#include "traps.h"
/*中断号码*/ 
//默认异常
#define IRQ0_DE             0
#define IRQ1_DB             1
#define IRQ2_NMI            2
#define IRQ3_BP             3
#define IRQ4_OF             4
#define IRQ5_BR             5
#define IRQ6_UD             6
#define IRQ7_NM             7
#define IRQ8_DF             8
#define IRQ10_TS            10
#define IRQ11_NP            11
#define IRQ12_SS            12
#define IRQ13_GP            13
#define IRQ14_PF            14
#define IRQ16_MF            16
#define IRQ17_AC            17
#define IRQ18_MC            18
#define IRQ19_XM            19
#define IRQ20_VE            20

//外部中断,来自8259
#define IRQ_CLOCK 0      // 时钟
#define IRQ_KEYBOARD 1   // 键盘
#define IRQ_CASCADE 2    // 8259 从片控制器
#define IRQ_SERIAL_2 3   // 串口 2
#define IRQ_SERIAL_1 4   // 串口 1
#define IRQ_PARALLEL_2 5 // 并口 2
#define IRQ_SB16 5       // SB16 声卡
#define IRQ_FLOPPY 6     // 软盘控制器
#define IRQ_PARALLEL_1 7 // 并口 1
#define IRQ_RTC 8        // 实时时钟
#define IRQ_REDIRECT 9   // 重定向 IRQ2
#define IRQ_NIC 11       // 网卡
#define IRQ_MOUSE 12     // 鼠标
#define IRQ_MATH 13      // 协处理器 x87
#define IRQ_HARDDISK 14  // ATA 硬盘第一通道
#define IRQ_HARDDISK2 15 // ATA 硬盘第二通道

#define IRQ_MASTER_NR 0x20 // 主片起始向量号
#define IRQ_SLAVE_NR 0x28  // 从片起始向量号

#define IRQ0_BASE           0x20
#define IRQ0_TIMER          (IRQ0_BASE+IRQ_CLOCK)               //clock中断
#define IRQ1_KEYBOARD		(IRQ0_BASE+IRQ_KEYBOARD)				// 按键中断
#define IRQ8_RTC            (IRQ0_BASE+IRQ_RTC)               //rtc中断
#define IRQ12_MOUSE         (IRQ0_BASE+IRQ_MOUSE)                // 鼠标中断号
#define IRQ14_HARDDISK_PRIMARY		(IRQ0_BASE+IRQ_HARDDISK)		// 主总线上的ATA磁盘中断
#define IRQ15_HARDDISK_SLAVE        (IRQ0_BASE+IRQ_HARDDISK2)       //ATA第二通道


//软中断
#define IRQ_SIGNAL          0x40               //用于信号机制
typedef uint32_t irq_state_t;



irq_state_t irq_enter_protection (void);

void irq_leave_protection (irq_state_t state);

void irq_disable_global(void);

void irq_enable_global(void);
#endif