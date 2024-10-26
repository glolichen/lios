#include "acpi.h"
#include "../const.h"
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

void find_acpi(EFI_SYSTEM_TABLE *efi_table) {
	// UEFI spec P91, P163
	EFI_GUID acpi_guid = (EFI_GUID) ACPI_TABLE_GUID;
	EFI_CONFIGURATION_TABLE *config_table = efi_table->ConfigurationTable;
	config_table = (EFI_CONFIGURATION_TABLE *) ((u64) config_table + KERNEL_OFFSET);

	for (u32 i = 0; i < efi_table->NumberOfTableEntries; i++) {
		EFI_GUID guid = config_table[i].VendorGuid;
		if (is_same_guid(&guid, &acpi_guid)) {
			serial_info("%u 0x%x", i, config_table[i].VendorTable);
		}
	}
}
