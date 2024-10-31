#include "panic.h"
#include "io/output.h"
#include "io/vga.h"

__attribute__((noreturn))
void panic(const char *msg) {
	serial_error("panic: %s", msg);
	vga_printf("panic: %s\n", msg);
	asm volatile("cli; hlt" ::: "memory");
	// thankfully this is not UB in C
	while (1);
}
