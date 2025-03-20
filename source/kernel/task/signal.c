#include "task/signal.h"
#include "irq/irq.h"
#include "string.h"
#include "task/task.h"
extern void exception_handler_signal(void);
void do_handler_signal(exception_frame_t *frame){
    task_t* cur = cur_task();
    memcpy(frame,&cur->frame,sizeof(exception_frame_t));

}

void signal_init(void){
    //注册0x40中断

    
    uint32_t handler_addr = (irq_handler_t)exception_handler_signal;
    gate_desc_t *desc = idt_get_entry(IRQ_SIGNAL);
    desc->offset_15_0 = handler_addr & 0xFFFF;
    desc->offset_31_16 = (handler_addr >> 16) & 0xFFFF;
    desc->DPL = GATE_DPL3;
    desc->p = 1;
    desc->param = 0;
    desc->selector = SELECTOR_KERNEL_CODE_SEG;
    desc->type = GATE_INT_TYPE_IDT;
    return;
    
}

void sigaciton(void){
    
}
