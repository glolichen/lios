#include <stdbool.h>

#include "nvme.h"
#include "acpi.h"
#include "../panic.h"
#include "../io/output.h"
#include "../mem/page.h"
#include "../mem/kmalloc.h"

#define VIRT_ADDR 0xFFFF900000000000

// header type 0x0 https://wiki.osdev.org/PCI#Header_Type_0x0
struct __attribute__((packed)) PCIHeader {
	u16 vendor_id, device_id;
	u16 command, status;
	u8 revision_id, prog_if, subclass, class_code;
	u8 cache_line_size, latency_timer, header_type, bist;
	u32 bar0, bar1, bar2, bar3, bar4, bar5;
	u32 cardbus_cis_pointer;
	u16 subsystem_vendor_id, subsystem_id;
	u32 expansion_rom;

	u8 capabilities;
	u8 reserved1;
	u16 reserved2;

	u32 reserved3;
	u8 interrupt_line, interrupt_pin, min_grant, max_latency;
};

struct __attribute__((packed)) NVMERegisters {
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

struct __attribute__((packed)) SubmissionEntry {
	// dword 0
	u8 opcode;
	u8 fused_op : 2;
	u8 reserved1 : 4;
	u8 prp_sgl : 2;
	u16 identifier;

	u32 nsid;			// dword 1
	u64 reserved2;		// dwords 2-3
	u64 metadata_ptr;	// dwords 4-5
	u64 prp1, prp2;		// dwords 6-9
	u32 dword10;
	u32 dword11;
	u32 dword12;
	u32 dword13;
	u32 dword14;
	u32 dword15;
};

struct __attribute__((packed)) CompletionEntry {
	u32 dword0;
	u32 dword1;
	u32 dword2;
	u32 dword3;
};

struct NVMERegisters *nvme_base;

u64 pci_get_addr(u64 base, u8 bus, u8 device, u8 function, u8 offset) {
	return (bus << 20 | device << 15 | function << 12) + base + offset;
}

bool nvme_find(struct MCFG *mcfg) {
	for (size_t i = 0; i < (mcfg->header.length - sizeof(struct MCFG)) / sizeof(struct MCFGEntry); i++) {
		struct MCFGEntry entry = mcfg->entries[i];
		u64 nvme_base_addr = 0;

		// 256 << 20 = maximum address that could be accessed in this process
		for (u64 i = 0; i < (256 << 20) / PAGE_SIZE; i++)
			page_map(VIRT_ADDR + i * PAGE_SIZE, entry.addr + i * PAGE_SIZE);

		for (u32 bus = entry.start_pci; bus <= entry.end_pci; bus++) {
			for (u32 device = 0; device < 32; device++) {
				for (u32 function = 0; function < 8; function++) {
					struct PCIHeader *header = (struct PCIHeader *) pci_get_addr(VIRT_ADDR, bus, device, function, 0);
					if (header->vendor_id == 0xFFFF)
						continue;

					vga_printf("PCIe device: bus %u, device %u, function %u\n", bus, device, function);
					if (header->class_code == 1 && header->subclass == 8 && header->prog_if == 2) {
						serial_info("bar0, bar1: 0x%x 0x%x", header->bar0, header->bar1);
						vga_printf("    vendor id 0x%x, device id 0x%x\n", header->vendor_id, header->device_id);
						vga_printf("    class code 0x%x, subclass 0x%x, prog if 0x%x\n",
							header->class_code, header->subclass, header->prog_if);
						vga_printf("    header type 0x%x\n", header->header_type);

						// https://wiki.osdev.org/NVMe#Base_Address_IO
						nvme_base_addr = ((u64) header->bar1 << 32) | (header->bar0 & 0xFFFFFFF0);

						// enable bus-mastering and memory space and set disable interrupt to false
						header->command |= 6;
						header->command &= ~(1 << 10);

						goto found_nvme_device;
					}
				}
			}
		}

found_nvme_device:
		// nvme_addr has to be 4K aligned anyway
		for (u64 i = 0; i < (256 << 20) / PAGE_SIZE; i++)
			page_unmap(VIRT_ADDR + i * PAGE_SIZE);

		nvme_base = (struct NVMERegisters *) VIRT_ADDR;
		page_map((u64) nvme_base, nvme_base_addr);
		page_map((u64) nvme_base + 0x1000, nvme_base_addr + 0x1000);

		return true;
	}

	return false;
}

u32 *admin_tail_doorbell;
u32 admin_submission_tail, admin_completion_head;

void nvme_init(void) {
	// disable controller and wait
	nvme_base->CC = 0;
	while (nvme_base->CSTS & 1) {
		// otherwise compiler optimizes this into a forever loop, very weird
		asm volatile("nop");
	}

	// capabilities check
	u8 min_page_size = (nvme_base->CAP >> 48) & 0xF;
	u8 max_page_size = (nvme_base->CAP >> 52) & 0xF;
	if (!(0 >= min_page_size && 0 <= max_page_size))
		panic("nvme: page size not supported!");

	u8 command_sets = (nvme_base->CAP >> 37) & 0xFF;
	if (!(command_sets & 1))
		panic("nvme: controller does not support nvm command set!");

	// set IO and admin queue size (all 63)
	nvme_base->CC = (63 << 16) | (63 << 20);
	nvme_base->AQA = (63 << 16) | 63;

	// set admin submission/completion queue address
	struct SubmissionEntry *asq = (struct SubmissionEntry *) kmalloc_page();
	struct CompletionEntry *acq = (struct CompletionEntry *) kmalloc_page();
	nvme_base->ASQ = page_virt_to_phys_addr((u64) asq);
	nvme_base->ACQ = page_virt_to_phys_addr((u64) acq);

	// re-enable controller
	nvme_base->CC |= 1;
	while (!(nvme_base->CSTS & 1))
		asm volatile("nop");

	admin_submission_tail = 0;
	admin_completion_head = 0;
	// nvme over pcie spec 9-10
	admin_tail_doorbell = (u32 *) (u64) nvme_base->doorbells;

	// base spec 54
	// u32 doorbell_stride = 4 << ((nvme_base->CAP >> 32) & 0xF);

	// create IO completion queue (base spec 101-102)
	u64 io_complete_addr = kmalloc_page();
	asq[admin_submission_tail] = (const struct SubmissionEntry) {0};
	asq[admin_submission_tail].opcode = 5;
	asq[admin_submission_tail].identifier = 1;
	asq[admin_submission_tail].nsid = 0;
	asq[admin_submission_tail].dword10 = (63 << 16) | 1; // size 64 identifier 1
	asq[admin_submission_tail].dword11 = 1; // disable interrupts, physically contiguous
	asq[admin_submission_tail].prp1 = page_virt_to_phys_addr(io_complete_addr);
	(*admin_tail_doorbell)++;
	admin_submission_tail++;
	// wait for phase change / new entry in completion queue
	while (!(acq[admin_completion_head].dword3 & 1))
		asm volatile("nop");

	vga_printf("completion ok\n");
	serial_info("ACQ: dword 0: 0x%x", acq[admin_completion_head].dword0);
	serial_info("ACQ: dword 1: 0x%x", acq[admin_completion_head].dword1);
	serial_info("ACQ: dword 2: 0x%x", acq[admin_completion_head].dword2);
	serial_info("ACQ: dword 3: 0x%x", acq[admin_completion_head].dword3);
	admin_completion_head++;

	// create io submission queue (base spec 102-103)
	u64 io_submit_addr = kmalloc_page();
	asq[admin_submission_tail] = (const struct SubmissionEntry) {0};
	asq[admin_submission_tail].opcode = 1;
	asq[admin_submission_tail].identifier = 2;
	asq[admin_submission_tail].nsid = 0;
	asq[admin_submission_tail].dword10 = (63 << 16) | 1; // size 64 identifier 1
	asq[admin_submission_tail].dword11 = (1 << 16) | 1; // completion identifier 1, physically contiguous
	asq[admin_submission_tail].prp1 = page_virt_to_phys_addr(io_submit_addr);
	(*admin_tail_doorbell)++;
	admin_submission_tail++;
	while (!(acq[admin_completion_head].dword3 & 1)) {
		vga_printf("waiting %u\n", acq[admin_completion_head].dword3);
		asm volatile("nop");
	}
	
	vga_printf("submission ok");
	serial_info("ACQ: dword 0: 0x%x", acq[admin_completion_head].dword0);
	serial_info("ACQ: dword 1: 0x%x", acq[admin_completion_head].dword1);
	serial_info("ACQ: dword 2: 0x%x", acq[admin_completion_head].dword2);
	serial_info("ACQ: dword 3: 0x%x", acq[admin_completion_head].dword3);
	admin_completion_head++;

	// u32 *CQ0TDBL = (u32 *) nvme_base->doorbells + doorbell_stride;
	// u32 *SQ1TDBL = (u32 *) nvme_base->doorbells + 2 * doorbell_stride;
	// u32 *CQ1TDBL = (u32 *) nvme_base->doorbells + 3 * doorbell_stride;
	//
	// serial_info("0x%x %u", admin_tail_doorbell, *admin_tail_doorbell);
	// serial_info("0x%x", CQ0TDBL);
	// serial_info("0x%x", SQ1TDBL);
	// serial_info("0x%x", CQ1TDBL);
	//
	// vga_printf("total size 0x%x\n", sizeof(struct NVMERegisters));

	// serial_info("%u %u", sizeof(struct CommandDword0), sizeof(struct SubmissionQueueEntry));
}

