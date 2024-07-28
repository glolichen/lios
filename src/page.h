#ifndef PAGE_H
#define PAGE_H

#include "const.h"
#include "interrupt.h"

void page_init();
void page_fault_handler(struct InterruptData *data);

#endif
