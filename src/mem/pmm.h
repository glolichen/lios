#ifndef PMM_H
#define PMM_H

#include "../util/const.h"

void pmm_set_total(u64 size);
void pmm_add_block(u64 start, u64 end);
void pmm_init_final(void);

PhysicalAddress pmm_alloc_low(void);
PhysicalAddress pmm_alloc_high(void);
void pmm_free(PhysicalAddress mem);
void pmm_clear_blocks(u64 start, u64 end);
void pmm_log_status(void);

#endif
