#include "keyboard.h"
#include "io.h"
#include "output.h"
#include "vga.h"
#include "../util/const.h"
#include "../util/misc.h"
#include "../int/interrupt.h"
#include "../testing.h"

#define LSHIFT 42
#define RSHIFT 54

#define ENTER 28

bool is_shift_held = false;

bool is_recording = false;

u8 *pressed;
u64 pressed_size = 0;

void keyboard_routine(const struct InterruptData *data) {
	u8 scan = inb(0x60);
	if (scan < MAX_KEYCODE) {
		if (scan == LSHIFT || scan == RSHIFT)
			is_shift_held = true;

		const char *str = KEYCODES[scan];
		if (strlen(str) == 1) {
			char c = str[0];
			if (is_shift_held) {
				switch (c) {
					case '0':
						c = ')';
						break;
					case '1':
						c = '!';
						break;
					case '2':
						c = '@';
						break;
					case '3':
						c = '#';
						break;
					case '4':
						c = '$';
						break;
					case '5':
						c = '%';
						break;
					case '6':
						c = '^';
						break;
					case '7':
						c = '&';
						break;
					case '8':
						c = '*';
						break;
					case '9':
						c = '(';
						break;
					case '`':
						c = '~';
						break;
					case '-':
						c = '_';
						break;
					case '=':
						c = '+';
						break;
					case '[':
						c = '{';
						break;
					case ']':
						c = '}';
						break;
					case '|':
						c = '\\';
						break;
					case ';':
						c = ':';
						break;
					case '\'':
						c = '"';
						break;
					case ',':
						c = '<';
						break;
					case '.':
						c = '>';
						break;
					case '/':
						c = '?';
						break;
					default: {
						if (c >= 'a' && c <= 'z')
							c += 'A' - 'a';
						break;
					}
				}
			}

			vga_putchar(c);
			if (is_recording) {
				pressed_size++;
			}
		}

		if (scan == 28) {
			is_recording = false;
			vga_printf("new line\n");
		}
	}
	else if (scan >= 0x81 && scan <= 0xD7) {
		u8 released_key = scan - 0x80;
		if (released_key == LSHIFT || released_key == RSHIFT)
			is_shift_held = false;

		// vga_printf("<%s released>\n", KEYCODES[scan - 0x80]);
	}
	else {
		// vga_printf("0x%x <\?\?\?>\n", scan);
	}

	// if (scan == 11)
	// 	test_div0();
}

void keyboard_start_recording(void) {
	pressed_size = 0;
	is_recording = true;
}
bool keyboard_is_recording(void) {
	return is_recording;
}
u32 keyboard_get_recording(void) {
	return pressed_size;
}

