#ifndef HEAP_H
#define HEAP_H

#include "const.h"

void heap_init();
void *kmalloc(u64 size);
void kfree(void *mem);
void heap_log_status();

#endif
