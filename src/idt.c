#include <stdbool.h>
#include "idt.h"
#include "io.h"
#include "output.h"
#include "pic.h"
#include "isr.h"
#include "irq.h"
#include "const.h"

struct IDTEntry idt[256];
struct IDTPointer idt_ptr;

void idt_set_entry(u8 index, u64 isr, u8 flags) { 
    idt[index].isr_low = isr & 0xFFFF;
	idt[index].isr_mid = (isr >> 16) & 0xFFFF;
    idt[index].isr_high = (isr >> 32) & 0xFFFFFFFF;
    idt[index].segment = 0x08;
    idt[index].attributes = flags;
    idt[index].ist = 0;
}
 
void idt_init() {
	for (u16 i = 0; i < 256; i++)
		idt_set_entry(i, 0, 0);

	idt_set_entry(0,  (u64) isr0,  0x8E);
	idt_set_entry(1,  (u64) isr1,  0x8E);
	idt_set_entry(2,  (u64) isr2,  0x8E);
	idt_set_entry(3,  (u64) isr3,  0x8E);
	idt_set_entry(4,  (u64) isr4,  0x8E);
	idt_set_entry(5,  (u64) isr5,  0x8E);
	idt_set_entry(6,  (u64) isr6,  0x8E);
	idt_set_entry(7,  (u64) isr7,  0x8E);
	idt_set_entry(8,  (u64) isr8,  0x8E);
	idt_set_entry(9,  (u64) isr9,  0x8E);
	idt_set_entry(10, (u64) isr10, 0x8E);
	idt_set_entry(11, (u64) isr11, 0x8E);
	idt_set_entry(12, (u64) isr12, 0x8E);
	idt_set_entry(13, (u64) isr13, 0x8E);
	idt_set_entry(14, (u64) isr14, 0x8E);
	idt_set_entry(15, (u64) isr15, 0x8E);
	idt_set_entry(16, (u64) isr16, 0x8E);
	idt_set_entry(17, (u64) isr17, 0x8E);
	idt_set_entry(18, (u64) isr18, 0x8E);
	idt_set_entry(19, (u64) isr19, 0x8E);
	idt_set_entry(20, (u64) isr20, 0x8E);
	idt_set_entry(21, (u64) isr21, 0x8E);
	idt_set_entry(22, (u64) isr22, 0x8E);
	idt_set_entry(23, (u64) isr23, 0x8E);
	idt_set_entry(24, (u64) isr24, 0x8E);
	idt_set_entry(25, (u64) isr25, 0x8E);
	idt_set_entry(26, (u64) isr26, 0x8E);
	idt_set_entry(27, (u64) isr27, 0x8E);
	idt_set_entry(28, (u64) isr28, 0x8E);
	idt_set_entry(29, (u64) isr29, 0x8E);
	idt_set_entry(30, (u64) isr30, 0x8E);
	idt_set_entry(31, (u64) isr31, 0x8E);

	idt_set_entry(32, (u64) irq0,  0x8E);
	idt_set_entry(33, (u64) irq1,  0x8E);
	idt_set_entry(34, (u64) irq2,  0x8E);
	idt_set_entry(35, (u64) irq3,  0x8E);
	idt_set_entry(36, (u64) irq4,  0x8E);
	idt_set_entry(37, (u64) irq5,  0x8E);
	idt_set_entry(38, (u64) irq6,  0x8E);
	idt_set_entry(39, (u64) irq7,  0x8E);
	idt_set_entry(40, (u64) irq8,  0x8E);
	idt_set_entry(41, (u64) irq9,  0x8E);
	idt_set_entry(42, (u64) irq10, 0x8E);
	idt_set_entry(43, (u64) irq11, 0x8E);
	idt_set_entry(44, (u64) irq12, 0x8E);
	idt_set_entry(45, (u64) irq13, 0x8E);
	idt_set_entry(46, (u64) irq14, 0x8E);
	idt_set_entry(47, (u64) irq15, 0x8E);

	pic_remap(0x20, 0x28);
	irq_set_mask(0);
	// outb(0x21, 0xFD);
	// outb(0xA1, 0xFF);

	idt_ptr.size = sizeof(idt) - 1;
	idt_ptr.offset = (u64) &idt;

	asm volatile("lidt %0" :: "m"(idt_ptr));
}
