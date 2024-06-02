#include <stdbool.h>
#include "pmm.h"
#include "const.h"

// each bit represents whether a block of memory is free
// because we do not use PSE for now, block size is 4KB = 4096B
u32 *bitmap;

void pmm_init() {

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
