ENTRY(_start)

KERNEL_OFFSET = 0xFFFFFFFF80000000;

SECTIONS {
	.mboot ALIGN(4K) : AT(ADDR(.mboot)) {
		*(.mboot)
	}

	. = 1M;

	.bootstrap ALIGN(4K) : AT(ADDR(.bootstrap)) {
		src/loader.o(.text)
	}

	. += KERNEL_OFFSET;

	.text ALIGN(4K) : AT(ADDR(.text) - KERNEL_OFFSET) {
		*(EXCLUDE_FILE(*src/loader.o) .text)
		*(.kernel_text)
	}

	.rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_OFFSET) {
		*(.rodata)
	}

	.data ALIGN(4K) : AT(ADDR(.data) - KERNEL_OFFSET) {
		*(.data)
	}

	.bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_OFFSET) {
		*(COMMON)
		*(.bss)
	}

	kernel_end = .;
}
