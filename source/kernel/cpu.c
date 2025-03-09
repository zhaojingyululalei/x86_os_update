

#include "cpu.h"
#include "cpu_instr.h"
#include "printk.h"
#include "cpu_cfg.h"
#include "irq/irq.h"
#include "apci.h"
#include "string.h"
#include "task/task.h"
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
 * @brief 初始化tss结构 ，安装tss描述符，主要用于特权级从3到0的切换
 */
void tss_init(void){
    tss_t* tss = &task_manager.tss;
    memset(tss,0,sizeof(tss_t));
    tss->ss0 = SELECTOR_KERNEL_DATA_SEG;
    // 每个进程的esp0(内核栈不同，例如要切换到进程A，然后把A的内核栈空间赋值给tss.esp0)
    int gdt_idx = SELECTOR_TSS_SEG>>3;
    seg_desc_t* tss_desc = gdt_table+gdt_idx;
    memset(tss_desc,0,sizeof(seg_desc_t));
    tss_desc->p = 1;
    tss_desc->DPL = 0;
    tss_desc->type = SEG_TYPE_TSS;
    tss_desc->base_15_0 = (uint32_t)(tss) & 0xFFFF;
    tss_desc->base_23_16 = ((uint32_t)(tss) >> 16) & 0xFF;
    tss_desc->base_31_24 = ((uint32_t)(tss) >> 24) & 0xFF;

    tss_desc->limit_15_0 = sizeof(tss_t) & 0xFFFF;
    tss_desc->limit_19_6 = (sizeof(tss_t) >> 16)&0xF;
    write_tr(SELECTOR_TSS_SEG);
}
extern void syscall_init(void);
/**
 * @brief 主要初始化了gdt和idt表
 */
void cpu_init(void){
    gdt_init(); //安装gdt表
    idt_init(); //安装异常
    tss_init();//安装tss段
    init_pic(); //初始化8259
    apci_init();//获取apci信息
    syscall_init(); //初始化系统调用
    return;
}