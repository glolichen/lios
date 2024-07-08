#include "printf.h"
#include "panic.h"

__attribute__((noreturn))
void panic(char *msg) {
	fb_printf("kernel panic: %s\n", msg);
	serial_error("kernel panic: %s\n", msg);
	asm("cli; hlt");
}