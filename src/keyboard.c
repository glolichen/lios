#include "keyboard.h"
#include "irq.h"
#include "io.h"
#include "output.h"

void keyboard_routine(struct Registers *regs) {
	u8 scan = inb(0x60);
	fb_printf("key press #%d\n", scan);
}
