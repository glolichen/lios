#ifndef PAGE_H
#define PAGE_H

#include <stdbool.h>
#include "../util/const.h"
#include "../int/interrupt.h"

void page_init(u64 *pml4);
void page_fault_handler(const struct InterruptData *data);
PhysicalAddress page_virt_to_phys_addr(u64 virt);
void page_map(u64 virt, PhysicalAddress phys/*, bool writable, bool execute*/);
void page_unmap(u64 virt);

#endif
