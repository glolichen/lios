#include <stdbool.h>

#include "nvme.h"
#include "acpi.h"
#include "../util/panic.h"
#include "../io/output.h"
#include "../mem/page.h"
#include "../mem/kmalloc.h"
#include "../mem/vmalloc.h"

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
	u32 dword0; // command specific
	u32 dword1; // command specific
	u32 dword2;
	u32 dword3;
};

u64 pci_get_addr(u64 base, u8 bus, u8 device, u8 function, u8 offset) {
	return (bus << 20 | device << 15 | function << 12) + base + offset;
}

struct NVMeDevice *nvme_find(const struct MCFG *mcfg) {
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

					if (header->class_code == 1 && header->subclass == 8 && header->prog_if == 2) {
						serial_info("bar0, bar1: 0x%x 0x%x", header->bar0, header->bar1);

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

		u64 nvme_base = VIRT_ADDR;
		page_map((u64) nvme_base, nvme_base_addr);
		page_map((u64) nvme_base + 0x1000, nvme_base_addr + 0x1000);

		return (struct NVMeDevice *) nvme_base;
	}

	return 0;
}

volatile struct SubmissionEntry *admin_submit, *io_submit;
volatile struct CompletionEntry *admin_complete, *io_complete;
volatile u32 *admin_submit_doorbell, *io_submit_doorbell, *io_complete_doorbell;
u32 admin_submission_tail, admin_completion_head;
u32 io_submission_tail, io_completion_head;

void log_dword3_info(u64 dword3) {
	serial_info("nvme: dword 3: 0x%x", dword3);
	serial_info("    status code type: 0x%x", (dword3 >> 25) & 7);
	serial_info("    status code: 0x%x", (dword3 >> 17) & 0xFF);
}
bool completion_check_success(u64 dword3) {
	// status code type is not 0 (general status) then there is problem
	if ((dword3 >> 25) & 7) {
		log_dword3_info(dword3);
		return false;
	}
	// status code is 0 and type is 0 means successful
	if ((dword3 >> 17) & 0xFF) {
		log_dword3_info(dword3);
		return false;
	}
	return true;
}

void nvme_init(volatile struct NVMeDevice *nvme) {
	serial_info("nvme device 0x%x", nvme);

	// disable controller and wait
	nvme->CC = 0;

	// otherwise the compiler optimizes away this loop
	while (nvme->CSTS & 1)
		asm volatile("nop");

	// capabilities check
	// TODO: ???? what is this???
	// u8 min_page_size = (nvme->CAP >> 48) & 0xF;
	// u8 max_page_size = (nvme->CAP >> 52) & 0xF;
	// if (!(0 >= min_page_size && 0 <= max_page_size))
	// 	panic("nvme: page size not supported!");

	// FIXME: why does this cause error?
	u8 command_sets = (nvme->CAP >> 37) & 0xFF;
	if (!(command_sets & 1))
		panic("nvme: controller does not support nvm command set!");

	// IO completion queue size is 4, IO submission queue size is 6 (spec P318)
	// host memory page is not modified, default 0 means 4 KiB
	nvme->CC = (6 << 16) | (4 << 20);
	nvme->AQA = (63 << 16) | 63;

	// set admin submission/completion queue address
	admin_submit = (struct SubmissionEntry *) kcalloc_page();
	admin_complete = (struct CompletionEntry *) kcalloc_page();
	nvme->ASQ = page_virt_to_phys_addr((u64) admin_submit);
	nvme->ACQ = page_virt_to_phys_addr((u64) admin_complete);

	// re-enable controller
	nvme->CC |= 1;
	while (!(nvme->CSTS & 1))
		asm volatile("nop");

	admin_submission_tail = 0;
	admin_completion_head = 0;
	
	// nvme over pcie spec P9-10, base spec P54
	u32 doorbell_stride = 4 << ((nvme->CAP >> 32) & 0xF);
	admin_submit_doorbell = (u32 *) (u64) nvme->doorbells;
	// we will create IO queue at slot 1
	io_submit_doorbell = (u32 *) (u64) (nvme->doorbells + 2 * doorbell_stride);
	io_complete_doorbell = (u32 *) (u64) (nvme->doorbells + 3 * doorbell_stride);

	io_complete = (struct CompletionEntry *) kcalloc_page();
	admin_submit[admin_submission_tail] = (const struct SubmissionEntry) {0};
	admin_submit[admin_submission_tail].opcode = 5;
	admin_submit[admin_submission_tail].identifier = 0;
	admin_submit[admin_submission_tail].nsid = 0;
	admin_submit[admin_submission_tail].dword10 = (63 << 16) | 1; // size 64 identifier 1
	admin_submit[admin_submission_tail].dword11 = 1; // disable interrupts, physically contiguous
	admin_submit[admin_submission_tail].prp1 = page_virt_to_phys_addr((u64) io_complete);
	*admin_submit_doorbell = ++admin_submission_tail;
	// wait for phase change / new entry in completion queue
	while (!((admin_complete[admin_completion_head].dword3 >> 16) & 1))
		asm volatile("nop");

	// status code type (SCT) 0 usually means success, spec P138-140, 419-420
	if (!completion_check_success(admin_complete[admin_completion_head].dword3))
		panic("nvme: failure when creating IO completion queue!");

	serial_info(
		"nvme: created IO completion queue at 0x%x",
		 page_virt_to_phys_addr((u64) io_complete)
	);
	admin_completion_head++;

	// create io submission queue (spec P102-103)
	io_submit = (struct SubmissionEntry *) kcalloc_page();
	admin_submit[admin_submission_tail] = (const struct SubmissionEntry) {0};
	admin_submit[admin_submission_tail].opcode = 1;
	admin_submit[admin_submission_tail].identifier = 0;
	admin_submit[admin_submission_tail].nsid = 0;
	admin_submit[admin_submission_tail].dword10 = (63 << 16) | 1; // size 64 identifier 1
	admin_submit[admin_submission_tail].dword11 = (1 << 16) | 1; // completion identifier 1, physically contiguous
	admin_submit[admin_submission_tail].prp1 = page_virt_to_phys_addr((u64) io_submit);
	*admin_submit_doorbell = ++admin_submission_tail;
	while (!((admin_complete[admin_completion_head].dword3 >> 16) & 1))
		asm volatile("nop");

	if (!completion_check_success(admin_complete[admin_completion_head].dword3))
		panic("nvme: failure when creating IO submission queue!");
	serial_info(
		"nvme: created IO submission queue at 0x%x",
		 page_virt_to_phys_addr((u64) io_submit)
	);
	admin_completion_head++;

	io_submission_tail = 0;
	io_completion_head = 0;

	// serial_info("0x%x %u", admin_tail_doorbell, *admin_tail_doorbell);
	// serial_info("0x%x", CQ0TDBL);
	// serial_info("0x%x", SQ1TDBL);
	// serial_info("0x%x", CQ1TDBL);
	//
	// vga_printf("total size 0x%x\n", sizeof(struct NVMERegisters));

	// serial_info("%u %u", sizeof(struct CommandDword0), sizeof(struct SubmissionQueueEntry));
}

