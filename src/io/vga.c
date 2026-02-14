#include "vga.h"
#include "output.h"
#include "vgafont.h"
#include "../util/panic.h"
#include "../util/kmath.h"
#include "../mem/page.h"
#include "../mem/vmalloc.h"

#define QUERY_BIT(num, pos) ((num >> (pos)) & ((u64) 1))

u32 pixel_width, pixel_height, vga_pitch, char_width, char_height;

u32 cur_row, cur_col;
u8 *vga_virt, *vga_chars;

bool flash = false;

void vga_init(u8 *addr, u32 width, u32 height, u32 pitch) {
	pixel_width = width, pixel_height = height, vga_pitch = pitch;

	if (!addr)
		panic("vga: framebuffer init: no frame buffer tag!");

	vga_chars = (u8 *) vcalloc((pixel_height / 16) * (pixel_width / 8));

	u32 pages_needed = ceil_u32_div(vga_pitch * pixel_height, PAGE_SIZE);
	u64 virt = 0xFFFF800000000000;
	for (u32 i = 0; i < pages_needed; i++)
		page_map(virt + i * PAGE_SIZE, (PhysicalAddress) addr + i * PAGE_SIZE, true);

	serial_info("vga: initialize framebuffer at 0x%x virt -> 0x%x phys", virt, page_virt_to_phys_addr(virt));

	vga_virt = (u8 *) virt;
	cur_row = 0, cur_col = 0;
}

void vga_putpixel(u32 x, u32 y, u8 red, u8 green, u8 blue) {
	u32 location = y * vga_pitch + x * 4;
	vga_virt[location] = blue; // blue
	vga_virt[location + 1] = green; // green
	vga_virt[location + 2] = red; // red
}

void draw_char(u32 row, u32 col, char c, bool inverted) {
	u8 color;
	u32 x = col * 8, y = row * 16;
	u64 a = VGA_FONT[c * 2], b = VGA_FONT[c * 2 + 1];
	for (u32 i = 0; i < 8; i++) {
		for (u32 j = 0; j < 8; j++) {
			// whether a pixel is foreground: FG = QUERY_BIT(a, i * 8 + j)
			// if not inverting colors (IC = 0): is white color = FG = (FG != 0)
			// if inverting colors (IC = 1): is white color = !FG = (FG != 1)

			color = (inverted != QUERY_BIT(a, i * 8 + j)) ? 183 : 0;
			vga_putpixel(x + j, y + i, color, color, color);

			color = (inverted != QUERY_BIT(b, i * 8 + j)) ? 183 : 0;
			vga_putpixel(x + j, y + i + 8, color, color, color);
		}
	}
}

u32 get_pos(u32 row, u32 col) {
	return row * (pixel_width / 8) + col;
}

void vga_newline(void) {
	cur_col = 0;
	if (++cur_row < (pixel_height / 16))
		return;

	cur_row--;
	for (u32 i = 0; i < (pixel_height / 16) - 1; i++) {
		for (u32 j = 0; j < (pixel_width / 8); j++) {
			u32 line = get_pos(i, j);
			u32 next = get_pos(i + 1, j);
			vga_chars[line] = vga_chars[next];
			if (vga_chars[line])
				draw_char(i, j, vga_chars[line], false);
		}
	}
	for (u32 j = 0; j < (pixel_width / 8); j++) {
		vga_chars[get_pos(cur_row, j)] = ' ';
		draw_char(cur_row, j, ' ', false);
	}
}

void vga_putchar(char c) {
	if (vga_chars == 0) {
		serial_warn("vga: putchar before initialization");
		return;
	}

	if (c == '\n') {
		vga_newline();
		return;
	}

	// backspace
	if (c == '\b') {
		if (cur_col != 0)
			cur_col--;
		else if (cur_row != 0) {
			cur_col = pixel_width / 8 - 1;
			cur_row--;
		}
		return;
	}

	vga_chars[get_pos(cur_row, cur_col)] = c;
	draw_char(cur_row, cur_col, c, false);

	if (++cur_col >= (pixel_width / 8))
		vga_newline();
}

void vga_toggle_flash(void) {
	draw_char(cur_row, cur_col, vga_chars[get_pos(cur_row, cur_col)], flash = !flash);
}

void vga_clear(void) {
	cur_row = 0, cur_col = 0;
	for (u32 i = 0; i < pixel_width ; i++) {
		for (u32 j = 0; j < pixel_height; j++)
			vga_putpixel(i, j, 0, 0, 0);
	}
}

bool vga_is_initialized(void) {
	return vga_chars != 0;
}

