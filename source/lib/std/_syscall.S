
 	.text
 	.global sys_call,sysenter_exit
	.extern 
# 根据约定，把用户态的esp保存在ecx，eip保存在edx
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