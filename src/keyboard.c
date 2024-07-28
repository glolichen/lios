#include "keyboard.h"
#include "interrupt.h"
#include "io.h"
#include "output.h"

void keyboard_routine(struct InterruptData *data) {
	u8 scan = inb(0x60);
	fb_printf("key press #%u\n", scan);
}
