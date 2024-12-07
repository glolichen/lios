#ifndef KMALLOC_H
#define KMALLOC_H

#include "../const.h"

void *kmalloc_page(void);
void *kcalloc_page(void);
void kfree_page(u64 addr);

#endif
