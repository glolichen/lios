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

extern void exception0(void);
extern void exception1(void);
extern void exception2(void);
extern void exception3(void);
extern void exception4(void);
extern void exception5(void);
extern void exception6(void);
extern void exception7(void);
extern void exception8(void);
extern void exception9(void);
extern void exception10(void);
extern void exception11(void);
extern void exception12(void);
extern void exception13(void);
extern void exception14(void);
extern void exception15(void);
extern void exception16(void);
extern void exception17(void);
extern void exception18(void);
extern void exception19(void);
extern void exception20(void);
extern void exception21(void);
extern void exception22(void);
extern void exception23(void);
extern void exception24(void);
extern void exception25(void);
extern void exception26(void);
extern void exception27(void);
extern void exception28(void);
extern void exception29(void);
extern void exception30(void);
extern void exception31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

void handle_exception(struct InterruptData *data);
void handle_interrupt(struct InterruptData *data);
void interrupt_init(void);

#endif
