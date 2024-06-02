#include "io.h"
#include "fb.h"
#include "const.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "pic.h"
#include "serial.h"
#include "multiboot.h"
#include "printf.h"
#include "pmm.h"

void kmain(multiboot_info_t *info, u32 magic) {
	gdt_init();
	idt_init();
	irq_init();

    __asm__ volatile ("sti");

	fb_init();
	fb_clear();

	u32 memSize = 1024 + info->mem_lower + info->mem_upper * 64;
	fb_printf("%d\n", memSize);
}
