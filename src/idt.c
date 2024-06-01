#include <stdbool.h>
#include "idt.h"
#include "io.h"
#include "printf.h"
#include "pic.h"
#include "isr.h"
#include "irq.h"
#include "const.h"

struct IDTEntry idt[256];
struct IDTPointer idt_ptr;

void idt_set_entry(u8 index, u32 isr, u8 flags) { 
    idt[index].isr_low = isr & 0xFFFF;
    idt[index].isr_high = (isr >> 16) & 0xFFFF;
    idt[index].segment = 0x08;
    idt[index].attributes = flags;
    idt[index].reserved = 0;
}
 
void idt_init() {
	for (u16 i = 0; i < 256; i++)
		idt_set_entry(i, 0, 0);

	idt_ptr.size = sizeof(idt) - 1;
	idt_ptr.offset = (u32) &idt;

	isr_init();
    idt_load(idt_ptr.size, idt_ptr.offset);
}
