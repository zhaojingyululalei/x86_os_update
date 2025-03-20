
    .code32
#include "cpu_cfg.h"
    .text
    .extern syscall_resolve
    .global syscall_init,sysenter_handler,sys_handler_exit

syscall_init:
    movl $0x174,%ecx # IA32_SYSENTER_CS
    movl $SELECTOR_KERNEL_CODE_SEG,%eax # 内核代码段
    xor %edx,%edx
    wrmsr # 写入MSR寄存器

    movl $0x176 ,%ecx          # IA32_SYSENTER_EIP
    movl $sysenter_handler, %eax  # 设定 sysenter 处理函数的地址
    xor %edx, %edx
    wrmsr
    ret


    # 用户态调用sysenter直接跳到这里来，并且esp直接指向0x2000
sysenter_handler:
    
    # eax --- &args   ecx  eip   edx esp
    push %ebp
    push %esi # 保存用户栈段
    push %ecx
    push %edx 
    push %esp
    push %eax 
    # 全部替换为内核段
    mov %ss,%edi
    mov %edi,%ds
    mov %edi,%es
    mov %edi,%fs
    mov %edi,%gs

    call syscall_resolve

    add $(2*4),%esp
sys_handler_exit:
    # cli
    pop %edx
    pop %ecx
    pop %esi # 用户栈段
    pop %ebp
    # 全部替换为用户段
    mov %esi,%edi
    mov %edi,%ds
    mov %edi,%es
    mov %edi,%fs
    mov %edi,%gs
    # sti
    sysexit
