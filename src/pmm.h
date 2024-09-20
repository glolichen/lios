#ifndef PMM_H
#define PMM_H

#include <stdbool.h>
#include "const.h"

void pmm_init(u64 start, u64 end);
u64 pmm_alloc_kernel();
u64 pmm_alloc_user();
void pmm_free(u64 mem);

#endif
