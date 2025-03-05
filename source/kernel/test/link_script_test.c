#include "types.h"
#include "printk.h"
extern uint8_t s_ro; //只读区起始地址
extern uint8_t s_rw; //读写区起始地址
extern uint8_t e_rw; //读写区结束地址

void link_script_test(void){
    ph_addr_t ph_s_rw = (&s_rw);
    dbg_info("s_ro:0x%x,s_rw:0x%x,e_rw:0x%x\r\n",&s_ro,&s_rw,&e_rw);
}