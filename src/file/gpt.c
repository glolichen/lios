#include "gpt.h"
#include "../util/const.h"
#include "../util/panic.h"
#include "../file/nvme.h"
#include "../io/output.h"
#include "../mem/vmalloc.h"

// https://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_table_header_(LBA_1)
struct __attribute__((packed)) GPTHeader {
	u64 signature;
	u32 revision_no, header_size, header_crc32;
	u32 reserved;
	u64 current_lba, backup_lba, first_lba, last_lba;
	u64 disk_guid[2];
	u64 part_array_start_lba;
	u32 part_array_entries, part_array_entry_size, part_array_crc32;
	u8 reserved2[420];
};

struct __attribute__((packed)) PartitionEntry {
	u64 guid[2];
	u64 partition_guid[2];
	u64 first_lba, last_lba, flags;
	u16 name[36];
};

struct Partition gpt_read(void) {
	struct GPTHeader *header = (struct GPTHeader *) vcalloc(NVME_LBA_SIZE);
	nvme_read(1, 1, header);

	// serial_info("gpt: header");
	// serial_info("    signature: 0x%x", header->signature);
	// serial_info("    revision #: %u", header->revision_no); // likely 65536 = 00 00 01 00
	// serial_info("    header size: %u", header->header_size); // should be 92
	// serial_info("    current lba: 0x%x", header->current_lba); // should be 1
	// serial_info("    backup lba: 0x%x", header->backup_lba);
	// serial_info("    first lba: 0x%x", header->first_lba);
	// serial_info("    last lba: 0x%x", header->last_lba);
	// serial_info("    partition array starting lba: 0x%x", header->part_array_start_lba); // should be 2
	// serial_info("    partition array # entries: %u", header->part_array_entries); // should be 128
	// serial_info("    partition array entry size: %u", header->part_array_entry_size); // should be 128

	// 0x5452415020494645 = "EFI PART" in little endian
	if (header->signature != 0x5452415020494645)
		panic("gpt: no header found at LBA 1");

	if (header->part_array_start_lba != 2 || header->part_array_entry_size != 128 ||
		header->part_array_entry_size != 128 || header->current_lba != 1 || header->header_size != 92) {
		panic("gpt: unknown error");
	}

	struct Partition part = {0};

	// each LBA has 4 entries, or pointer to array of length 4
	struct PartitionEntry (*entries)[4] = (struct PartitionEntry (*)[4]) vcalloc(NVME_LBA_SIZE);
	struct PartitionEntry entry;

	// LBAs 2-33 are partition entries
	for (u32 i = 2; i <= 33; i++) {
		nvme_read(i, 1, entries);
		for (u32 j = 0; j < 4; j++) {
			// dereference entries for array of pointers, then get element
			entry = (*entries)[j];

			// guid is 0 = partition does not exist
			if (!entry.guid[0] && !entry.guid[1])
				continue;

			// FAT is Microsoft basic data partition, GUID is EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
			// https://en.wikipedia.org/wiki/Microsoft_basic_data_partition
			// in little-endian, that would be the below values
			if (entry.guid[0] != 0x4433B9E5EBD0A0A2 || entry.guid[1] != 0xC79926B7B668C087)
				continue;

			if (part.first_lba || part.last_lba) {
				vfree(header);
				vfree(entries);
				panic("gpt: more than 1 FAT partition found! (not yet supported)");
			}

			part.first_lba = entry.first_lba;
			part.last_lba = entry.last_lba;
		}
	}

	vfree(header);
	vfree(entries);

	if (!part.first_lba && !part.last_lba)
		panic("gpt: no FAT partition found!");

	serial_info("gpt: found FAT partition: 0x%x->0x%x", part.first_lba, part.last_lba);
	return part;
}

