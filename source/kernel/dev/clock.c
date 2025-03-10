#include "time/clock.h"
#include "irq/irq.h"
#include "printk.h"
#include "cpu_instr.h"
#include "task/sche.h"
#include "cpu_cfg.h"
static uint32_t sys_tick;
/**
 * @brief 初始化计数器0
 */
static void pit_init(void)
{
    //计数器计数到reload_count就发生一次中断
    //每个时钟reload_count++
    //实际就是计算10ms有多少时钟周期，1秒有1193182个时钟，算10ms有几个
    uint32_t reload_count = PIT_OSC_FREQ * (OS_TICK_MS / 1000.0);

    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNLE0 | PIT_LOAD_LOHI | PIT_MODE3);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);        // 加载低8位
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF); // 再加载高8位
}
/**
 * @brief 时钟中断服务程序
 */
void do_handler_timer (exception_frame_t *frame) {
    sys_tick+=10;
    // 发送EOI
    pic_send_eoi(IRQ0_TIMER);
    clock_sleep_check(); //睡醒的任务加入就绪队列
    clock_gwait_check(); //等待超时的任务加入就绪队列
    task_t* cur = cur_task();
    if(!cur){
        dbg_error("cur task is NULL\r\n");
    }
    if(!is_stack_magic(cur)){
        dbg_error("stack magic err,maybe overflow\r\n");
        return;
    }
    if(! --cur->ticks){
        cur->ticks = cur->priority;
        schedule();
    }
   
}
/**
 * @brief 时钟初始化
 */
void clock_init(void)
{
    sys_tick = 0;
    pit_init();
    interupt_install(IRQ0_TIMER, (irq_handler_t)exception_handler_timer);
    irq_enable(IRQ0_TIMER);
}