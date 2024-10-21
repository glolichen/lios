#include "panic.h"
#include "io/output.h"

__attribute__((noreturn))
void panic(char *msg) {
	vga_printf("kernel panic: %s\n", msg);
	serial_error("kernel panic: %s\n", msg);
	asm volatile("cli; hlt" ::: "memory");
	// thankfully this is not UB in C
	while (1);
}
