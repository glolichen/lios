#ifndef KMALLOC_H
#define KMALLOC_H

#include "../const.h"

u64 kmalloc_page(void);
void kfree_page(u64 addr);

#endif
