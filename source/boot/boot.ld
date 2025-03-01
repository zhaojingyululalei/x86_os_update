SECTIONS
{
    . = 0x7c00;  /* 设置起始地址为 0x7c00 */
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}