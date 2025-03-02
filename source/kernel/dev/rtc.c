#include "time/rtc.h"
#include "irq/irq.h"
#include "printk.h"
#include "time/time.h"
#include "cpu_instr.h"
#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

#define CMOS_SECOND 0x01
#define CMOS_MINUTE 0x03
#define CMOS_HOUR 0x05

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80

// 读 cmos 寄存器的值
static inline uint8_t read_cmos(uint8_t reg)
{
    outb(CMOS_ADDR, CMOS_NMI | reg);
    return inb(CMOS_DATA);
};

// 写 cmos 寄存器的值
static inline void write_cmos(uint8_t reg, uint8_t value)
{
    outb(CMOS_ADDR, CMOS_NMI | reg);
    outb(CMOS_DATA, value);
}

void do_handler_rtc(exception_frame_t *frame){
    pic_send_eoi(IRQ8_RTC);
    dbg_info("rtc handler\r\n");
}


/**
 * @brief 设置定时器,几秒后发生中断
 */
void set_alarm(uint32_t secs)
{
    
    tm_t time;
    tm_t cur;
    sys_get_clocktime(&cur);
    time_t stamp = mktime(&cur);
    stamp += secs;
    localtime(&time,stamp);

    write_cmos(CMOS_HOUR, BIN_TO_BCD(time.tm_hour));
    write_cmos(CMOS_MINUTE, BIN_TO_BCD(time.tm_min));
    write_cmos(CMOS_SECOND, BIN_TO_BCD(time.tm_sec));

    write_cmos(CMOS_B, 0b00100010); // 打开闹钟中断，第5位
    read_cmos(CMOS_C);              // 读 C 寄存器，以允许 CMOS 中断
}
void rtc_init(void){
    interupt_install(IRQ8_RTC,exception_handler_rtc);
    irq_enable(IRQ8_RTC);
}