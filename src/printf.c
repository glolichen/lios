#include <stdarg.h>
#include <stdbool.h>

#include "fb.h"
#include "const.h"
#include "serial.h"
#include "printf.h"

const u32 U32_MAX_LENGTH = 10;
const u32 U64_MAX_LENGTH = 19;
const u64 POWERS_10[] = {
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000,
	10000000000,
	100000000000,
	1000000000000,
	10000000000000,
	100000000000000,
	1000000000000000,
	10000000000000000,
	100000000000000000,
	1000000000000000000,
};

typedef enum {
	FRAME_BUFFER, SERIAL
} Destination;

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
			else if (*(format + 1) == 'd' || *(format + 1) == 'x') {
				bool is64 = *(format + 1) == 'x';
				u64 num = is64 ? va_arg(*arg, u64) : va_arg(*arg, u32);
				if (num == 0) {
					putchar('0', dest);
					length++;
				}
				else {
					bool hasPrinted = false;
					for (int i = (is64 ? U64_MAX_LENGTH : U32_MAX_LENGTH) - 1; i >= 0; i--) {
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
	print(SERIAL, "DEBUG: ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	print(SERIAL, "\n", 1);
	return length;
}
u32 serial_info(const char *format, ...) {
	print(SERIAL, "INFO: ", 6);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	print(SERIAL, "\n", 1);
	return length;
}
u32 serial_warn(const char *format, ...) {
	print(SERIAL, "WARN: ", 6);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	print(SERIAL, "\n", 1);
	return length;
}
u32 serial_error(const char *format, ...) {
	print(SERIAL, "ERROR: ", 7);

	va_list arg;
	va_start(arg, format);

	u32 length = printf(SERIAL, format, &arg);
	va_end(arg);

	print(SERIAL, "\n", 1);
	return length;
}