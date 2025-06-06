
 	.text
 	.global sys_call,sysenter_exit
	.extern 
# ...   
# &arg
# eip  ebx指向这里 --- 栈返回到这里 ---ip返回到 sys_call的下地址

# 调用前 ip：0x80000050  esp:0x0xeffeff7f，指向eax  eax存着&arg
# sys_call:
#     push %ebx 

#     mov %ss,%esi  # 保存用户栈段到esi
#     mov %esp,%ebx  # 用ebx存放此刻的esp  
#     addl $4,%ebx
#     # mov 4(%ebx),%eax   &arg已经在eax中
#     mov (%ebx),%edx # 把eip放在edx中
#     mov %ebx,%ecx
#     addl $8,%ecx  #  把esp的值放入ecx  ，此时esp指向的是eip

    
#     pop %ebx
#     sysenter # 直接跳转到内核栈，以后再也跳不回来了

# sysenter动作
# 从msr寄存器取 eip(sysenter_handle) esp(0x2000) 跳转到内核的sysenter_handle函数  
# 注：用户态eip esp并不会保存。根据约定，把用户态的esp保存在ecx，eip保存在edx
sys_call:
    push %ebx 
    push %esi 
    push %edi 

    mov %ss,%esi  # 保存用户栈段到esi
    mov $sysenter_exit,%edx # 把eip放在edx中
    mov %esp,%ecx
    sysenter
sysenter_exit:
    pop %edi 
    pop %esi 
    pop %ebx 
    ret 