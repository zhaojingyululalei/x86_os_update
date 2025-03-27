ENTRY(_start)
SECTIONS
{
    /*为了让qemu调试方便，因此入口地址定为0x81000000*/
	. = 0x81000000;
	.text : {
		*(.text.start) 
		*(*.text)
	}

	.rodata : {
		*(*.rodata)
	}

	.data : {
		*(*.data)
	}

	.bss : {
		__bss_start__ = .;
		*(*.bss)
    	__bss_end__ = . ;
	}
}