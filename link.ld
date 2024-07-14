ENTRY(_start)
SECTIONS {
	.multiboot : {
		*(.multiboot)
	}

	KERNEL_VMA = 0x0;
	. = 0x00100000;

	.boot_text BLOCK(4K) : ALIGN(4K) {
		*(.boot_text)
	}
	.boot_rodata BLOCK(4K) : ALIGN(4K) {
		*(.boot_rodata)
	}
	.boot_data BLOCK(4K) : ALIGN(4K) {
		*(.boot_data)
	}
	.boot_bss BLOCK(4K) : ALIGN(4K) {
		*(.boot_bss)
	}

	. += KERNEL_VMA;

	.text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA) {
		*(.text)
	}

	.rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA) {
		*(.rodata)
	}

	.data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {
		*(.data)
	}

	.bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {
		*(COMMON)
		*(.bss)
	}

	kernel_end = .;
}
