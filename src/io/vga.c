#include "vga.h"
#include "../panic.h"
#include "../mem/page.h"
#include "../mem/pmm.h"
#include "../kmath.h"
#include "output.h"
#include "vgafont.h"

#define QUERY_BIT(num, pos) ((num >> (pos)) & ((u64) 1))

u32 cur_row, cur_col;
u8 *fb_virt;

void putpixel(u32 x, u32 y, u8 red, u8 green, u8 blue) {
	u32 location = y * 4096 + x * 4;
	fb_virt[location] = blue; // blue
	fb_virt[location + 1] = green; // green
	fb_virt[location + 2] = red; // red
}

void vga_init(u8 *addr) {
	if (!addr)
		panic("vga: framebuffer init: no frame buffer tag!");

	pmm_clear_blocks((u64) addr, (u64) addr + FRAMEBUFFER_SIZE);
	u32 pages_needed = ceil_u32_div(FRAMEBUFFER_SIZE, PAGE_SIZE);
	u64 virt = 0xFFFF800000000000;
	for (u32 i = 0; i < pages_needed; i++)
		page_map(virt + i * PAGE_SIZE, (PhysicalAddress) addr + i * PAGE_SIZE);

	serial_info("vga: initialize framebuffer at 0x%x virt -> 0x%x phys", virt, page_virt_to_phys_addr(virt));

	fb_virt = (u8 *) virt;
	cur_row = 0, cur_col = 0;

	for (u32 i = 0; i < 1000; i++)
		putpixel(i, 0, 255, 255, 255);
}

void vga_newline() {
	serial_info("a");
	// cur_col = 0;
	// if (++cur_row < FB_ROWS)
	// 	return;
	// cur_row--;
	// for (u32 i = 0; i < FB_ROWS - 1; i++) {
	// 	for (u32 j = 0; j < FB_COLS; j++) {
	// 		u32 curLine = get_pos(i, j);
	// 		u32 nextLine = get_pos(i + 1, j);
	// 		fb[curLine] = fb[nextLine];
	// 		fb[curLine + 1] = fb[nextLine + 1];
	// 	}
	// }
	// for (u32 j = 0; j < FB_COLS; j++)
	// 	fb[get_pos(cur_row, j)] = ' ';
}

void vga_putchar(char c) {
	if (c == '\n') {
		vga_newline();
		return;
	}

	u8 color;
	u32 x = cur_col * 8, y = cur_row * 16;
	u64 a = VGA_FONT[c * 2], b = VGA_FONT[c * 2 + 1];
	for (u32 i = 0; i < 8; i++) {
		for (u32 j = 0; j < 8; j++) {
			color = QUERY_BIT(a, i * 8 + j) ? 183 : 0;
			putpixel(x + j, y + i, color, color, color);

			color = QUERY_BIT(b, i * 8 + j) ? 183 : 0;
			putpixel(x + j, y + i + 8, color, color, color);
		}
	}

	serial_info("%u", cur_col);
	if (++cur_col >= VGA_COLS)
		vga_newline();
	// fb_move_cursor(get_pos(cur_row, cur_col));
}

// void fb_clear() {
// 	fb_move_cursor(0);
// 	for (u32 i = 0; i < FB_ROWS ; i++) {
// 		for (u32 j = 0; j < FB_COLS; j++)
// 			fb_write_cell(get_pos(i, j), ' ', LIGHT_GRAY, BLACK);
// 	}
// 	serial_info("Frame buffer cleared");
// }

