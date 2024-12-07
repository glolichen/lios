#ifndef NVME_H
#define NVME_H

#include <stdbool.h>
#include "acpi.h"

struct NVMEDevice *nvme_find(struct MCFG *mcfg);
void nvme_init(volatile struct NVMEDevice *nvme);
bool nvme_read(volatile struct NVMEDevice* nvme, u64 lba_start,
			   u16 num_lbas, volatile void *buffer);
bool nvme_write(volatile struct NVMEDevice *nvme, u64 lba_start,
				u16 num_lbas, volatile void *buffer);

#endif

