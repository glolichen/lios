#include <stdarg.h>
#include <stdbool.h>

#include "io.h"
#include "serial.h"
#include "output.h"
#include "../const.h"
#include "../panic.h"
#include "../kmath.h"
#include "../mem/pmm.h"
#include "../mem/vmm.h"
#include "../mem/page.h"

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

void putchar(char c, Destination dest) {
	// if (dest == FRAME_BUFFER)
	// 	fb_putchar(c);
	// else
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

