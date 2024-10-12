#include <stdarg.h>
#include <stdbool.h>

#include "io.h"
#include "serial.h"
#include "output.h"
#include "../const.h"
#include "../panic.h"
#include "../mem/pmm.h"

const u32 U64_MAX_LENGTH_DEC = 20;
const u64 POWERS_10[] = {
	1ULL,
	10ULL,
	100ULL,
	1000ULL,
	10000ULL,
	100000ULL,
	1000000ULL,
	10000000ULL,
	100000000ULL,
	1000000000ULL,
	10000000000ULL,
	100000000000ULL,
	1000000000000ULL,
	10000000000000ULL,
	100000000000000ULL,
	1000000000000000ULL,
	10000000000000000ULL,
	100000000000000000ULL,
	1000000000000000000ULL,
	10000000000000000000ULL
};

const u32 U32_MAX_LENGTH_HEX = 16;
const u64 POWERS_16[] = {
	0x1ULL,
	0x10ULL,
	0x100ULL,
	0x1000ULL,
	0x10000ULL,
	0x100000ULL,
	0x1000000ULL,
	0x10000000ULL,
	0x100000000ULL,
	0x1000000000ULL,
	0x10000000000ULL,
	0x100000000000ULL,
	0x1000000000000ULL,
	0x10000000000000ULL,
	0x100000000000000ULL,
	0x1000000000000000ULL
};

typedef enum {
	FRAME_BUFFER, SERIAL
} Destination;

u32 cur_row, cur_col;
char *fb;

u32 get_pos(u32 row, u32 col) {
	return col * 2 + row * FB_COLS * 2;
}

void fb_init(u8 *addr) {
	if (!addr)
		panic("output: framebuffer init: no frame buffer tag!");

	pmm_clear_blocks((u64) addr, (u64) addr + FRAMEBUFFER_SIZE);

	cur_row = 0, cur_col = 0;
	fb = (char *) FB_ADDRESS;
	serial_info("output: frame buffer initialized");
	// fb_clear();
}

void fb_write_cell(u32 pos, char c, enum FBColor fg, enum FBColor bg) {
	fb[pos] = c;
	fb[pos + 1] = MAKE_COLOR(fg, bg);
}

void fb_move_cursor(u16 pos) {
	outb(FB_COMMAND_PORT, FB_COMMAND_HIGH_BYTE_COMMAND);
	outb(FB_DATA_PORT, (pos & U16_HIGH_BYTE) >> 8);
	outb(FB_COMMAND_PORT, FB_COMMAND_LOW_BYTE_COMMAND);
	outb(FB_DATA_PORT, pos & U16_LOW_BYTE);
}

void fb_newline() {
	cur_col = 0;
	if (++cur_row < FB_ROWS)
		return;
	cur_row--;
	for (u32 i = 0; i < FB_ROWS - 1; i++) {
		for (u32 j = 0; j < FB_COLS; j++) {
			u32 curLine = get_pos(i, j);
			u32 nextLine = get_pos(i + 1, j);
			fb[curLine] = fb[nextLine];
			fb[curLine + 1] = fb[nextLine + 1];
		}
	}
	for (u32 j = 0; j < FB_COLS; j++)
		fb[get_pos(cur_row, j)] = ' ';
}

void fb_putchar(char c) {
	if (c == '\n') {
		fb_newline();
		return;
	}
	fb_write_cell(get_pos(cur_row, cur_col), c, LIGHT_GRAY, BLACK);
	if (++cur_col >= FB_COLS)
		fb_newline();
	fb_move_cursor(get_pos(cur_row, cur_col));
}

void fb_clear() {
	fb_move_cursor(0);
	for (u32 i = 0; i < FB_ROWS ; i++) {
		for (u32 j = 0; j < FB_COLS; j++)
			fb_write_cell(get_pos(i, j), ' ', LIGHT_GRAY, BLACK);
	}
	serial_info("Frame buffer cleared");
}

