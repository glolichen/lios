#include <stdbool.h>

#include "pci.h"
#include "acpi.h"
#include "../io/output.h"
#include "../mem/page.h"

#define VIRT_ADDR 0xFFFF900000000000

u32 pci_read_32(u64 base, u8 bus, u8 device, u8 function, u8 offset) {
	return *((u32 *) ((bus << 20 | device << 15 | function << 12) + base + offset));
}
bool pci_device_exists(u32 vendor_device_id) {
    return (vendor_device_id & 0xFFFF) != 0xFFFF;
}

void list_pci_devices(struct MCFG *mcfg) {
	for (size_t i = 0; i < (mcfg->header.length - sizeof(struct MCFG)) / sizeof(struct MCFGEntry); i++) {
		struct MCFGEntry entry = mcfg->entries[i];

		// 256 << 20 = maximum address that could be accessed in this process
		for (u64 i = 0; i < (256 << 20) / PAGE_SIZE; i++)
			page_map(VIRT_ADDR + i * PAGE_SIZE, entry.addr + i * PAGE_SIZE);

		for (u32 bus = entry.start_pci; bus <= entry.end_pci; bus++) {
			for (u32 device = 0; device < 32; device++) {
				for (u32 function = 0; function < 8; function++) {
					u32 vendor_device_id = pci_read_32(VIRT_ADDR, bus, device, function, 0x00);
					if (!pci_device_exists(vendor_device_id))
						continue;

					u16 vendor_id = vendor_device_id & 0xFFFF;
					u16 device_id = (vendor_device_id >> 16) & 0xFFFF;

					// https://wiki.osdev.org/PCI#Common_Header_Fields
					u32 register2 = pci_read_32(VIRT_ADDR, bus, device, function, 0x08);
					u8 class_code = (register2 >> 24) & 0xFF;
					u8 subclass = (register2 >> 16) & 0xFF;
					u8 prog_if = (register2 >> 8) & 0xFF;

					serial_info("PCIe device: bus %u, device %u, function %u", bus, device, function);
					serial_info("    vendor id 0x%x, device id 0x%x", vendor_id, device_id);
					serial_info("    class code 0x%x, subclass 0x%x, prog if 0x%x", class_code, subclass, prog_if);

					vga_printf("PCIe device: bus %u, device %u, function %u\n", bus, device, function);
					if (class_code == 1) {
						vga_printf("    vendor id 0x%x, device id 0x%x\n", vendor_id, device_id);
						vga_printf("    class code 0x%x, subclass 0x%x, prog if 0x%x\n\n", class_code, subclass, prog_if);
					}
				}
			}
		}
	}
}

