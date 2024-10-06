#ifndef PAGE_H
#define PAGE_H

#include "const.h"
#include "interrupt.h"

void page_init(u64 *pml4);
void page_fault_handler(struct InterruptData *data);
PhysicalAddress page_virt_to_phys_addr(u64 virt);
void page_map(u64 virt, PhysicalAddress phys);
void page_unmap(u64 virt);

#endif
