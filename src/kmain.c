#include "io.h"
#include "const.h"
// #include "gdt.h"
#include "idt.h"
#include "irq.h"
// #include "pic.h"
#include "serial.h"
#include "multiboot.h"
#include "output.h"
// #include "pmm.h"
// #include "page.h"

// extern u32 kernel_end;

// void kmain(multiboot_info_t *info, u32 magic, u16 size) {
// 	serial_init();

// 	pmm_init(info, magic, (u32) &kernel_end);

// 	gdt_init();
// 	idt_init();
// 	irq_init();

//     asm("sti");
// 	serial_info("Interrupts enabled");

// 	fb_init();
// 	fb_clear();

// 	fb_printf("Hello world!\n");

// 	fb_printf("%x\n", info);
// 	fb_printf("%d %d\n", magic, info->flags);
// }

struct __attribute__((packed)) GDTEntryTSS {
	u16 limit;
	u16 base_low;
	u8 base_mid;
	u8 access;
	u8 flags_and_limit;
	u8 base_high;
	u32 base_highest;
	u32 reserved;
};

void kmain(struct GDTEntryTSS *tss_entry, u64 tss_start, u64 tss_end, multiboot_info_t *info) {
	// TODO FIX THE SERIAL AT SOME POINT
	serial_init();

	fb_init();
	fb_printf("Hello world!\n");

	u64 limit = tss_end - tss_start;
	tss_entry->limit = limit & 0xFFFF;
	tss_entry->base_low = tss_start & 0xFFFF;
	tss_entry->base_mid = (tss_start >> 16) & 0xFF;
	tss_entry->access = 0x89;
	tss_entry->flags_and_limit = 0x40;
	tss_entry->flags_and_limit = ((limit >> 16) & 0xF) | (0 << 4);
	tss_entry->base_high = (tss_start >> 24) & 0xFF;
	tss_entry->base_highest = (tss_start >> 32) & 0xFFFFFFFF;
	tss_entry->reserved = 0;

	asm volatile("mov ax, 0x18; ltr ax");

	idt_init();

	asm volatile("xchg bx, bx; sti");

	// for (int i = 0; i < 100; i++)
	// 	fb_printf("OK %d\n", i);
}
