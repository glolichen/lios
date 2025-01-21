#include "io.h"
#include "../util/const.h"

u8 inb(u16 port) {
	u8 val;
	asm volatile("in %b0, %w1" : "=a"(val) : "Nd"(port));
	return val;
}
void outb(u16 port, u8 data) {
	asm volatile("out %w0, %b1" :: "Nd"(port), "a"(data));
}
void io_wait(void) {
	outb(0x80, 0);
}
