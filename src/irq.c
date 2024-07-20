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

// void irq_handle_interrupt(struct Registers regs, u8 irq_number, u8 error_code) {
void irq_handle_interrupt(u64 irq_number, u64 error_code) {
	irq_number -= 32;
	// void (*routine)(struct Registers *) = irq_routines[irq_number];
	// if (routine)
	// 	routine(&regs);
	pic_send_eoi(irq_number);
}

void irq_init() {
	// disable all irqs except keypress
	// outb(0x21, 0xFD);
	// outb(0xA1, 0xFF);

	irq_set_routine(1, keyboard_routine);

	serial_info("Keyboard IRQ set");
}
