#include "pmm.h"
#include "../const.h"
#include "../io/output.h"

void *kmalloc_page(void) {
	return (void *) (u64) (pmm_alloc_low() + KERNEL_OFFSET);
}

void *kcalloc_page(void) {
	u64 *page = (u64 *) (pmm_alloc_low() + KERNEL_OFFSET);
	for (u32 i = 0; i < PAGE_SIZE / sizeof(u64); i++)
		page[i] = 0;
	return (void *) page;
}

void kfree_page(u64 addr) {
	pmm_free(addr - KERNEL_OFFSET);
}

