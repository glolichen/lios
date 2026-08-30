#include "syscall.h"
#include "keyboard.h"
#include "../io/vga.h"
#include "../io/output.h"
#include "../int/interrupt.h"
#include "../util/const.h"
#include "../util/misc.h"

u64 min(u64 a, u64 b) {
	return a < b ? a : b;
}

void syscall_routine(const struct InterruptData *data) {
	u64 id = data->rax;
	switch (id) {
		// read
		case 0: {
			u64 dest = data->rdi;

			// rdi = 0: standard input
			if (dest == 0) {
				u8 *buffer = (u8 *) data->rsi;
				u64 size = data->rdx;
				keyboard_start_recording();
				while (keyboard_is_recording());
				struct KeyboardRecordingList recording = keyboard_get_recording();
				memset(buffer, 0, size);
				memcpy(buffer, recording.pressed_keys, min(size, recording.length));
				keyboard_end_recording();
			}

			break;
		}

		// write
		case 1: {
			u64 dest = data->rdi;

			// rdi = 1: standard output
			if (dest == 1) {
				u8 *buffer = (u8 *) data->rsi;
				u64 size = data->rdx;
				for (u64 i = 0; i < size; i++) {
					if (buffer[i] != 0)
						vga_putchar(buffer[i]);
				}
			}

			break;
		}

		// top secret hello world test
		case 999: {
			vga_printf("Hello world!\n");
			break;
		}
	}
}

