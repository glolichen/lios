#ifndef IDT_H
#define IDT_H

#include "const.h"

struct __attribute__((packed)) IDTEntry {
	u16 isr_low;
	u16 segment; // kernel code segment
	u8 ist; // low 3 IST, rest reserved
	u8 attributes;
	u16 isr_mid;
	u32 isr_high;
	u32 reserved;
};

struct __attribute__((packed)) IDTPointer {
	u16 size; // aka limit
	u64 offset; // aka base
};

void idt_init();
void idt_set_entry(u8 index, u64 isr, u8 flags) ;

#endif
