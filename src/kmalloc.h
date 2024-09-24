#ifndef KMALLOC_H
#define KMALLOC_H

#include "const.h"

void *kmalloc(u32 size);
void kfree(void *mem);

#endif
