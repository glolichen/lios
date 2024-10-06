#ifndef PMM_H
#define PMM_H

#include "../const.h"

u64 pmm_init(u64 start, u64 end);
PhysicalAddress pmm_alloc_kernel();
PhysicalAddress pmm_alloc_user();
void pmm_free(PhysicalAddress mem);

#endif
