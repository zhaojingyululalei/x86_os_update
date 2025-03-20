
    .section .text.start
	#include "cpu_cfg.h"
    .extern kernel_init
    .global _start

_start:
    ljmpl $SELECTOR_KERNEL_CODE_SEG,$entry_kernel
entry_kernel:
    mov $SELECTOR_KERNEL_DATA_SEG,%ax
    mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs


    push 4(%esp) # 将boot_info入栈

    call kernel_init
    jmp .



# 中断处理函数，因为返回不能用ret，只能用iret，所以写在汇编里

    .text
.macro exception_handler name num with_error_code
	    .extern do_handler_\name
		.global exception_handler_\name
	exception_handler_\name:
		# 如果没有错误码，压入一个缺省值
		# 这样堆栈就和有错误码的情形一样了
		.if \with_error_code == 0
			push $0
		.endif

		// 压入异常号
		push $\num

		// 保存所有寄存器
		pushal
		push %ds
		push %es
		push %fs
		push %gs

		// 调用中断处理函数
		push %esp
		call do_handler_\name
		add $(1*4), %esp		// 丢掉esp

		// 恢复保存的寄存器
		pop %gs
		pop %fs
		pop %es
		pop %ds
		popal

		// 跳过压入的异常号和错误码
		add $(2*4), %esp
		iret
.endm

# 注册中断处理函数
exception_handler unknown, -1, 0
exception_handler divider, 0, 0
exception_handler Debug, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler bound_range, 5, 0
exception_handler invalid_opcode, 6, 0
exception_handler device_unavailable, 7, 0
exception_handler double_fault, 8, 1
exception_handler invalid_tss, 10, 1
exception_handler segment_not_present, 11, 1
exception_handler stack_segment_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler alignment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler smd_exception, 19, 0
exception_handler virtual_exception, 20, 0

exception_handler timer, 0x20, 0
# exception_handler kbd, 0x21, 0
exception_handler rtc, 0x28, 0
exception_handler signal, 0x40, 0
# exception_handler mouse, 0x2C, 0
# # 中断号不确定
# exception_handler rtl8139, 0x0, 0
