#include "dev/serial.h"
#include "irq/irq.h"
#include "cpu_instr.h"
#define COM1_PORT           0x3F8       // RS232端口0初始化

static void rs232_init(void)
{
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  
    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM1_PORT + 4, 0x0F);
}

void serial_init(void)
{
    rs232_init();
}

/**
 * @brief 日志打印
 */
void serial_print(const char * str_buf) {
    const char * p = str_buf;   
    irq_state_t state =  irq_enter_protection(); 
    while (*p != '\0') {
        while ((inb(COM1_PORT + 5) & (1 << 6)) == 0);
        outb(COM1_PORT, *p++);
    }
    irq_leave_protection(state);
}


