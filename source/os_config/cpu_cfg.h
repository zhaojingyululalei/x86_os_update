#ifndef __CPU_CFG_H
#define __CPU_CFG_H

#include "mem_addr_cfg.h"
#include "blk_cfg.h"

//GDT描述符格式
#define GDT_DESC_LIMIT_0_15_POS    0
#define GDT_DESC_BASE_0_15_POS     16
#define GDT_DESC_BASE_16_23_POS    0
#define GDT_DESC_TYPE_POS          8
#define GDT_DESC_S_POS             12
#define GDT_DESC_DPL_POS           13
#define GDT_DESC_P_POS             15
#define GDT_DESC_LIMIT_16_19_POS   16
#define GDT_DESC_AVL_POS           20
#define GDT_DESC_DB_POS            22
#define GDT_DESC_G_POS             23
#define GDT_DESC_BASE_24_31_POS    24

//描述符设置
#define DESC_GDT_DESC_MAX_SEG_LIMIT          0xFFFFF

#define DESC_BOOTLOADER_BASE_ADDR        (BOOTLOADER_SEG << 4)
#define DESC_BOOTLOADER_SEG_LIMIT        DESC_GDT_DESC_MAX_SEG_LIMIT

#define DESC_KERNEL_CODE_BASE_ADDR        0x0
#define DESC_KERNEL_CODE_SEG_LIMIT       DESC_GDT_DESC_MAX_SEG_LIMIT

#define DESC_KERNEL_DATA_BASE_ADDR       0x0
#define DESC_KERNEL_DATA_SEG_LIMIT       DESC_GDT_DESC_MAX_SEG_LIMIT  

#define DESC_USR_CODE_BASE_ADDR         0x0
#define DESC_USR_CODE_SEG_LIMIT         DESC_GDT_DESC_MAX_SEG_LIMIT

#define DESC_USR_DATA_BASE_ADDR       0x0
#define DESC_USR_DATA_SEG_LIMIT       DESC_GDT_DESC_MAX_SEG_LIMIT  


//GDTR设置
#define GDT_ENTRYS_NUM  256
#define GDTR_LIMIT  (GDT_ENTRYS_NUM << 3 - 1)
//IDTR设置
#define IDT_ENTRYS_NUM  256
#define IDTR_LIMIT (IDT_ENTRYS_NUM << 3 - 1)
//段选择子设置

#define RPL_0   0
#define RPL_3   3


#define SELECTOR_KERNEL_CODE_SEG        (1<<3|RPL_0)
#define SELECTOR_KERNEL_DATA_SEG        (2<<3|RPL_0)
#define SELECTOR_USR_CODE_SEG           (3<<3|RPL_3)   
#define SELECTOR_USR_DATA_SEG           (4<<3|RPL_3)
#define SELECTOR_CALL_GATE              (5<<3|RPL_3)





#endif
