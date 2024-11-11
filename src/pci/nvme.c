#include <stdbool.h>

#include "nvme.h"
#include "acpi.h"
#include "../io/output.h"
#include "../mem/page.h"

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
	u32 AQA;	// admin queue attributes
	u64 ASQ;	// admin submission queue
	u64 ACQ;	// admin completion queue
	u8 padding[4044];

	// below are offset 0x1000 from base address
	u8 test;
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

					// serial_info("0x%x 0x%x", class_code, header->class_code);
					// u32 register3 = *((u32 *) pci_get_addr(VIRT_ADDR, bus, device, function, 0xC));https://wiki.osdev.org/PCI#Header_Type_0x0
					// u8 header_type = (register3 >> 16) & 0xFF;
					//
					// serial_info("PCIe device: bus %u, device %u, function %u", bus, device, function);
					// serial_info("    vendor id 0x%x, device id 0x%x",
					// 	vendor_id, device_id);
					// serial_info("    class code 0x%x, subclass 0x%x, prog if 0x%x",
					// 	class_code, subclass, prog_if);

					vga_printf("PCIe device: bus %u, device %u, function %u\n", bus, device, function);
					if (header->class_code == 1 && header->subclass == 8 && header->prog_if == 2) {
						serial_info("bar0, bar1: 0x%x 0x%x", header->bar0, header->bar1);
						vga_printf("    vendor id 0x%x, device id 0x%x\n", header->vendor_id, header->device_id);
						vga_printf("    class code 0x%x, subclass 0x%x, prog if 0x%x\n",
							header->class_code, header->subclass, header->prog_if);
						vga_printf("    header type 0x%x\n", header->header_type);

						// https://wiki.osdev.org/NVMe#Base_Address_IO
						nvme_base_addr = ((u64) header->bar1 << 32) | (header->bar0 & 0xFFFFFFF0);

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

		return true;
	}

	return false;
}

void nvme_init(void) {
	serial_info("CAP: 0x%x", nvme_base->CAP);

	// when I ran this, it prints 0x10400
	// which is major version 1, minor 4, tertiary 0
	// appears to be specification version 1.4 (NVMe spec page 55)
	serial_info("VS: 0x%x", nvme_base->VS);

	// base spec page 54
	u32 doorbell_stride = 1 << (2 + ((nvme_base->CAP >> 32) & 0xF));
	vga_printf("stride 0x%x\n", doorbell_stride);

	vga_printf("total size 0x%x\n", sizeof(struct NVMERegisters));
	vga_printf("0x%x\n", (&nvme_base->test - (u64) nvme_base));
}

