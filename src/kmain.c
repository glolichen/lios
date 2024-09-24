#include "const.h"
#include "interrupt.h"
#include "page.h"
#include "pmm.h"
#include "serial.h"
#include "multiboot2.h"
#include "output.h"
#include "vmm.h"

extern u64 kernel_end;

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

void kmain(struct GDTEntryTSS *tss_entry, u64 tss_start, u64 tss_end, u64 mboot_addr,
			u64 pml4[512], u64 pdpt_low[512], u64 pdt_low[512], u64 pt_low[512]) {
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
	asm volatile("ltr %d0" :: "r"(0x18));
	serial_info("TSS loaded");

	serial_info("multiboot pointer: 0x%x", mboot_addr);
	
	interrupt_init();

	/*  kernel.c - the C part of the kernel */
	/*  Copyright (C) 1999, 2010  Free Software Foundation, Inc.
	*
	* This program is free software: you can redistribute it and/or modify
	* it under the terms of the GNU General Public License as published by
	* the Free Software Foundation, either version 3 of the License, or
	* (at your option) any later version.
	*
	* This program is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY; without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	* GNU General Public License for more details.
	*
	* You should have received a copy of the GNU General Public License
	* along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

	// FSF COPIED CODE START
	u32 multiboot_size = *(unsigned *) mboot_addr;
	serial_info("announced mbi size 0x%x", multiboot_size);

	u64 mem_start_mbi = 0, mem_size_mbi = 0;

	struct multiboot_tag *tag = (struct multiboot_tag *) (mboot_addr + 8);
	fb_printf("0x%x\n", tag);
	while (tag->type != MULTIBOOT_TAG_TYPE_END) {
		serial_info("tag 0x%x, Size 0x%x", tag->type, tag->size);
		switch (tag->type) {
			case MULTIBOOT_TAG_TYPE_CMDLINE:
				serial_info("command line = %s", ((struct multiboot_tag_string *) tag)->string);
				break;
			case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
				serial_info("boot loader name = %s", ((struct multiboot_tag_string *) tag)->string);
				break;
			case MULTIBOOT_TAG_TYPE_MODULE:
				serial_info(
					"module at 0x%x-0x%x. Command line %s",
					((struct multiboot_tag_module *) tag)->mod_start,
					((struct multiboot_tag_module *) tag)->mod_end,
					((struct multiboot_tag_module *) tag)->cmdline
				);
				break;
			case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
				serial_info(
					"mem_lower = %uKB, mem_upper = %uKB",
					((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
					((struct multiboot_tag_basic_meminfo *) tag)->mem_upper
				);
				break;
			case MULTIBOOT_TAG_TYPE_BOOTDEV:
				serial_info(
					"boot device 0x%x, 0x%x, 0x%x",
					((struct multiboot_tag_bootdev *) tag)->biosdev,
					((struct multiboot_tag_bootdev *) tag)->slice,
					((struct multiboot_tag_bootdev *) tag)->part
				);
				break;
			case MULTIBOOT_TAG_TYPE_MMAP: {
				serial_info("mmap");
				multiboot_memory_map_t *mmap = ((struct multiboot_tag_mmap *) tag)->entries;
				while((multiboot_uint8_t *) mmap < (multiboot_uint8_t *) tag + tag->size) {
					serial_info(
						" base_addr = 0x%x, length = 0x%x, type = %s, 0x%x",
						mmap->addr,
						mmap->len,
						MULTIBOOT_ENTRY_TYPES[(unsigned) mmap->type],
						mmap
					);
					if (mmap->addr != 0 && mmap->type == MBOOT_MEM_AVAILABLE) {
						mem_start_mbi = mmap->addr;
						mem_size_mbi = mmap->len;
					}
					mmap = (multiboot_memory_map_t *) ((u64) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size);
				}
				break;
			}
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
				multiboot_uint32_t color;
				unsigned i;
				struct multiboot_tag_framebuffer *tagfb = (struct multiboot_tag_framebuffer *) tag;
				void *fb = (void *) (unsigned long) tagfb->common.framebuffer_addr;
				switch (tagfb->common.framebuffer_type) {
					case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED: {
						unsigned best_distance, distance;
						struct multiboot_color *palette;
					
						palette = tagfb->framebuffer_palette;

						color = 0;
						best_distance = 4*256*256;
					
						for (i = 0; i < tagfb->framebuffer_palette_num_colors; i++) {
							distance = (0xff - palette[i].blue) 
								* (0xff - palette[i].blue)
								+ palette[i].red * palette[i].red
								+ palette[i].green * palette[i].green;
							if (distance < best_distance) {
								color = i;
								best_distance = distance;
							}
						}
					} break;

					case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
						color = ((1 << tagfb->framebuffer_blue_mask_size) - 1) 
						<< tagfb->framebuffer_blue_field_position;
						break;

					case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
						color = '\\' | 0x0100;
						break;

					default:
						color = 0xffffffff;
						break;
				}
				break;
			}
		}

		// move on to next multiboot tag
		// add seven then clear last 3 bits for 8 byte alignment
		tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7));
	}
	tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7));
	serial_info("total mbi size 0x%x", tag - mboot_addr);
	// FSF COPIED CODE END
	
	// un-identity map first 2G
	pml4[0] = 0;
	pdpt_low[0] = 0;
	for (u32 i = 0; i < 512; i++)
		pdt_low[i] = 0, pt_low[i] = 0;
	asm volatile("mov rax, cr3; mov cr3, rax" ::: "memory");
	serial_info("removed 0-2GiB identity map");

	// calculate when the kernel actually ends from info provided by linker
	u64 mem_start = ((u64) &kernel_end) - 0xFFFFFFFF80000000;
	// 4096 byte align
	mem_start = (mem_start + 0x1000) & ~(0x1000 - 1);
	u64 mem_end = mem_start_mbi + mem_size_mbi;
	u64 free_virt_start = pmm_init(mem_start, mem_end);

	pml4 = (u64 *) ((u64) pml4 + KERNEL_OFFSET);
	page_init(pml4);

	vmm_init(free_virt_start);

	u64 *thing1 = (u64 *) vmm_alloc(3);
	u64 *thing2 = (u64 *) vmm_alloc(2);
	for (u32 i = 0; i < 10; i++)
		thing1[i] = 1 << i;
	// page unmap test, should result in page fault
	// page_unmap((u64) thing1);
	for (u32 i = 0; i < 8; i++)
		thing1[i] = 80 * i;
	for (u32 i = 0; i < 8; i++)
		thing2[i] = 10 * i;

	for (u32 i = 0; i < 10; i++)
		fb_printf("%u ", thing1[i]);
	fb_printf("\n");
	for (u32 i = 0; i < 10; i++)
		fb_printf("%u ", thing2[i]);
	fb_printf("\n");

	vmm_free(thing1);
	// destroy header test
	// *((u64 *) ((u64) thing2 - 8)) = 2000;
	vmm_free(thing2);

	u64 *thing3 = (u64 *) vmm_alloc(8);
	vmm_free(thing3);

	fb_printf("ok\n");


	// page_map testing code
	// u64 page_frame = pmm_alloc_kernel();
	// asm("xchg bx, bx");
	//
	// page_map(page_frame, page_frame);
	// asm("xchg bx, bx");
	// fb_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));
	// *((u64 *) page_frame) = 0xDEADBEEF;
	//
	// asm("xchg bx, bx");
	// fb_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));
	//
	// page_unmap(page_frame);
	// asm("xchg bx, bx");
	// fb_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));

	// physical allocation testing code
	// u64 mem1 = pmm_alloc_kernel();
	// pmm_free(mem1);
	// u64 mem2 = pmm_alloc_kernel();
	// u64 mem3 = pmm_alloc_kernel();
	// pmm_free(mem2);
	// pmm_free(mem3);
    // 
	// u64 last_mem;
	// for (u64 i = 0;; i++) {
	// 	if (i % 5 == 4)
	// 		pmm_free(last_mem);
	// 	u64 mem = pmm_alloc_user();
	// 	if (i % 5 == 0)
	// 		last_mem = mem;
	// }
}

