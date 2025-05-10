#include "acpi.h"
#include "nvme.h"
#include "../util/const.h"
#include "../util/panic.h"
#include "../io/output.h"

#include "efi.h"
#include "efidef.h"
#include "efiapi.h"

#include <stdbool.h>

bool is_same_guid(EFI_GUID *a, EFI_GUID *b) {
	for (u32 i = 0; i < sizeof(a->Data4) / sizeof(a->Data4[0]); i++) {
		if (a->Data4[i] != b->Data4[i])
			return false;
	}
	return a->Data1 == b->Data1 && a->Data2 == b->Data2 && a->Data3 == b->Data3;
}

struct MCFG *find_acpi(const EFI_SYSTEM_TABLE *efi_table) {
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

	return mcfg;
}

