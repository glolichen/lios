#ifndef VGA_H
#define VGA_H

#include "../const.h"

void vga_init(u8 *addr);
void vga_putchar(char c);
void vga_clear();

#endif
