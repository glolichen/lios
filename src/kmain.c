#include "io.h"
#include "fb.h"
#include "const.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "pic.h"
#include "serial.h"
#include "printf.h"

void kmain() {
	gdt_init();
	idt_init();
	irq_init();

    __asm__ volatile ("sti");

	fb_init();
	fb_clear();
}
