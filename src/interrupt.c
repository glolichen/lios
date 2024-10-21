#include "interrupt.h"
#include "const.h"
#include "io/keyboard.h"
#include "io/output.h"
#include "io/io.h"
#include "mem/page.h"

struct IDTEntry idt[256];
struct IDTPointer idt_ptr;

void (*irq_routines[16])(struct InterruptData *) = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
void (*exception_handlers[32])(struct InterruptData *) = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
void irq_set_routine(u8 irq_number, void (*routine)(struct InterruptData *)) {
	irq_routines[irq_number] = routine;
}
void exception_set_handler(u8 exception, void (*handler)(struct InterruptData *)) {
	irq_routines[exception] = handler;
}

void idt_set_entry(u8 index, u64 isr, u8 flags) { 
	idt[index].isr_low = isr & 0xFFFF;
	idt[index].isr_mid = (isr >> 16) & 0xFFFF;
	idt[index].isr_high = (isr >> 32) & 0xFFFFFFFF;
	idt[index].segment = 0x08;
	idt[index].attributes = flags;
	idt[index].ist = 0;
}

__attribute__((noreturn))
void handle_exception(struct InterruptData *data) {
	// fb_printf(
	// 	"Exception 0x%x: %s, code %u\n",
	// 	data->interrupt_num,
	// 	EXCEPTIONS[data->interrupt_num],
	// 	data->error_code
	// );
	serial_error(
		"Exception 0x%x: %s, code %u",
		data->interrupt_num,
		EXCEPTIONS[data->interrupt_num],
		data->error_code
	);
	void (*handler)(struct InterruptData *) = irq_routines[data->interrupt_num];
	if (handler)
		handler(data);
	asm("cli; hlt");
	while (1);
}

void pic_acknowledge(u8 irq) {
	if (irq >= 8)
		outb(PICS_COMMAND, PIC_EOI);
	outb(PICM_COMMAND, PIC_EOI);
}
void handle_interrupt(struct InterruptData *data) {
	u32 irq_number = data->interrupt_num - 32;
	void (*routine)(struct InterruptData *) = irq_routines[irq_number];
	if (routine)
		routine(data);
	pic_acknowledge(irq_number);
}

void interrupt_init() {
	for (u16 i = 0; i < 256; i++)
		idt_set_entry(i, 0, 0);

	idt_set_entry(0,  (u64) exception0,  0x8E);
	idt_set_entry(1,  (u64) exception1,  0x8E);
	idt_set_entry(2,  (u64) exception2,  0x8E);
	idt_set_entry(3,  (u64) exception3,  0x8E);
	idt_set_entry(4,  (u64) exception4,  0x8E);
	idt_set_entry(5,  (u64) exception5,  0x8E);
	idt_set_entry(6,  (u64) exception6,  0x8E);
	idt_set_entry(7,  (u64) exception7,  0x8E);
	idt_set_entry(8,  (u64) exception8,  0x8E);
	idt_set_entry(9,  (u64) exception9,  0x8E);
	idt_set_entry(10, (u64) exception10, 0x8E);
	idt_set_entry(11, (u64) exception11, 0x8E);
	idt_set_entry(12, (u64) exception12, 0x8E);
	idt_set_entry(13, (u64) exception13, 0x8E);
	idt_set_entry(14, (u64) exception14, 0x8E);
	idt_set_entry(15, (u64) exception15, 0x8E);
	idt_set_entry(16, (u64) exception16, 0x8E);
	idt_set_entry(17, (u64) exception17, 0x8E);
	idt_set_entry(18, (u64) exception18, 0x8E);
	idt_set_entry(19, (u64) exception19, 0x8E);
	idt_set_entry(20, (u64) exception20, 0x8E);
	idt_set_entry(21, (u64) exception21, 0x8E);
	idt_set_entry(22, (u64) exception22, 0x8E);
	idt_set_entry(23, (u64) exception23, 0x8E);
	idt_set_entry(24, (u64) exception24, 0x8E);
	idt_set_entry(25, (u64) exception25, 0x8E);
	idt_set_entry(26, (u64) exception26, 0x8E);
	idt_set_entry(27, (u64) exception27, 0x8E);
	idt_set_entry(28, (u64) exception28, 0x8E);
	idt_set_entry(29, (u64) exception29, 0x8E);
	idt_set_entry(30, (u64) exception30, 0x8E);
	idt_set_entry(31, (u64) exception31, 0x8E);

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

	// disable all irqs except keypress
	outb(0x21, 0xFD);
	outb(0xA1, 0xFF);
	irq_set_routine(INT_KEYBOARD, keyboard_routine);
	exception_set_handler(EXCEPT_PAGE_FAULT, page_fault_handler);

	idt_ptr.size = sizeof(idt) - 1;
	idt_ptr.offset = (u64) &idt;

	asm volatile("lidt %0; sti" :: "m"(idt_ptr));

	serial_info("Interrupts enabled");
}
