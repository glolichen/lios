#include "syscall.h"
#include "../io/vga.h"
#include "../io/output.h"
#include "../util/const.h"
#include "../int/interrupt.h"

void syscall_routine(const struct InterruptData *data) {
	u64 id = data->rax;
	switch (id) {
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

