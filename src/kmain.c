#include "testing.h"

#include "int/interrupt.h"

#include "io/serial.h"
#include "io/output.h"
#include "io/vga.h"

#include "util/const.h"
#include "util/panic.h"
#include "util/kmath.h"
#include "util/hexdump.h"
#include "util/multiboot2.h"
#include "util/misc.h"

#include "mem/vmm.h"
#include "mem/pmm.h"
#include "mem/page.h"
#include "mem/vmalloc.h"
#include "mem/kmalloc.h"

#include "file/acpi.h"
#include "file/nvme.h"
#include "file/gpt.h"
#include "file/fat32.h"

#include "proc/elf.h"

struct __attribute__((packed)) GDTEntryTSS {
	u16 limit_low;
	u16 base_low;
	u8 base_mid;
	u8 access;
	u8 limit_high : 4;
	u8 flags : 4;
	u8 base_high;
	u32 base_highest;
	u32 reserved;
};

struct __attribute__((packed)) TaskStateSegment {
	u32 reserved1;
	u64 rsp0;
	u64 rsp1;
	u64 rsp2;
	u64 reserved2;
	u64 ist1;
	u64 ist2;
	u64 ist3;
	u64 ist4;
	u64 ist5;
	u64 ist6;
	u64 ist7;
	u64 reserved3;
	u16 reserved4;
	u16 iopb;
};

extern u64 kernel_end;

// the top of the stack we switch to after entering ring 3
void enter_user_mode(u64 stack_base);

