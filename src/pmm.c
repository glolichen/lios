#include <stdbool.h>
#include "pmm.h"
#include "output.h"
#include "const.h"
#include "panic.h"

// each bit represents whether a block of memory is free
// because we do not use PSE for now, block size is 4KB = 4096B
u32 *bitmap;
u32 total_memory, usable_memory, total_blocks, filled_blocks;

void set_block(u32 bit) {
	filled_blocks++;
	bitmap[bit / 32] |= 1 << (31 - bit % 32);
}

void unset_block(u32 bit) {
	filled_blocks--;
	bitmap[bit / 32] &= ~(1 << (31 - bit % 32));
}

// void set_region(u32 base, u32 size) {
// 	base /= PMM_BLOCK_SIZE;
// 	size /= PMM_BLOCK_SIZE;
// 	for (u32 i = base; i < base + size; i++)
// 		set_block(i);
// }

void unset_region(u32 base, u32 size) {
	base /= PMM_BLOCK_SIZE;
	size /= PMM_BLOCK_SIZE;
	for (u32 i = base; i < base + size; i++)
		unset_block(i);
	set_block(0);
}

// bool is_block_free(u32 bit) {
// 	return (bitmap[bit / 32] >> (31 - bit % 32)) & 1;
// }

// location of first [size] free blocks
u32 get_first_free(u32 size) {
	// block 0 can never be free so we can return 0 as an error signal
	if (total_blocks == 0 || filled_blocks == total_blocks)
		return 0;
	u32 address = 0, counter = 0;
	for (u32 i = 0; i < total_blocks / 32; i++) {
		u32 entry = bitmap[i];
		if (entry == 0) {
			counter += 8;
			if (counter >= size)
				return address;
			continue;
		}
		for (u32 j = 0; j < 32; j++) {
			if ((entry >> (31 - j)) & 1) {
				address = i * 32 + j + 1;
				counter = 0;
			}
			else
				counter++;
			if (counter == size)
				return address;
		}
	}
	return 0;
}

void pmm_init(multiboot_info_t *info, u32 magic, u32 bitmap_location) {
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
		panic("Invalid multiboot magic number!");
	if (!(info->flags >> 6 & 0x1))
		panic("Invalid multiboot memory map!");

	// in bytes	
	total_memory = 0;
	serial_info("Memory map:");
	for (u32 i = 0; i < info->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *map = (multiboot_memory_map_t *) (info->mmap_addr + i);
		serial_info(
			"Region %d: start %x, length %x, %s",
			i / sizeof(multiboot_memory_map_t),
			(u32) map->addr, (u32) map->len,
			MULTIBOOT_ENTRY_TYPES[map->type]
		);
		total_memory += map->len;
		if (map->type == MULTIBOOT_MEMORY_AVAILABLE)
			usable_memory += map->len;
	}
	total_blocks = total_memory / PMM_BLOCK_SIZE;
	filled_blocks = total_blocks;

	serial_info("Detected total memory size: %dKiB (%d blocks)", total_memory / 1024, total_blocks);
	serial_info("Detected available memory size: %dKiB", usable_memory / 1024);

	bitmap = (u32 *) bitmap_location;
	serial_info("Bitmap initialized at %x", bitmap);
	for (u32 i = 0; i < total_blocks / 32; i++)
		bitmap[i] = 0xFFFFFFFF;
	
	for (u32 i = 0; i < info->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *map = (multiboot_memory_map_t *) (info->mmap_addr + i);
		if (map->type == MULTIBOOT_MEMORY_AVAILABLE) {
			unset_region(map->addr, map->len);
		}
	}

	serial_info("%d blocks used, %d blocks free", filled_blocks, total_blocks - filled_blocks);
	// for (u32 i = 0; i < 50; i++)
	// 	serial_info("%d: %x", i, bitmap[i]);

	// testing code
	// set_block(5);
	// set_block(28);

	// PhysicalAddress mem = pmm_allocate_blocks(82);

	// serial_info("%d blocks used, %d blocks free", filled_blocks, total_blocks - filled_blocks);
	// for (u32 i = 0; i < 50; i++)
	// 	serial_info("%d: %x", i, bitmap[i]);

	// pmm_free_blocks(mem, 82);

	// serial_info("%d blocks used, %d blocks free", filled_blocks, total_blocks - filled_blocks);
	// for (u32 i = 0; i < 50; i++)
	// 	serial_info("%d: %x", i, bitmap[i]);
}

PhysicalAddress pmm_allocate_blocks(u32 size) {
	if (size > total_blocks - filled_blocks)
		return 0;

	u32 address = get_first_free(size);
	if (address == 0)
		return 0;

	for (u32 i = 0; i < size; i++)
		set_block(address + i);

	return address * PMM_BLOCK_SIZE;
}
void pmm_free_blocks(PhysicalAddress address, u32 size) {
	if (!address)
		return;
	u32 base = ((u32) address) / PMM_BLOCK_SIZE;
	for (u32 i = 0; i < size; i++)
		unset_block(base + i);
}