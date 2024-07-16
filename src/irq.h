#ifndef IRQ_H
#define IRQ_H

#include "const.h"

struct __attribute__((packed)) Registers {
	u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
};

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

void irq_set_routine(u8 irq_number, void (*routine)(struct Registers *));
void irq_handle_interrupt(struct Registers regs, u8 irq_number, u8 error_code);
void irq_init();

#endif
