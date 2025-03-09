# taskt* next
# last ip
# last ebp ---src esp
# ebx
# esi
# edi   ---now esp
    .text
    .extern cur_task set_cur_task
    .global task_switch
task_switch:
    # 保存当前任务的上下文
    push %ebp
    mov %esp,%ebp

    push %ebx
    push %esi
    push %edi

    # 获取当前任务的task_t指针
    call cur_task # 返回值是 task_t* cur_task,保存在eax中
    # cur_task 第一个元素就是栈顶指针
    # eax里存的是cur_task指针，该处存放着esp指针
    # # 将当前栈指针保存到cur_task->esp
    movl %esp,(%eax)

    # 获取下一个任务的task_t指针 (参数传入)
    movl 8(%ebp),%eax 

    # 将下一个要执行的进程设置为当前进程
    push %eax
    call set_cur_task
    pop %eax
    
    # 从task_t中加载下一个任务的栈指针到esp
    movl (%eax),%esp
    xor %eax ,%eax  # 清空返回值
    # 恢复下一个任务的上下文
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    ret