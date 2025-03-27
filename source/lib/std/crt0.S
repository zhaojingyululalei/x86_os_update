


    .section .text.start
    .global _start
    .extern cstart
_start:
    
    mov %ss, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # 必须用call，压入一个ip值
    call cstart