#include "irq.h"
#include "output.h"
#include "idt.h"
#include "io.h"
#include "pic.h"
#include "keyboard.h"
#include "const.h"

void (*irq_routines[16])(struct Registers *) = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void irq_set_routine(u8 irq_number, void (*routine)(struct Registers *)) {
	irq_routines[irq_number] = routine;
}

// for some reason, triggering any irq creates a gpf (isr 13)??
void irq_handle_interrupt(struct Registers regs, u8 irq_number, u8 error_code) {
	irq_number -= 32;
	void (*routine)(struct Registers *) = irq_routines[irq_number];
	if (routine)
		routine(&regs);
	pic_send_eoi(irq_number);
}
void irq_init() {
	pic_remap(0x20, 0x28);

	serial_info("PIC remapped");

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

	serial_info("IRQs loaded");

	// disable all irqs except keypress
	// outb(0x21, 0xFD);
	// outb(0xA1, 0xFF);

	irq_set_routine(1, keyboard_routine);

	serial_info("Keyboard IRQ set");
}
