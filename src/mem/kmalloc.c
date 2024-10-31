#include "pmm.h"
#include "../const.h"
#include "../io/output.h"

u64 kmalloc_page(void) {
	return pmm_alloc_low() + KERNEL_OFFSET;
}

u64 kcalloc_page(void) {
	u64 *page = (u64 *) (pmm_alloc_low() + KERNEL_OFFSET);
	for (u32 i = 0; i < PAGE_SIZE / sizeof(u64); i++)
		page[i] = 0;
	return (u64) page;
}

void kfree_page(u64 addr) {
	pmm_free(addr - KERNEL_OFFSET);
}

