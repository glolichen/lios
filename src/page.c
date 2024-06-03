#include "page.h"
#include "panic.h"
#include "const.h"
#include "pmm.h"

u32 *pdt;

void page_enable();

void page_init() {
	// 1 block = 4096 byte = 1024 u32's (exactly a PDT)
	pdt = pmm_allocate_blocks(1);
	if (!pdt)
		panic("Insufficient memory for PDT!");
	for (u32 i = 0; i < PAGE_TABLES_PER_DIRECTORY; i++)
		pdt[i] = 0x02;
	
	
}