void kmain(struct GDTEntryTSS *tss_entry, struct TaskStateSegment *tss, u64 tss_end, u64 mboot_addr,
			u64 pml4[512], u64 pdpt_low[512], u64 pdt_low[512], u64 pt_low[512]) {

	serial_init();
	serial_info("kernel end: 0x%x", (u64) &kernel_end - KERNEL_OFFSET);

	u64 tss_start = (u64) tss;
	u64 limit = tss_end - tss_start;
	tss_entry->limit_low = limit & 0xFFFF;
	tss_entry->base_low = tss_start & 0xFFFF;
	tss_entry->base_mid = (tss_start >> 16) & 0xFF;
	tss_entry->access = 0x89;
	tss_entry->limit_high = (limit >> 16) & 0xF;
	tss_entry->flags = 8;
	tss_entry->base_high = (tss_start >> 24) & 0xFF;
	tss_entry->base_highest = (tss_start >> 32) & 0xFFFFFFFF;
	tss_entry->reserved = 0;
	asm volatile("ltr %d0" :: "r"(0x28));
	serial_info("TSS loaded");

	serial_info("multiboot pointer: 0x%x", mboot_addr);

	interrupt_init();

	// enter_user_mode(0x2000, 0x1000);
	// test_div0();
	
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

	u64 total_available_mem = 0;
	EFI_SYSTEM_TABLE *efi_table = 0;

	struct multiboot_tag *tag = (struct multiboot_tag *) (mboot_addr + 8);
	u8 *framebuffer_addr = 0;
	u32 pixel_width = 0, pixel_height = 0, vga_pitch = 0;

	u64 multiboot_start = mboot_addr, multiboot_end = 0;

	while (tag->type != MULTIBOOT_TAG_TYPE_END) {
		serial_info("tag %u, size 0x%x", tag->type, tag->size);
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
						"    base_addr = 0x%x, length = 0x%x, type = %s, 0x%x",
						mmap->addr,
						mmap->len,
						mmap->type <= 5 ? MULTIBOOT_ENTRY_TYPES[(unsigned) mmap->type] : "???",
						mmap
					);

					if (mmap->type == MBOOT_MEM_AVAILABLE && mmap->addr + mmap->len > ((u64) &kernel_end - KERNEL_OFFSET))
						total_available_mem += mmap->addr + mmap->len - u64_max(mmap->addr, ((u64) &kernel_end - KERNEL_OFFSET));

					mmap = (multiboot_memory_map_t *) ((u64) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size);
				}
				break;
			}
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
				multiboot_uint32_t color;
				unsigned i;
				struct multiboot_tag_framebuffer *tagfb = (struct multiboot_tag_framebuffer *) tag;

				framebuffer_addr = (u8 *) tagfb->common.framebuffer_addr;
				serial_info("framebuffer address 0x%x", tagfb->common.framebuffer_addr);
				serial_info("framebuffer type %u", tagfb->common.framebuffer_type);
				serial_info("framebuffer pitch %u", tagfb->common.framebuffer_pitch);
				serial_info("framebuffer width %u", tagfb->common.framebuffer_width);
				serial_info("framebuffer height %u", tagfb->common.framebuffer_height);

				vga_pitch = tagfb->common.framebuffer_pitch;
				pixel_width = tagfb->common.framebuffer_width;
				pixel_height = tagfb->common.framebuffer_height;

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
				serial_info("framebuffer color type %u", color);
				break;
			}
			case MULTIBOOT_TAG_TYPE_EFI64: {
				struct multiboot_tag_efi64 *tag_efi = (struct multiboot_tag_efi64 *) tag;
				serial_info("EFI table at 0x%x", tag_efi->pointer);
				efi_table = (EFI_SYSTEM_TABLE *) ((u64) tag_efi->pointer + KERNEL_OFFSET);
			}
		}

		// move on to next multiboot tag
		// add seven then clear last 3 bits for 8 byte alignment
		tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7));
	}

	serial_info("total mbi size 0x%x", tag - mboot_addr);
	multiboot_end = (u64) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7));

	pmm_set_total(total_available_mem);

	tag = (struct multiboot_tag *) (mboot_addr + 8);
	while (tag->type != MULTIBOOT_TAG_TYPE_END) {
		if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
			multiboot_memory_map_t *mmap = ((struct multiboot_tag_mmap *) tag)->entries;
			while ((multiboot_uint8_t *) mmap < (multiboot_uint8_t *) tag + tag->size) {
				if (mmap->type == MBOOT_MEM_AVAILABLE && mmap->addr + mmap->len > ((u64) &kernel_end - KERNEL_OFFSET)) {
					u64 zone_start = u64_max(mmap->addr, ((u64) &kernel_end - KERNEL_OFFSET));
					zone_start = ceil_u64_div(zone_start, 0x1000) * 0x1000;
					u64 zone_end = mmap->addr + mmap->len;

					u64 start1 = 0, end1 = 0;
					u64 start2 = 0, end2 = 0;

					// the multiboot structure might actually come in the middle of this zone for some reason
					// split it into two zones: start to mmap start and mmap end to end
					if ((multiboot_start >= zone_start && multiboot_start <= zone_end) || 
							(multiboot_end >= zone_start && multiboot_end <= zone_end)) {

						// start1 to end1 will be start to mmap start
						// start2 to end2 will be mmap end to end

						start1 = zone_start;
						end1 = multiboot_start;
						start2 = multiboot_end;
						end2 = zone_end;

						// it's possible that the mmap is not entirely in the middle of this zone
						// if that happens, then one of the start[x] >= end[x], which doesn't make sense

						if (start1 < end1)
							pmm_add_block(start1, end1);
						if (start2 < end2)
							pmm_add_block(start2, end2);
					}
					else
						pmm_add_block(zone_start, zone_end);
				}
				mmap = (multiboot_memory_map_t *) ((u64) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size);
			}
		}

		// move on to next multiboot tag
		// add seven then clear last 3 bits for 8 byte alignment
		tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7));
	}

	// FSF COPIED CODE END

	pmm_init_final();
	pmm_log_status();

	// un-identity map first 2G
	pml4[0] = 0;
	pdpt_low[0] = 0;
	for (u32 i = 0; i < 512; i++)
		pdt_low[i] = 0, pt_low[i] = 0;
	asm volatile("mov rax, cr3; mov cr3, rax" ::: "memory");
	serial_info("removed 0-2GiB identity map");

	page_set_pml4((u64) pml4, (u64) pml4 + KERNEL_OFFSET);

	pmm_clear_blocks((u64) framebuffer_addr, (u64) framebuffer_addr + vga_pitch * pixel_height);

	vmm_init();
	vmm_log_status();

	vmalloc_init();
	vmalloc_log_status();

	vga_init(framebuffer_addr, pixel_width, pixel_height, vga_pitch);
	vga_clear();

	// WARN: renable for QEMU
	if (!efi_table)
		panic("no EFI system table found!");

	struct MCFG *mcfg = find_acpi(efi_table);
	volatile struct NVMeDevice *nvme = nvme_find(mcfg);
	if (!nvme)
		panic("no NVMe device!");

	nvme_init(nvme);

	struct Partition data_part = gpt_read();
	fat32_init(data_part);

	serial_info("setup ok");
	vga_printf("setup ok\n");

	// test_run_tests();

	// test_div0();

	// elf_load("HLWORLD", "OUT");

	// test_run_tests();

	// page_set_pml4((u64) 500, (u64) 500 + KERNEL_OFFSET);

	// hexdump(file_data.ptr, file_data.size_or_error.size, 1);
	// vfree(file_data.ptr);

	// fat32_new_file("file5", "txt");

	u64 new_kernel_stack = (u64) kmalloc_page();
	tss->rsp0 = new_kernel_stack;

	u64 user_stack_phys = pmm_alloc_low();
	u64 *user_stack = (u64 *) 0x800000;
	page_map((u64) user_stack, user_stack_phys, true);
	*user_stack = 100;

	enter_user_mode((u64) user_stack + 0x1000 - 8);
}

