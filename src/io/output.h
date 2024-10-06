#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdarg.h>
#include "../const.h"

void fb_init();
void fb_putchar(char c);
void fb_clear();

u32 fb_printf(const char *format, ...);

u32 serial_debug_no_line(const char *format, ...);
u32 serial_info_no_line(const char *format, ...);
u32 serial_warn_no_line(const char *format, ...);
u32 serial_error_no_line(const char *format, ...);

u32 serial_debug(const char *format, ...);
u32 serial_info(const char *format, ...);
u32 serial_warn(const char *format, ...);
u32 serial_error(const char *format, ...);

#endif
