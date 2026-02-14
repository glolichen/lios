#include "keyboard.h"
#include "interrupt.h"
#include "../io/io.h"
#include "../io/output.h"
#include "../io/vga.h"
#include "../util/const.h"
#include "../util/misc.h"
#include "../util/panic.h"
#include "../mem/vmalloc.h"

#include "../testing.h"

#define LSHIFT 42
#define RSHIFT 54
#define CONTROL 29
#define SPACE 57
#define ENTER 28
#define BACKSPACE 14

bool is_shift_held = false, is_control_held = false;

bool is_recording = false;

u8 *pressed;
u64 pressed_size = 0, pressed_capacity = 0;

void keyboard_routine(const struct InterruptData *data) {
	u8 scan = inb(0x60);

	// vga_printf("%u\n", scan);
	
	if (scan < MAX_KEYCODE) {
		if (scan == LSHIFT || scan == RSHIFT)
			is_shift_held = true;
		if (scan == CONTROL)
			is_control_held = true;

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

			if (is_control_held) {
				if (c == 'm')
					panic("System Halted");
			}

			vga_putchar(c);
			if (is_recording) {
				// if adding another element to list would be larger than capacity
				if (pressed_size + 1 > pressed_capacity) {
					u8 *pressed_copy = pressed;
					pressed = (u8 *) vcalloc((pressed_capacity *= 2) * sizeof(u8));
					
					serial_info("new pressed: 0x%x, previous pressed: 0x%x", pressed, pressed_copy);

					for (u32 i = 0; i < pressed_size; i++)
						pressed[i] = pressed_copy[i];

					vfree(pressed_copy);
				}
				pressed[pressed_size++] = c;
			}
		}
		else if (scan == BACKSPACE) {
			vga_putchar('\b');
			vga_putchar(' ');
			vga_putchar('\b');

			// could change capacity, but won't
			if (is_recording)
				pressed[--pressed_size] = 0;
		}
		else if (scan == ENTER) {
			is_recording = false;
			vga_printf("\n");
		}
	}
	else if (scan >= 0x81 && scan <= 0xD7) {
		u8 released_key = scan - 0x80;
		if (released_key == LSHIFT || released_key == RSHIFT)
			is_shift_held = false;
		if (released_key == CONTROL)
			is_control_held = false;

		// vga_printf("<%s released>\n", KEYCODES[scan - 0x80]);
	}
	else {
		// vga_printf("0x%x <\?\?\?>\n", scan);
	}

	// if (scan == 11)
	// 	test_div0();
}

void keyboard_start_recording(void) {
	pressed_size = 0, pressed_capacity = 8;
	pressed = (u8 *) vcalloc(pressed_capacity * sizeof(u8));
	is_recording = true;
}

bool keyboard_is_recording(void) {
	return is_recording;
}

struct KeyboardRecordingList keyboard_get_recording(void) {
	struct KeyboardRecordingList ret;
	ret.pressed_keys = pressed;
	ret.length = pressed_size;
	return ret;
}

void keyboard_end_recording(void) {
	pressed_size = 0, pressed_capacity = 0;
	vfree(pressed);
	is_recording = false;
}

