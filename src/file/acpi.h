#ifndef ACPI_H
#define ACPI_H

#include "efi.h"
#include "../const.h"

// https://wiki.osdev.org/RSDP
// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#root-system-description-pointer-rsdp-structure
struct __attribute__((packed)) RSDP2 {
	u64 signature;
	u8 checksum;
	u8 oem_id[6];
	u8 revision;
	u32 deprecated;
	u32 length;
	u64 xsdt_addr;
	u8 ext_checksum;
	u8 reserved[3];
};

// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html?highlight=description_header#system-description-table-header
struct __attribute__((packed)) DescriptionHeader {
	u32 signature;
	u32 length;
	u8 revision;
	u8 checksum;
	u8 oem_id[6];
	u8 oem_table_id[8];
	u32 oem_revision;
	u32 creator_id;
	u32 creator_revision;
};

// https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#extended-system-description-table-xsdt
struct __attribute__((packed)) XSDT {
	struct DescriptionHeader header;
	u64 entries[];
};

// https://wiki.osdev.org/PCI_Express
struct __attribute__((packed)) MCFGEntry {
	u64 addr;
	u16 pci_group;
	u8 start_pci;
	u8 end_pci;
	u32 reserved;
};
struct __attribute__((packed)) MCFG {
	struct DescriptionHeader header;
	u64 reserved;
	struct MCFGEntry entries[];
};

struct MCFG *find_acpi(const EFI_SYSTEM_TABLE *efi_table);

#endif
