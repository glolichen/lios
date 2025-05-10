#include "syscall.h"
#include "../io/vga.h"
#include "../io/output.h"
#include "../io/keyboard.h"
#include "../io/io.h"
#include "../util/const.h"
#include "../int/interrupt.h"

#include "../testing.h"

void syscall_routine(const struct InterruptData *data) {
	u64 id = data->rax;
	switch (id) {
		// read
		case 0: {
			u64 dest = data->rdi;

			// rdi = 0: standard input
			if (dest == 0) {
				char *buffer = (char *) data->rsi;
				u64 size = data->rdx;

				vga_printf("received read syscall\n");

				asm volatile("jmp $");

				// keyboard_start_recording();
				// while (keyboard_is_recording());
				//
				// vga_printf("you pressed: %u keys\n", keyboard_get_recording());
			}

			break;
		}

		// write
		case 1: {
			u64 dest = data->rdi;

			// rdi = 1: standard output
			if (dest == 1) {
				char *buffer = (char *) data->rsi;
				u64 size = data->rdx;
				for (u64 i = 0; i < size; i++)
					vga_putchar(buffer[i]);
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

