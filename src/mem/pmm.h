#ifndef PMM_H
#define PMM_H

#include "../const.h"

void pmm_init(u64 start, u64 end);
PhysicalAddress pmm_alloc_low();
PhysicalAddress pmm_alloc_high();
void pmm_free(PhysicalAddress mem);
void pmm_clear_blocks(u64 start, u64 end);
void pmm_log_status();

#endif
