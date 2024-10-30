#include "pmm.h"
#include "../const.h"

u64 kmalloc_page(void) {
	return pmm_alloc_low() + KERNEL_OFFSET;
}

void kfree_page(u64 addr) {
	pmm_free(addr - KERNEL_OFFSET);
}

