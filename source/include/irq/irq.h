
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

#define IRQ0_BASE           0x20
#define IRQ0_TIMER          0x20                //clock中断
#define IRQ1_KEYBOARD		0x21				// 按键中断
#define IRQ8_RTC            0x28                //rtc中断
#define IRQ12_MOUSE         0x2C                // 鼠标中断号
#define IRQ14_HARDDISK_PRIMARY		0x2E		// 主总线上的ATA磁盘中断
typedef uint32_t irq_state_t;



irq_state_t irq_enter_protection (void);

void irq_leave_protection (irq_state_t state);

void irq_disable_global(void);

void irq_enable_global(void);
#endif