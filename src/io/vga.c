#include "vga.h"
#include "../panic.h"
#include "../mem/page.h"
#include "../mem/pmm.h"
#include "../mem/vmalloc.h"
#include "../kmath.h"
#include "output.h"
#include "vgafont.h"

#define QUERY_BIT(num, pos) ((num >> (pos)) & ((u64) 1))

u32 cur_row, cur_col;
u8 *vga_virt, *vga_chars;

void vga_init(u8 *addr) {
	if (!addr)
		panic("vga: framebuffer init: no frame buffer tag!");

	// vga_chars = (u8 *) vmalloc(VGA_ROWS * VGA_COLS);

	u32 pages_needed = ceil_u32_div(FRAMEBUFFER_SIZE, PAGE_SIZE);
	u64 virt = 0xFFFF900000000000;
	for (u32 i = 0; i < pages_needed; i++)
		page_map(virt + i * PAGE_SIZE, (PhysicalAddress) addr + i * PAGE_SIZE);

	serial_info("vga: initialize framebuffer at 0x%x virt -> 0x%x phys", virt, page_virt_to_phys_addr(virt));

	vga_virt = (u8 *) virt;
	cur_row = 0, cur_col = 0;
}

void vga_putpixel(u32 x, u32 y, u8 red, u8 green, u8 blue) {
	u32 location = y * 4096 + x * 4;
	vga_virt[location] = blue; // blue
	vga_virt[location + 1] = green; // green
	vga_virt[location + 2] = red; // red
}
void draw_char(u32 row, u32 col, char c) {
	u8 color;
	u32 x = col * 8, y = row * 16;
	u64 a = VGA_FONT[c * 2], b = VGA_FONT[c * 2 + 1];
	for (u32 i = 0; i < 8; i++) {
		for (u32 j = 0; j < 8; j++) {
			color = QUERY_BIT(a, i * 8 + j) ? 183 : 0;
			vga_putpixel(x + j, y + i, color, color, color);

			color = QUERY_BIT(b, i * 8 + j) ? 183 : 0;
			vga_putpixel(x + j, y + i + 8, color, color, color);
		}
	}

}

u32 get_pos(u32 row, u32 col) {
	return row * VGA_COLS + col;
}

void vga_newline(void) {
	cur_col = 0;
	if (++cur_row < VGA_ROWS)
		return;

	cur_row--;
	for (u32 i = 0; i < VGA_ROWS - 1; i++) {
		for (u32 j = 0; j < VGA_COLS; j++) {
			u32 line = get_pos(i, j);
			u32 next = get_pos(i + 1, j);
			vga_chars[line] = vga_chars[next];
			if (vga_chars[line])
				draw_char(i, j, vga_chars[line]);
		}
	}
	for (u32 j = 0; j < VGA_COLS; j++) {
		vga_chars[get_pos(cur_row, j)] = ' ';
		draw_char(cur_row, j, ' ');
	}
}

void vga_putchar(char c) {
	if (c == '\n') {
		vga_newline();
		return;
	}

	vga_chars[get_pos(cur_row, cur_col)] = c;
	draw_char(cur_row, cur_col, c);

	if (++cur_col >= VGA_COLS)
		vga_newline();
}

void vga_clear(void) {
	for (u32 i = 0; i < 1024 ; i++) {
		for (u32 j = 0; j < 768; j++)
			vga_putpixel(i, j, 0, 0, 0);
	}
}

