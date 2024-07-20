#include "isr.h"
#include "idt.h"
#include "const.h"
#include "output.h"

__attribute__((noreturn))
void isr_handle_exception(u64 isr_number, u64 error_code) {
	fb_printf("ISR %x: %s, code %d\n", isr_number, EXCEPTIONS[isr_number], error_code);
	asm("cli; hlt");
	while (1);
}
