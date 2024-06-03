#include <stdbool.h>
#include "pmm.h"
#include "printf.h"
#include "const.h"
#include "panic.h"

// each bit represents whether a block of memory is free
// because we do not use PSE for now, block size is 4KB = 4096B
u32 *bitmap;

void pmm_init(multiboot_info_t *info, u32 magic) {
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
		panic("Invalid multiboot magic number!");
	if (!(info->flags >> 6 & 0x1))
		panic("Invalid multiboot memory map!");

	bitmap = (u32 *) ((multiboot_memory_map_t *) info->mmap_addr)->addr;

	u64 memory_size = 0;
	serial_info("Memory map:");
	for (int i = 0; i < info->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *map = (multiboot_memory_map_t *) (info->mmap_addr + i);
		serial_info(
			"Region %d: start %x, length %x, size %d, type %d (%s)",
			i / sizeof(multiboot_memory_map_t),
			map->addr, map->len, map->size, map->type,
			MULTIBOOT_ENTRY_TYPES[map->type]
		);
		if (map->type == MULTIBOOT_MEMORY_AVAILABLE) {
			memory_size += map->len;

		}
	}
	serial_info("Detected memory size: %xKiB", memory_size / 1024);


}

void pmm_use_block(u32 bit) {
	bitmap[bit / 32] |= 1 << (bit % 32);
}

void pmm_free_block(u32 bit) {
	bitmap[bit / 32] &= ~(1 << (bit % 32));
}

bool pmm_is_block_free(u32 bit) {
	return (bitmap[bit / 32] >> (bit % 32)) & 1;
}

u32 pmm_get_first_free() {
	return 0;
}
