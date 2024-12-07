#ifndef NVME_H
#define NVME_H

#include <stdbool.h>
#include "acpi.h"

// useful: base P49-?? 
struct __attribute__((packed)) NVMeDevice {
	u64 CAP;	// controller capabilities
	u32 VS;		// version
	u32 INTMS;	// interrupt mask set
	u32 INTMC;	// interrupt mask clear
	u32 CC;		// controller configuration
	u32 reserved;
	u32 CSTS;	// controller status
	u32 NSSR;	// NVMe subsystem reset
	u32 AQA;	// admin queue attributes
	u64 ASQ;	// admin submission queue
	u64 ACQ;	// admin completion queue
	u8 padding[4040];

	// below are offset 0x1000 from base address
	u8 doorbells[];
};

struct NVMeDevice *nvme_find(const struct MCFG *mcfg);
void nvme_init(volatile struct NVMeDevice *nvme);
bool nvme_read(u64 lba_start, u16 num_lbas, void *buffer);
bool nvme_write(u64 lba_start, u16 num_lbas, void *buffer);

#endif

