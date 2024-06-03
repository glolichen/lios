#ifndef PMM_H
#define PMM_H

#include <stdbool.h>
#include "multiboot.h"
#include "const.h"

void pmm_init(multiboot_info_t *info, u32 magic);
void pmm_use_block(u32 bit);
void pmm_free_block(u32 bit);
bool pmm_is_block_free(u32 bit);
u32 pmm_get_first_free();

#endif