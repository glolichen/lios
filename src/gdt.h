#ifndef GDT_H
#define GDT_H

#include "const.h"

// https://wiki.osdev.org/Global_Descriptor_Table#Segment_Descriptor
struct __attribute__((packed)) GDTEntry {
	u16 limit_low;
	u16 base_low;
	u8 base_mid;
	u8 access;
	u8 limit_high_and_flags;
	u8 base_high;
};

// https://wiki.osdev.org/Global_Descriptor_Table#GDTR
struct __attribute__((packed)) GDTPointer {
	u16 size; // aka limit
	u32 offset; // aka base
};

void gdt_load(u16 size, u32 offset);
void gdt_init();

void reloadSegments();

#endif 
