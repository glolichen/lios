#include "pci.h"
#include "../io/output.h"
#include "../const.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

u32 inl(u16 port) {
	u32 val;
	asm volatile("in %0, %1" : "=a"(val) : "Nd"(port));
	return val;
}
void outl(u16 port, u32 data) {
	asm volatile("out %0, %1" :: "Nd"(port), "a"(data));
}

// Function to write to the PCI configuration address
void pci_write_config_address(u32 address) {
    outl(PCI_CONFIG_ADDRESS, address);
}

// Function to read from the PCI configuration data
u32 pci_read_config_data() {
    return inl(PCI_CONFIG_DATA);
}

// Function to create the PCI configuration address for a given bus, device, function, and offset
u32 pci_create_address(u8 bus, u8 device, u8 function, u8 offset) {
    return (u32)(0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC));
}

// Function to read the vendor ID of a PCI device
u16 pci_read_vendor_id(u8 bus, u8 device, u8 function) {
    u32 address = pci_create_address(bus, device, function, 0x00);
    pci_write_config_address(address);
    u32 data = pci_read_config_data();
    return (u16)(data & 0xFFFF);
}

// Function to read the device ID of a PCI device
u16 pci_read_device_id(u8 bus, u8 device, u8 function) {
    u32 address = pci_create_address(bus, device, function, 0x00);
    pci_write_config_address(address);
    u32 data = pci_read_config_data();
    return (u16)((data >> 16) & 0xFFFF);
}

// Function to enumerate all PCI devices on the bus
void pci_enumerate_devices() {
	serial_info("=== PCI ===");
    for (u16 bus = 0; bus < 256; bus++) {
        for (u8 device = 0; device < 32; device++) {
            for (u8 function = 0; function < 8; function++) {
                u16 vendor_id = pci_read_vendor_id(bus, device, function);

                if (vendor_id != 0xFFFF) {  // If vendor ID is 0xFFFF, the device doesn't exist
                    u16 device_id = pci_read_device_id(bus, device, function);
                    serial_info("PCI Device Found: Bus %u, Device %u, Function %u, Vendor ID: 0x%x, Device ID: 0x%x",
                           bus, device, function, vendor_id, device_id);
                }
            }
        }
    }
}

