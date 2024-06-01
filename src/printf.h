#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include "const.h"

u32 fb_printf(const char *format, ...);
u32 serial_debug(const char *format, ...);
u32 serial_info(const char *format, ...);
u32 serial_warn(const char *format, ...);
u32 serial_error(const char *format, ...);

#endif