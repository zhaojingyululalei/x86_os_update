
  	# 16位代码，务必加上
  	.code16
#include "mem_addr_cfg.h"
 	.section .text
	.global _start,print
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



	

# 设置屏幕模式为文本模式，清除屏幕
    mov $3,%ax
    int $0x10
# 显示一个字符茶串
	mov     $hello_msg,%si
    call    print
	jmp read_loader

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


read_loader:
	mov $LOAD_START_ADDR_REL, %bx	# 读取到的内存地址
	mov $0x2, %cx		# ch:磁道号，cl起始扇区号 #这是从1开始算的，没有0号扇区
	mov $0x2, %ah		# ah: 0x2读磁盘命令
	mov $64, %al		# al: 读取的扇区数量, 必须小于128，暂设置成32KB
	mov $0x0080, %dx	# dh: 磁头号，dl驱动器号0x80(磁盘1)
	int $0x13
	cmp $0,%ah
	jz jmp_to_loader

	//显示一个字符茶串
	mov     $read_disk_fault_msg,%si
    call    print

	// 原地跳转
	jmp .

	
jmp_to_loader:
	mov $LOAD_START_ADDR_REL,%bx
	jmp *%bx



hello_msg: .asciz "hello zhao os ...\n"
read_disk_fault_msg: .asciz "read disk fault ...\n"
	.org 510   # 再当前文件中的偏移
	.word 0xaa55 #

	




	