// TODO: tail wrap around after reaching 63 (max)
// it should work, but might also be broken

// NOTE: buffer must be dword (4 byte) aligned!
bool nvme_read(u64 lba_start, u16 num_lbas, void *buffer) {
	// command set P48-51
	io_submit[io_submission_tail] = (const struct SubmissionEntry) {0};
	io_submit[io_submission_tail].opcode = 2;
	io_submit[io_submission_tail].identifier = 2;
	// ??? somehow nsid 1 works ???
	io_submit[io_submission_tail].nsid = 1;

	// dwords 10, 11 are bits 0:31, 32:63 of lba start respectively
	io_submit[io_submission_tail].dword10 = lba_start & 0xFFFFFFFF;
	io_submit[io_submission_tail].dword11 = (lba_start >> 32) & 0xFFFFFFFF;
	// dword 12 is number of sectors minus 1, leave everything else as 0
	io_submit[io_submission_tail].dword12 = num_lbas - 1;
	io_submit[io_submission_tail].prp1 = page_virt_to_phys_addr((u64) buffer);

	u8 phase = (io_complete[io_completion_head].dword3 >> 16) & 1;
	io_submission_tail = (io_submission_tail + 1) % 64;
	*io_submit_doorbell = io_submission_tail;

	// wait for phase change / new entry in completion queue
	while (((io_complete[io_completion_head].dword3 >> 16) & 1) == phase)
		asm volatile("nop");

	io_completion_head = (io_completion_head + 1) % 64;
	*io_complete_doorbell = io_completion_head;

	if (!completion_check_success(io_complete[io_completion_head].dword3))
		return false;

	serial_info("nvme: read %u block(s) starting at offset 0x%x", num_lbas, lba_start);
	return true;
}

// NOTE: buffer must be dword (4 byte) aligned!
bool nvme_write(u64 lba_start, u16 num_lbas, void *buffer) {
	// command set P53-56
	io_submit[io_submission_tail] = (const struct SubmissionEntry) {0};
	io_submit[io_submission_tail].opcode = 1;
	io_submit[io_submission_tail].identifier = 1;
	// ??? somehow nsid 1 works ???
	io_submit[io_submission_tail].nsid = 1;

	// dwords 10, 11 are bits 0:31, 32:63 of lba start respectively
	io_submit[io_submission_tail].dword10 = lba_start & 0xFFFFFFFF;
	io_submit[io_submission_tail].dword11 = (lba_start >> 32) & 0xFFFFFFFF;
	// dword 12 is number of sectors minus 1, leave everything else as 0
	io_submit[io_submission_tail].dword12 = num_lbas - 1;
	io_submit[io_submission_tail].prp1 = page_virt_to_phys_addr((u64) buffer);

	u8 phase = (io_complete[io_completion_head].dword3 >> 16) & 1;
	io_submission_tail = (io_submission_tail + 1) % 64;
	*io_submit_doorbell = io_submission_tail;

	// wait for phase change / new entry in completion queue
	while (((io_complete[io_completion_head].dword3 >> 16) & 1) == phase)
		asm volatile("nop");

	io_completion_head = (io_completion_head + 1) % 64;
	*io_complete_doorbell = io_completion_head;

	if (!completion_check_success(io_complete[io_completion_head].dword3))
		return false;

	serial_info("nvme: written %u block(s) starting at offset 0x%x", num_lbas, lba_start);
	return true;
}

