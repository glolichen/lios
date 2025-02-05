#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdarg.h>
#include <stdbool.h>
#include "../util/const.h"

u32 vga_printf(const char *format, ...);

u32 serial_debug(const char *format, ...);
u32 serial_info(const char *format, ...);
u32 serial_warn(const char *format, ...);
u32 serial_error(const char *format, ...);

u32 serial_print_no_fancy(const char *format, ...);

#endif
