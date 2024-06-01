#include "isr.h"
#include "idt.h"
#include "const.h"
#include "printf.h"

__attribute__((noreturn))
void isr_handle_exception(u8 isr_number, u8 error_code) {
	fb_printf("ISR #%d: %s, code %d\n", isr_number, EXCEPTIONS[isr_number], error_code);
	__asm__("cli; hlt");
	while (1);
}

void isr_init() {
	idt_set_entry(0,  (u32) isr0,  0x8E);
	idt_set_entry(1,  (u32) isr1,  0x8E);
	idt_set_entry(2,  (u32) isr2,  0x8E);
	idt_set_entry(3,  (u32) isr3,  0x8E);
	idt_set_entry(4,  (u32) isr4,  0x8E);
	idt_set_entry(5,  (u32) isr5,  0x8E);
	idt_set_entry(6,  (u32) isr6,  0x8E);
	idt_set_entry(7,  (u32) isr7,  0x8E);
	idt_set_entry(8,  (u32) isr8,  0x8E);
	idt_set_entry(9,  (u32) isr9,  0x8E);
	idt_set_entry(10, (u32) isr10, 0x8E);
	idt_set_entry(11, (u32) isr11, 0x8E);
	idt_set_entry(12, (u32) isr12, 0x8E);
	idt_set_entry(13, (u32) isr13, 0x8E);
	idt_set_entry(14, (u32) isr14, 0x8E);
	idt_set_entry(15, (u32) isr15, 0x8E);
	idt_set_entry(16, (u32) isr16, 0x8E);
	idt_set_entry(17, (u32) isr17, 0x8E);
	idt_set_entry(18, (u32) isr18, 0x8E);
	idt_set_entry(19, (u32) isr19, 0x8E);
	idt_set_entry(20, (u32) isr20, 0x8E);
	idt_set_entry(21, (u32) isr21, 0x8E);
	idt_set_entry(22, (u32) isr22, 0x8E);
	idt_set_entry(23, (u32) isr23, 0x8E);
	idt_set_entry(24, (u32) isr24, 0x8E);
	idt_set_entry(25, (u32) isr25, 0x8E);
	idt_set_entry(26, (u32) isr26, 0x8E);
	idt_set_entry(27, (u32) isr27, 0x8E);
	idt_set_entry(28, (u32) isr28, 0x8E);
	idt_set_entry(29, (u32) isr29, 0x8E);
	idt_set_entry(30, (u32) isr30, 0x8E);
	idt_set_entry(31, (u32) isr31, 0x8E);
}