#ifndef NVME_H
#define NVME_H

#include <stdbool.h>
#include "acpi.h"

bool nvme_find(struct MCFG *mcfg);
void nvme_init(void);

#endif

