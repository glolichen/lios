#include "gdt.h"
#include "printf.h"
#include "const.h"

struct GDTPointer gdt_ptr;
struct GDTEntry gdt[3];

void gdt_set_entry(u8 index, u32 base, u32 limit, u8 access, u8 flags) {
	gdt[index].limit_low = limit & 0xFFFF;
	gdt[index].base_low = base & 0xFFFF;
	gdt[index].base_mid = (base >> 16) & 0xFF;
	gdt[index].access = access;
	gdt[index].limit_high_and_flags = ((limit >> 16) & 0xF) | ((flags & 0xF) << 4);
	gdt[index].base_high = (base >> 24) & 0xFF;
}

void gdt_init() {
	gdt_set_entry(0, 0, 0, 0, 0);
	gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9B, 0xC);
	gdt_set_entry(2, 0, 0xFFFFFFFF, 0x93, 0xC);

	serial_info("GDT populated");

	gdt_ptr.size = sizeof(gdt) - 1;
	gdt_ptr.offset = (u32) &gdt;

	gdt_load(gdt_ptr.size, gdt_ptr.offset);

	serial_info("GDT loaded");
}

