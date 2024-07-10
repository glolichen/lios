ENTRY(_start)
SECTIONS {
	. = 0x00100000;

	.multiboot ALIGN(0x1000): {
		*(.multiboot)
	}

	. += 0xC0000000;

	.text ALIGN(0x1000): AT(ADDR(.text) - 0xC0000000){
		*(.text)
	}

	.data ALIGN(0x1000): AT(ADDR(.data) - 0xC0000000){
		*(.data)
	}

	.bss ALIGN(0x1000): AT(ADDR(.bss) - 0xC0000000){
		*(COMMON)
		*(.bss)
	}

	_kernel_end = .;
}
