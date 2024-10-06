#include "keyboard.h"
#include "const.h"
#include "interrupt.h"
#include "io.h"
#include "output.h"

void keyboard_routine(struct InterruptData *data) {
	u8 scan = inb(0x60);
	if (scan < MAX_KEYCODE) {
		fb_printf("%s\n", KEYCODES[scan]);
	}
	else if (scan >= 0x81 && scan <= 0xD7) {
		// fb_printf("<%s released>\n", KEYCODES[scan - 0x80]);
	}
	else {
		fb_printf("0x%x <\?\?\?>\n", scan);
	}
}
