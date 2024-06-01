#include "fb.h"
#include "io.h"
#include "const.h"

u32 cur_row, cur_col;
char *fb;

u32 get_pos(u32 row, u32 col) {
	return col * 2 + row * FB_COLS * 2;
}

void fb_init() {
	cur_row = 0, cur_col = 0;
	fb = (char *) FB_ADDRESS;
}

void fb_write_cell(u32 pos, char c, FBColor fg, FBColor bg) {
	fb[pos] = c;
	fb[pos + 1] = MAKE_COLOR(fg, bg);
}

void fb_move_cursor(u16 pos) {
	outb(FB_COMMAND_PORT, FB_COMMAND_HIGH_BYTE_COMMAND);
	outb(FB_DATA_PORT, pos & SHORT_HIGH_BYTE);
	outb(FB_COMMAND_PORT, FB_COMMAND_LOW_BYTE_COMMAND);
	outb(FB_DATA_PORT, pos & SHORT_LOW_BYTE);
}

void fb_newline() {
	cur_col = 0;
	if (++cur_row < FB_ROWS)
		return;
	cur_row--;
	for (u32 i = 0; i < FB_ROWS - 1; i++) {
		for (u32 j = 0; j < FB_COLS; j++) {
			u32 curLine = get_pos(i, j);
			u32 nextLine = get_pos(i + 1, j);
			fb[curLine] = fb[nextLine];
			fb[curLine + 1] = fb[nextLine + 1];
		}
	}
	for (u32 j = 0; j < FB_COLS; j++)
		fb[get_pos(cur_row, j)] = ' ';
}

void fb_putchar(char c) {
	if (c == '\n') {
		fb_newline();
		return;
	}
	fb_write_cell(get_pos(cur_row, cur_col), c, LIGHT_GRAY, BLACK);
	if (++cur_col >= FB_COLS)
		fb_newline();
	fb_move_cursor(get_pos(cur_row, cur_col));
}

void fb_clear() {
	fb_move_cursor(0);
	for (u32 i = 0; i < FB_ROWS ; i++) {
		for (u32 j = 0; j < FB_COLS; j++)
			fb_write_cell(get_pos(i, j), ' ', LIGHT_GRAY, BLACK);
	}
}
