#ifndef VMM_H
#define VMM_H

#include "../const.h"

void vmm_init(u64 free_virt_start);
void *vmm_alloc(u32 pages);
void vmm_free(void *mem);

void vmm_log_status();

#endif
