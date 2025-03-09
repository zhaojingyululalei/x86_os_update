ENTRY(_start);

SECTIONS
{
    /* 内核区 */
    . = 0x100000;
    PROVIDE(s_ro = .);

    /* 只读段 */
    .text : {
        *(.text.start) 
        *(EXCLUDE_FILE(*init_task* *applib*).text)
    }
    .rodata : {
        *(EXCLUDE_FILE(*init_task* *applib*).rodata)
    }

    /* 可写段 */
    . = ALIGN(4096);
    PROVIDE(s_rw = .);
    .data : {
        *(EXCLUDE_FILE(*init_task* *applib*).data)
    }
    .bss : {
        *(EXCLUDE_FILE(*init_task* *applib*).bss)
    }
    . = ALIGN(4096);
    PROVIDE(e_rw = .);
    e_kernel = .;

    /* 初始进程区 */
    . = 0x80000000;
    PROVIDE(s_init_task_ph = LOADADDR(.init_task));
    PROVIDE(s_init_task_vm = ADDR(.init_task));

    .init_task : AT(e_kernel) {
        *init_task*(.text .data .rodata .bss)
        *applib*(.text .data .rodata .bss)  /* 将 libstd.a 的代码和数据放入初始进程区 */
    }

    PROVIDE(e_init_task_ph = LOADADDR(.init_task) + SIZEOF(.init_task));
    PROVIDE(e_init_task_vm = ADDR(.init_task) + SIZEOF(.init_task));
}