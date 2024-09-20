#ifndef PAGE_H
#define PAGE_H

#include <stdbool.h>
#include "const.h"
#include "interrupt.h"

void page_init(u64 *pml4);
void page_fault_handler(struct InterruptData *data);
bool page_map(PhysicalAddress phys, u64 virt);

#endif
