#ifndef PMM_H
#define PMM_H

#include <stdbool.h>
#include "multiboot.h"
#include "const.h"

void pmm_init(multiboot_info_t *info, u32 magic, u32 bitmap_location);
void *pmm_allocate_blocks(u64 size);
void pmm_free_blocks(void *address, u64 size);

#endif