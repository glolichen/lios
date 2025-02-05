#include <stdbool.h>
#include "hexdump.h"
#include "const.h"
#include "../io/output.h"

void print_byte(const u8 *src, u64 index) {
	serial_print_no_fancy(src[index] <= 0xF ? "0%x " : "%x ", src[index]);
}
void print_byte_ascii(const u8 *src, u64 index) {
	// http://facweb.cs.depaul.edu/sjost/it212/documents/ascii-pr.htm
	u8 byte = src[index];
	if (byte >= 32 && byte <= 126)
		serial_print_no_fancy("%c", byte);
	else
		serial_print_no_fancy(".");
}

void hexdump(const void *src, u64 len, bool find_readable) {
	serial_debug("");
	serial_debug("hexdumping addr 0x%x:", src);
	for (u64 i = 0; i < len / 16; i++) {
		serial_print_no_fancy("DEBUG:     ");

		for (u64 j = 0; j < 8; j++)
			print_byte((const u8 *) src, i * 16 + j);
		serial_print_no_fancy("  ");
		for (u64 j = 0; j < 8; j++)
			print_byte((const u8 *) src, i * 16 + j + 8);

		if (!find_readable) {
			serial_print_no_fancy("\n");
			continue;
		}

		serial_print_no_fancy(" |");

		for (u64 j = 0; j < 16; j++)
			print_byte_ascii((const u8 *) src, i * 16 + j);

		serial_print_no_fancy("|\n");
	}

	serial_print_no_fancy("DEBUG:     ");
	u32 chars_printed = 0;
	for (u64 i = 0; i < len % 16; i++) {
		u64 index = (len / 16) * 16 + i;
		print_byte((const u8 *) src, index);
		chars_printed += 3;
		if (i == 7) {
			serial_print_no_fancy("  ");
			chars_printed += 2;
		}
	}
	for (u32 i = chars_printed; i < 51; i++)
		serial_print_no_fancy(" ");
	serial_print_no_fancy("|");

	for (u64 i = 0; i < len % 16; i++) {
		u64 index = (len / 16) * 16 + i;
		print_byte_ascii((const u8 *) src, index);
	}
	for (u64 i = len % 16; i < 16; i++)
		serial_print_no_fancy(" ");

	serial_print_no_fancy("|\n");
	serial_debug("");
}

