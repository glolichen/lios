#ifndef VGA_H
#define VGA_H

#include <stdbool.h>
#include "../const.h"

void vga_init(u8 *addr, u32 width, u32 height, u32 pitch);
void vga_putpixel(u32 x, u32 y, u8 red, u8 green, u8 blue);
void vga_putchar(char c);
void vga_clear(void);
bool vga_is_initialized(void);

#endif
