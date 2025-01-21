#ifndef VMM_H
#define VMM_H

#include "../util/const.h"

void vmm_init(void);
void *vmm_alloc(u32 pages);
void vmm_free(void *mem);
void vmm_log_status(void);

#endif
