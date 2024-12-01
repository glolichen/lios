#ifndef NVME_H
#define NVME_H

#include <stdbool.h>
#include "acpi.h"

bool nvme_find(struct MCFG *mcfg);
void nvme_init(void);
bool nvme_read(u64 lba_start, u16 num_lbas, volatile void *buffer);
bool nvme_write(u64 lba_start, u16 num_lbas, volatile void *buffer);

#endif

