#include "const.h"
#include "int/interrupt.h"
#include "multiboot2.h"
#include "testing.h"
#include "panic.h"
#include "kmath.h"

#include "io/serial.h"
#include "io/output.h"
#include "io/vga.h"

#include "mem/vmm.h"
#include "mem/pmm.h"
#include "mem/page.h"
#include "mem/vmalloc.h"
#include "mem/kmalloc.h"

#include "file/acpi.h"
#include "file/nvme.h"
#include "file/gpt.h"
#include "file/fat32.h"

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
	serial_info("kernel end: 0x%x", (u64) &kernel_end - KERNEL_OFFSET);

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

	u64 total_available_mem = 0;
	EFI_SYSTEM_TABLE *efi_table = 0;

	struct multiboot_tag *tag = (struct multiboot_tag *) (mboot_addr + 8);
	u8 *framebuffer_addr = 0;
	u32 pixel_width = 0, pixel_height = 0, vga_pitch = 0;

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

	tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7));
	serial_info("total mbi size 0x%x", tag - mboot_addr);
	pmm_set_total(total_available_mem);

	tag = (struct multiboot_tag *) (mboot_addr + 8);
	while (tag->type != MULTIBOOT_TAG_TYPE_END) {
		if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
			multiboot_memory_map_t *mmap = ((struct multiboot_tag_mmap *) tag)->entries;
			while ((multiboot_uint8_t *) mmap < (multiboot_uint8_t *) tag + tag->size) {
				if (mmap->type != MBOOT_MEM_AVAILABLE)
					goto bad_memory_region;
				if (mmap->addr + mmap->len <= ((u64) &kernel_end - KERNEL_OFFSET))
					goto bad_memory_region;
				pmm_add_block(u64_max(mmap->addr, ((u64) &kernel_end - KERNEL_OFFSET)), mmap->addr + mmap->len);

			bad_memory_region:
				mmap = (multiboot_memory_map_t *) ((u64) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size);
			}

		}
		tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7));
	}

	pmm_init_final();
	pmm_log_status();

	// un-identity map first 2G
	pml4[0] = 0;
	pdpt_low[0] = 0;
	for (u32 i = 0; i < 512; i++)
		pdt_low[i] = 0, pt_low[i] = 0;
	asm volatile("mov rax, cr3; mov cr3, rax" ::: "memory");
	serial_info("removed 0-2GiB identity map");

	pml4 = (u64 *) ((u64) pml4 + KERNEL_OFFSET);
	page_init(pml4);

	pmm_clear_blocks((u64) framebuffer_addr, (u64) framebuffer_addr + vga_pitch * pixel_height);

	vmm_init();
	vmm_log_status();

	vmalloc_init();
	vmalloc_log_status();

	vga_init(framebuffer_addr, pixel_width, pixel_height, vga_pitch);
	vga_clear();

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

	// run_tests(nvme);
}

