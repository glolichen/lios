#ifndef PMM_H
#define PMM_H

#include <stdbool.h>
#include "const.h"

void pmm_init(u64 start, u64 size);
void *pmm_alloc();
void pmm_free(void *mem);

#endif
