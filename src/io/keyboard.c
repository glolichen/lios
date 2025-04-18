#include "keyboard.h"
#include "io.h"
#include "output.h"
#include "../util/const.h"
#include "../int/interrupt.h"
#include "../testing.h"

void keyboard_routine(const struct InterruptData *data) {
	u8 scan = inb(0x60);
	if (scan < MAX_KEYCODE) {
		vga_printf("%s\n", KEYCODES[scan]);
	}
	else if (scan >= 0x81 && scan <= 0xD7) {
		vga_printf("<%s released>\n", KEYCODES[scan - 0x80]);
	}
	else {
		vga_printf("0x%x <\?\?\?>\n", scan);
	}

	if (scan == 11)
		test_div0();
}

