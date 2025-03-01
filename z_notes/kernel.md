# 调用约定
- 有多种
- 经典调用约定 System V ABI
- 我的方案
    所有参数push入栈，从左向右
    调用方负责push参数，实现方负责保存各种寄存器的值，实现完成后恢复，再ret
    任务切换两种方法 
    - Tss选择子，效率较低
    - 利用调用约定，效率很高
        ； 调用方之前保存了eax ecx edx
        task_switch:
            push ebp ；保存栈顶指针，一般用于获取参数的
            mov ebp, esp
            ；被调用方，保存ebp ebx esi edi
            push ebx  
            push esi
            push edi

            mov eax, esp ；将此时的栈顶指针保存到eax中

            ；栈向低地址增长，找了个4KB对齐的地方把栈顶指针存到了内存当中
            and eax, 0xfffff000; current  
            mov [eax], esp

            ；通过ebp+8获取task_t的首地址
            ；首地址的第一个字段就是uint32_t stack,他记录了存储栈顶指针的地址
            ；读取到eax中，然后去内存里把next task的栈顶指针从内存中取出，并赋值给esp
            mov eax, [ebp + 8]; next
            mov esp, [eax]

            ；此时切换了栈顶指针，因此，再pop，pop出的是next任务的 edi esi等等
            pop edi
            pop esi
            pop ebx
            pop ebp

            ret




# 分页
    - 32位系统就3级页表
    - 每个进程都有独立页表，共享内核页表，都是4G虚拟内存
    - 页大小可以配置，4KB或者4MB (cr4寄存器的PSE位可以配置)
    - 我的方案
        - loader中启用临时页表，4MB的页，覆盖全部内存128MB，平摊模型
        - 进入kernel后，再次配置页表，改为4KB的页。
        - 每个进程可以维护4GB的虚拟空间，共4GB/4KB = 1M个页
