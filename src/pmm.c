#include <stdbool.h>
#include "pmm.h"
#include "printf.h"
#include "const.h"
#include "panic.h"

// each bit represents whether a block of memory is free
// because we do not use PSE for now, block size is 4KB = 4096B
u32 *bitmap;
u64 total_memory, usable_memory, total_blocks, filled_blocks;

void pmm_set_block(u64 bit) {
	filled_blocks++;
	bitmap[bit / 32] |= 1 << (31 - bit % 32);
}

void pmm_unset_block(u64 bit) {
	filled_blocks--;
	bitmap[bit / 32] &= ~(1 << (31 - bit % 32));
}

void pmm_set_region(u64 base, u64 size) {
	base /= PMM_BLOCK_SIZE;
	size /= PMM_BLOCK_SIZE;
	for (u64 i = base; i < base + size; i++)
		pmm_set_block(i);
}

void pmm_unset_region(u64 base, u64 size) {
	base /= PMM_BLOCK_SIZE;
	size /= PMM_BLOCK_SIZE;
	for (u64 i = base; i < base + size; i++)
		pmm_unset_block(i);
	pmm_set_block(0);
}

bool pmm_is_block_free(u64 bit) {
	return (bitmap[bit / 32] >> (31 - bit % 32)) & 1;
}

// location of first [size] free blocks
i64 pmm_get_first_free(u64 size) {
	if (total_blocks == 0 || filled_blocks == total_blocks)
		return -1;
	i64 address = 0, counter = 0;
	for (u64 i = 0; i < total_blocks / 32; i++) {
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
	return -1;
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
			"Region %d: start %x, length %x, size %d, type %d (%s)",
			i / sizeof(multiboot_memory_map_t),
			map->addr, map->len, map->size, map->type,
			MULTIBOOT_ENTRY_TYPES[map->type]
		);
		total_memory += map->len;
		if (map->type == MULTIBOOT_MEMORY_AVAILABLE)
			usable_memory += map->len;
	}
	total_blocks = total_memory / PMM_BLOCK_SIZE;
	filled_blocks = total_blocks;

	serial_info("Detected total memory size: %xKiB (%x blocks)", total_memory / 1024, total_blocks);
	serial_info("Detected available memory size: %xKiB", usable_memory / 1024);

	bitmap = (u32 *) bitmap_location;
	serial_info("Bitmap initialized at %d", bitmap);
	for (u64 i = 0; i < total_blocks / 32; i++)
		bitmap[i] = 0xFFFFFFFF;
	
	for (u32 i = 0; i < info->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *map = (multiboot_memory_map_t *) (info->mmap_addr + i);
		if (map->type == MULTIBOOT_MEMORY_AVAILABLE) {
			pmm_unset_region(map->addr, map->len);
		}
	}

	serial_info("%x blocks used, %x blocks free", filled_blocks, total_blocks - filled_blocks);
	for (u64 i = 0; i < 50; i++)
		serial_info("%x: %d", i, bitmap[i]);

	// testing code
	// pmm_set_block(5);
	// pmm_set_block(28);

	// void *mem = pmm_allocate_blocks(82);

	// serial_info("%x blocks used, %x blocks free", filled_blocks, total_blocks - filled_blocks);
	// for (u64 i = 0; i < 50; i++)
	// 	serial_info("%x: %d", i, bitmap[i]);

	// pmm_free_blocks(mem, 82);

	// serial_info("%x blocks used, %x blocks free", filled_blocks, total_blocks - filled_blocks);
	// for (u64 i = 0; i < 50; i++)
	// 	serial_info("%x: %d", i, bitmap[i]);
}

void *pmm_allocate_blocks(u64 size) {
	if (size > total_blocks - filled_blocks)
		return (void *) 0;

	i64 address = pmm_get_first_free(size);
	if (address == -1)
		return (void *) 0;

	for (u64 i = 0; i < size; i++)
		pmm_set_block(address + i);

	return (void *) (address * PMM_BLOCK_SIZE);
}
void pmm_free_blocks(void *address, u64 size) {
	u64 base = ((u64) address) / PMM_BLOCK_SIZE;
	for (u64 i = 0; i < size; i++)
		pmm_unset_block(base + i);
}