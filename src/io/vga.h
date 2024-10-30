#ifndef VGA_H
#define VGA_H

#include "../const.h"

void vga_init(u8 *addr);
void vga_putpixel(u32 x, u32 y, u8 red, u8 green, u8 blue);
void vga_putchar(char c);
void vga_clear(void);

#endif
