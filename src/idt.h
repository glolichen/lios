#ifndef IDT_H
#define IDT_H

#include "const.h"

struct __attribute__((packed)) IDTEntry {
	u16 isr_low;
	u16 segment; // kernel code segment
	u8 reserved;
	u8 attributes;
	u16 isr_high;
};

struct __attribute__((packed)) IDTPointer {
	u16 size; // aka limit
	u32 offset; // aka base
};

extern struct IDTEntry idt[256];
extern struct IDTPointer idt_ptr;

void idt_load();
void idt_init();
void idt_set_entry(u8 index, u32 isr, u8 flags) ;

#endif