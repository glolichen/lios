#include "io.h"
#include "const.h"

u8 inb(u16 port) {
	u8 val;
	asm volatile("in %b0, %d1" : "=a"(val) : "d"(port));
	return val;
}
void outb(u16 port, u8 data) {
	asm volatile("out %d0, %b1" :: "d"(port), "a"(data));
}
void io_wait() {
	outb(0x80, 0);
}