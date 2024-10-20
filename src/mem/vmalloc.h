#ifndef VMALLOC_H
#define VMALLOC_H

#include "../const.h"

void vmalloc_init();
void *vmalloc(u64 size);
void vfree(void *mem);
void vmalloc_log_status();

#endif
