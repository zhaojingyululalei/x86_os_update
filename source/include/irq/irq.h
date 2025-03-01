
#ifndef __IRQ_H
#define __IRQ_H
#include "types.h"
typedef uint32_t irq_state_t;


/**
 * @brief 进入中断保护
 */
irq_state_t irq_enter_protection (void);
/**
 * @brief 退出中断保护
 */
void irq_leave_protection (irq_state_t state);
/**
 * @brief 全局关中断
 */
void irq_disable_global(void);
/**
 * @brief 全局开中断
 */
void irq_enable_global(void);
#endif