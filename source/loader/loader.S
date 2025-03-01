  	# 16位代码，务必加上
  	.code16
#include "cpu_cfg.h"
 	.text
    .extern detect_memory
	.global _start
    .equ ARD_MAX_NR ,10  
    .equ ARD_SIZE   ,20 

    ljmp $BOOTLOADER_SEG,$_start # 设置CS寄存器

_start:

# 重置数据段寄存器
	mov $BOOTLOADER_SEG, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	mov $STACK_KERNEL_TOP_ADDR,%sp
    mov     $loader_msg,%si
    call    print

# 内存检测
    mov     $detect_mm_msg,%si
    call    print

    call detect_memory

# 关中断
    cli
# 加载gdt表
    lgdt gdt_ptr
# 开A20
    inb $0x92, %al
    or $0x02, %al
    outb %al, $0x92
# 设置保护模式
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

# 保存gdt表首地址，ARDS首地址 以后进了内核要用
    movl $gdt_table,%ebx 
    movl $ARDS_BUFFER,%edx
    ljmpl $SELECTOR_KERNEL_CODE_SEG,$protected_mode


    jmp .
detect_memory:
    xor %ebx, %ebx            # EBX = 0，表示从第一个内存区域开始检测
    mov $ARDS_BUFFER,%di     # ES:DI 指向 ARDS 缓冲区
    mov $0x534D4150,%edx     # EDX = 'SMAP'
    mov $ARD_SIZE,%cx             # ECX = ARDS 的大小（20字节）

detect_memory_loop:
    movl $0xE820,%eax         # EAX = 0xE820，表示调用 E820 功能
    int $0x15                # 调用 BIOS 中断
    jc detect_memory_err   # 如果 CF 被置位，表示调用失败
    cmp $0x534D4150,%eax     # 检查 EAX 是否等于 'SMAP'
    jne detect_memory_err  # 如果不等于，表示 BIOS 不支持 E820 功能
    add $ARD_SIZE,%di              # 更新 DI，指向下一个 ARDS 的存储位置
    test %ebx, %ebx           # 检查 EBX 是否为 0
    jnz detect_memory_loop  # 如果 EBX 不为 0，继续循环

detect_memory_done:
    xor %esi,%esi
    mov     $detect_mm_ok,%si
    call    print
    ret
detect_memory_err:
    xor %esi,%esi
    mov     $detect_memory_err,%si
    call    print
    ret

print:
    mov $0x0e, %ah       # BIOS: 0x10 中断，0x0E 功能号（TTY 输出）
    mov $0x01, %bl       # 字符颜色（默认亮白）

print_loop:
    lodsb                # 加载 (%SI) 到 AL，SI 自动增加
    cmp $0, %al          # 判断是否到字符串末尾
    jz print_done        # 如果 AL == 0，则结束打印

    cmp $'\n', %al       # 检测换行符 '\n'
    je print_newline     # 如果是换行符，跳转处理

    int $0x10            # 通过 BIOS 中断打印字符
    jmp print_loop       # 打印下一个字符

print_newline:
    mov $'\r', %al       # 回车 '\r'
    int $0x10            # 打印回车符
    mov $'\n', %al       # 换行 '\n'
    int $0x10            # 打印换行符
    jmp print_loop       # 继续打印下一个字符

print_done:
    ret

    .data
loader_msg: .asciz "enter loader successful!\n"
detect_mm_msg: .asciz "detecting mm...\n"
detect_mm_err: .asciz "detect memory fail!\n"
detect_mm_ok: .asciz "detect memory success!\n"

gdt_ptr:
    .word GDTR_LIMIT
    .long gdt_table
gdt_table: 
    .long 0,0
kernel_code_desc:
    .long ((DESC_KERNEL_CODE_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_KERNEL_CODE_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_KERNEL_CODE_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b1000 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b00 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_KERNEL_CODE_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_KERNEL_CODE_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

kernel_data_desc:
    .long ((DESC_KERNEL_DATA_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_KERNEL_DATA_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_KERNEL_DATA_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b0010 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b00 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_KERNEL_DATA_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_KERNEL_DATA_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

usr_code_desc:
    .long ((DESC_USR_CODE_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_USR_CODE_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_USR_CODE_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b1000 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b11 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_USR_CODE_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_USR_CODE_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

usr_data_desc:
    .long ((DESC_USR_DATA_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_USR_DATA_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_USR_DATA_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b0010 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b11 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_USR_DATA_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_USR_DATA_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS


    .bss
ARDS_BUFFER: .space ARD_SIZE*ARD_MAX_NR


    .code32
    .text
    .extern load_kernel
protected_mode:
    mov $SELECTOR_KERNEL_DATA_SEG,%ax
    mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    movl $STACK_KERNEL_TOP_ADDR,%esp

    push %edx  # ADRS_BUFF
    push %ebx  # gdt_table
    call load_kernel
    jmp .