#ifndef VMALLOC_H
#define VMALLOC_H

#include "../const.h"

void vmalloc_init(void);
void *vmalloc(u64 size);
void *vcalloc(u64 size);
void vfree(const void *mem);
void vmalloc_log_status(void);

#endif
