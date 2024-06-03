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
	serial_info("Interrupts enabled");

	fb_init();
	fb_clear();

	pmm_init(info, magic);
}