void putchar(char c, Destination dest) {
	if (dest == FRAME_BUFFER)
		fb_putchar(c);
	else
		serial_putchar(c);
}

void print(Destination dest, const char *text, u32 len) {
	for (u32 i = 0; i < len; i++)
		putchar(text[i], dest);
}

// shitty printf implementation
// only supports printing 32-bit integers, chars and strings
u32 printf(Destination dest, const char *format, va_list *arg) {
	u32 length = 0;
	while (*format != '\0') {
		if (*format == '%') {
			if (*(format + 1) == '%') {
				putchar('%', dest);
				length++;
			}
			else if (*(format + 1) == 'u') {
				u64 num = va_arg(*arg, u64);
				if (num == 0) {
					putchar('0', dest);
					length++;
				}
				else {
					bool hasPrinted = false;
					for (i32 i = U64_MAX_LENGTH_DEC - 1; i >= 0; i--) {
						u64 power = POWERS_10[i];
						u64 divide = num / power;
						if (divide == 0) {
							if (hasPrinted) {
								putchar('0', dest);
								length++;
							}
							continue;
						}
						hasPrinted = true;
						putchar(divide + '0', dest);
						length++;
						num = num % power;
					}
				}
			}
			else if (*(format + 1) == 'x') {
				u64 num = va_arg(*arg, u64);

				if (num == 0) {
					putchar('0', dest);
					length++;
				}
				else {
					bool hasPrinted = false;
					for (i32 i = U32_MAX_LENGTH_HEX - 1; i >= 0; i--) {
						u64 power = POWERS_16[i];
						u64 divide = num / power;
						if (divide == 0) {
							if (hasPrinted) {
								putchar('0', dest);
								length++;
							}
							continue;
						}
						hasPrinted = true;
						putchar(divide + (divide <= 9 ? '0' : ('A' - 10)), dest);
						length++;
						num = num % power;
					}
				}
			}
			else if (*(format + 1) == 'c') {
				putchar((char) va_arg(*arg, int), dest);
				length++;
			}
			else if (*(format + 1) == 's') {
				char *str = va_arg(*arg, char *);
				while (*str != '\0') {
					putchar(*str, dest);
					length++;
					str++;
				}
			}
			format += 2;
			continue;
		}
		putchar(*format, dest);
		length++;
		format++;
	}

	va_end(*arg);
	return length;
}

u32 fb_printf(const char *format, ...) {
	va_list arg;
	va_start(arg, format);
	u32 length = printf(FRAME_BUFFER, format, &arg);
	va_end(arg);
	return length;
}

u32 serial_debug_no_line(const char *format, ...) {
	if (!PRINT_INFO_SERIAL)
		return 0;
	
	print(SERIAL, "DEBUG: ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	return length;
}
u32 serial_info_no_line(const char *format, ...) {
	if (!PRINT_INFO_SERIAL)
		return 0;

	print(SERIAL, "INFO:  ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	return length;
}
u32 serial_warn_no_line(const char *format, ...) {
	print(SERIAL, "WARN:  ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	return length;
}
u32 serial_error_no_line(const char *format, ...) {
	print(SERIAL, "ERROR: ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	return length;
}

u32 serial_debug(const char *format, ...) {
	if (!PRINT_INFO_SERIAL)
		return 0;
	
	print(SERIAL, "DEBUG: ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	putchar('\n', SERIAL);
	return length;
}
u32 serial_info(const char *format, ...) {
	if (!PRINT_INFO_SERIAL)
		return 0;

	print(SERIAL, "INFO:  ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	putchar('\n', SERIAL);
	return length;
}
u32 serial_warn(const char *format, ...) {
	print(SERIAL, "WARN:  ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	putchar('\n', SERIAL);
	return length;
}
u32 serial_error(const char *format, ...) {
	print(SERIAL, "ERROR: ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	putchar('\n', SERIAL);
	return length;
}

