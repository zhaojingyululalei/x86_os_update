

#include "cpu.h"
#include "cpu_instr.h"
#include "printk.h"
#include "cpu_cfg.h"
#include "irq/irq.h"
/**
 * gdt表首地址
 */
static  seg_desc_t * gdt_table;
/**
 * gdt有多少个表项
 */
static uint16_t gdt_entry_nr;
static gate_desc_t idt_table[IDT_ENTRYS_NUM];
 
static void gdt_init(void){
    uint32_t gdt_base = 0;
    uint16_t gdt_limit = 0;
    sgdt(&gdt_base,&gdt_limit);
    gdt_table = (seg_desc_t *)gdt_base;
    gdt_entry_nr = (gdt_limit+1)>>3;
    ASSERT(gdt_entry_nr == GDT_ENTRYS_NUM);
}
static void idt_init(void)
{
    //安装全部异常处理程序
    trap_init();
    lidt((uint32_t)idt_table,IDTR_LIMIT);
}

/**
 * @brief 返回对应irq_num的中断描述符地址
 */
gate_desc_t* idt_get_entry(int irq_num)
{
    return &idt_table[irq_num];
}

/**
 * @brief 主要初始化了gdt和idt表
 */
void cpu_init(void){
    gdt_init();
    idt_init();
    init_pic();
}