#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "const.h"

struct __attribute__((packed)) InterruptData {
	u64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
	u64 interrupt_num, error_code;
	u64 rip, cs, rflags, rsp, ss;
};

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

extern void exception0();
extern void exception1();
extern void exception2();
extern void exception3();
extern void exception4();
extern void exception5();
extern void exception6();
extern void exception7();
extern void exception8();
extern void exception9();
extern void exception10();
extern void exception11();
extern void exception12();
extern void exception13();
extern void exception14();
extern void exception15();
extern void exception16();
extern void exception17();
extern void exception18();
extern void exception19();
extern void exception20();
extern void exception21();
extern void exception22();
extern void exception23();
extern void exception24();
extern void exception25();
extern void exception26();
extern void exception27();
extern void exception28();
extern void exception29();
extern void exception30();
extern void exception31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

void handle_exception(struct InterruptData *data);
void handle_interrupt(struct InterruptData *data);
void interrupt_init();

#endif
