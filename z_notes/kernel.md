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

# 中断
    - IDT表  lidt指令加载IDT表  中断门注册中断函数
    - int指令
        int num；压入eflags，cs，ip  ，
        3特权级想用int，得设置好CPL和DPL，或者异步中断，直接还会先压入ss3和esp3
        实模式下，中断表中低2字节存offset，高2字节存seg
        保护模式下，32位全部存offset
    - iret 中断返回指令，弹出 ip cs eflags
# 内存管理
    alloc_pages
    kamlloc
    vmalloc
    水位线
    alloc_context
    快速分配 get_page_from_free_list
    慢速分配 __alloc_pages_slowpath
    pcp 每内存缓冲区
    内存分配策略：NUMA架构


    pglist----NUMA节点有几个，就有几个pglist
    zone_dma
    zone_dma
    zone_NORMAL   中有free_area,是一个数组，每个数组元素是一个page链表
    zone_HIGHMFM

    page 页帧
    page_to_pfn
    pfn_to_page

    zone与page如何关联的
    automic_long_t
    c中的原子操作


    NUMA节点 pglist zone page 物理页
    备用节点

    分配order=的物理页
    mempolicy 内存分配策略
    直接内存回收，异步内存回收

    slab slob slub

    vma
    
