#include "isr.h"
#include "idt.h"
#include "const.h"
#include "output.h"

__attribute__((noreturn))
void isr_handle_exception(u8 isr_number, u8 error_code) {
	fb_printf("ISR #%d: %s, code %d\n", isr_number, EXCEPTIONS[isr_number], error_code);
	asm("cli; hlt");
	while (1);
}

void isr_init() {
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
}
