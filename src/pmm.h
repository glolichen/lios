#ifndef PMM_H
#define PMM_H

#include <stdbool.h>
#include "const.h"

void pmm_init();
void pmm_use_block(u32 bit);
void pmm_free_block(u32 bit);
bool pmm_is_block_free(u32 bit);
u32 pmm_get_first_free();

#endif