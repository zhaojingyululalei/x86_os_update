ENTRY(_start);
SECTIONS
{
    /*内核区*/
    . = 0x100000;
    PROVIDE(s_ro = .);
    /*只读*/
    .text : {
		*(.text.start) 
    *(.text)
	}
	.rodata : {
		*(.rodata)
	}

    /*可写*/
    . = ALIGN(4096);
    PROVIDE(s_rw = .);
    .data : {
		*(.data)
	}
	.bss : {
		*(.bss)
	}
    . = ALIGN(4096);
    PROVIDE(e_rw = .);


}