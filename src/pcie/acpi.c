#include "acpi.h"
#include "../const.h"
#include "../panic.h"
#include "../io/output.h"

#include "efi.h"
#include "efidef.h"
#include "efiapi.h"

#include <stdbool.h>

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

bool is_same_guid(EFI_GUID *a, EFI_GUID *b) {
	for (u32 i = 0; i < sizeof(a->Data4) / sizeof(a->Data4[0]); i++) {
		if (a->Data4[i] != b->Data4[i])
			return false;
	}
	return a->Data1 == b->Data1 && a->Data2 == b->Data2 && a->Data3 == b->Data3;
}

void find_acpi(EFI_SYSTEM_TABLE *efi_table) {
	// https://uefi.org/specs/UEFI/2.10_A/04_EFI_System_Table.html#id6
	// https://uefi.org/specs/UEFI/2.10_A/04_EFI_System_Table.html#industry-standard-configuration-tables

	EFI_GUID acpi_guid = (EFI_GUID) ACPI_TABLE_GUID;
	EFI_GUID acpi_guid2 = (EFI_GUID) ACPI_20_TABLE_GUID;

	EFI_CONFIGURATION_TABLE *config_table = efi_table->ConfigurationTable;
	config_table = (EFI_CONFIGURATION_TABLE *) ((u64) config_table + KERNEL_OFFSET);

	struct RSDP2 *rsdp = 0;
	for (u32 i = 0; i < efi_table->NumberOfTableEntries; i++) {
		EFI_GUID guid = config_table[i].VendorGuid;
		if (is_same_guid(&guid, &acpi_guid)) {
			// TODO: figure out what to do for ACPI 1.0 table
		}

		if (is_same_guid(&guid, &acpi_guid2)) {
			u64 *vendor_table = (u64 *) ((u64) config_table[i].VendorTable + KERNEL_OFFSET);
			// 0x2052545020445352 = "RSD PTR "
			if (vendor_table[0] ==	0x2052545020445352) {
				rsdp = (struct RSDP2 *) vendor_table;
				break;
			}
		}
	}

	if (!rsdp)
		panic("acpi: RSDP not found!");

	struct XSDT *xsdt = (struct XSDT *) (rsdp->xsdt_addr + KERNEL_OFFSET);
	struct MCFG *mcfg = 0;
	u32 rsdp_entries = (xsdt->header.length - sizeof(struct XSDT)) / 8;
	for (u32 i = 0; i < rsdp_entries; i++) {
		u64 entry = xsdt->entries[i];
		struct DescriptionHeader *header = (struct DescriptionHeader *) (entry + KERNEL_OFFSET);
		// 0x4746434D = "MCFG"
		if (header->signature == 0x4746434D) {
			mcfg = (struct MCFG *) header;
			break;
		}
	}

	if (!mcfg)
		panic("acpi: MCFG not found!");

	serial_info("0x%x 0x%x", mcfg, mcfg->header.signature);
	u32 mcfg_entries = (mcfg->header.length - sizeof(struct MCFG)) / 16;
	serial_info("%u", mcfg_entries);
}
