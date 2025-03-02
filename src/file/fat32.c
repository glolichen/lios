#include "fat32.h"
#include "gpt.h"
#include "nvme.h"
#include "../io/output.h"
#include "../mem/vmalloc.h"
#include "../util/panic.h"
#include "../util/misc.h"
#include "../util/hexdump.h"

// MS FAT spec P7-12
struct __attribute__((packed)) BPB_FAT32 {
	u8 BS_jmpBoot_0;
	u8 BS_jmpBoot_1;
	u8 BS_jmpBoot_2;
	u64 BS_OEMName;
	u16 BPB_BytsPerSec;
	u8 BPB_SecPerClus;
	u16 BPB_RsvdSecCnt;
	u8 BPB_NumFATs;
	u16 BPB_RootEntCnt;
	u16 BPB_TotSec16;
	u8 BPB_Media;
	u16 BPB_FATSz16;
	u16 BPB_SecPerTrk;
	u16 BPB_NumHeads;
	u32 BPB_HiddSec;
	u32 BPB_TotSec32;

	// below are FAT32 specific
	u32 BPB_FATSz32;
	u16 BPB_ExtFlags;
	u16 BPB_FSVer;
	u32 BPB_RootClus;
	u16 BPB_FSInfo;
	u16 BPB_BkBootSec;
	u8 reserved[12];
	u8 BS_DrvNum;
	u8 reserved1;
	u8 BS_BootSig;
	u32 BS_VolID;
	u8 BS_VolLab[11];
	u8 BS_FilSysType[8];
	u8 reserved2[420];
	u16 Signature_word;
};

struct __attribute__((packed)) FAT_DirectoryEntry {
	u8 DIR_Name[11];
	u8 DIR_Attr;
	u8 DIR_NTRes;
	u8 DIR_CrtTimeTenth;
	u16 DIR_CrtTime;
	u16 DIR_CrtDate;
	u16 DIR_LstAccDate;
	u16 DIR_FstClusHI; // high 2 bytes of cluster #
	u16 DIR_WrtTime;
	u16 DIR_WrtDate;
	u16 DIR_FstClusLO; // low 2 bytes of cluster #
	u32 DIR_FileSize;
};

u32 FAT_size; // number of 512 BYTE SECTORS
u64 volume_first_lba, volume_last_lba;
u64 FAT_start_lba, data_start_lba, root_dir_lba;

struct BPB_FAT32 *bpb;

u32 *FAT;
void update_FAT(void) {
	// nvme_read is somewhat broken, reading more than 1 LBA doesn't work
	memset(FAT, 0, 512 * FAT_size * sizeof(u32));
	for (u32 i = 0; i < FAT_size; i++)
		nvme_read(FAT_start_lba + i, 1, (void *) ((u64) FAT + i * 512));
}
u32 find_free_FAT_cluster(void) {
	for (u32 i = 0; i < FAT_size * 512; i++) {
		if (FAT[i] == 0)
			return i;
	}
	return 0;
}

// FAT spec P14
bool is_ending_cluster(u32 num) {
	u64 temp = num & 0xFFFFFFF;
	return temp >= 0xFFFFFF8 && num <= 0xFFFFFFF;
}

// parse 1 directory FAT entry (512 bytes) to see if file is within that directory
// strlen(name) <= 8, strlen(ext) <= 3
u32 check_directory_entry_for_file(u32 sector_num, const char *name, const char *ext) {
	char want_str[12];
	for (u32 i = 0; i < 11; i++)
		want_str[i] = ' ';
	want_str[11] = 0;
	for (u32 i = 0; i < strlen(name); i++)
		want_str[i] = toupper(name[i]);
	for (u32 i = 0; i < strlen(ext); i++)
		want_str[i + 8] = toupper(ext[i]);

	struct FAT_DirectoryEntry *directory = (struct FAT_DirectoryEntry *) vcalloc(512);
	nvme_read(sector_num - 2 + data_start_lba, 1, directory);

	for (u32 i = 0; i < 512 / 32; i++) {
		if (strncmp((const char *) directory[i].DIR_Name, want_str, 11) == 0) {
			u32 return_val = (directory[i].DIR_FstClusHI << 16) | directory[i].DIR_FstClusLO;
			vfree(directory);
			return return_val;
		}
	}

	vfree(directory);
	return 0;
}

// given the name of a file, find the cluster the file is stored in
// if file does not exist, returns 0
u32 get_cluster_num(const char *name, const char *ext) {
	if (strlen(name) > 8)
		return 0;
	if (strlen(ext) > 3)
		return 0;

	u64 file_cluster = 0, directory_cluster = root_dir_lba;
	do {
		file_cluster = check_directory_entry_for_file(directory_cluster, name, ext);
		if (file_cluster != 0)
			break;

		directory_cluster = FAT[directory_cluster];
		if (directory_cluster == 0)
			panic("FAT32: broken FAT!");
	} while (!is_ending_cluster(directory_cluster));

	if (file_cluster == 0)
		return 0;

	serial_info("FAT32: found cluster for file, is 0x%x", file_cluster);

	return file_cluster;
}

// initialize
void fat32_init(struct Partition part) {
	volume_first_lba = part.first_lba, volume_last_lba = part.last_lba;

	// first, check if partition is acceptable
	// BIOS Parameter Block (BPB) is 512 bytes
	bpb = vcalloc(512);
	nvme_read(part.first_lba, 1, bpb);

	if (!(bpb->BS_jmpBoot_0 == 0xEB && bpb->BS_jmpBoot_2 == 0x90) && bpb->BS_jmpBoot_0 != 0xE9)
		panic("FAT32: incorrect BS_jmpBoot!");
	if (bpb->BPB_BytsPerSec != 512)
		panic("FAT32: only 512 bytes per sector supported!");
	if (bpb->BPB_NumFATs != 2)
		panic("FAT32: only 2 FATs supported!");
	if (bpb->BPB_SecPerClus != 1)
		panic("FAT32: only 1 sector per cluster supported!");
	if (bpb->BPB_TotSec16 != 0 || bpb->BPB_FATSz16 != 0 || bpb->BPB_TotSec32 == 0)
		panic("FAT32: appears to be FAT12/16?");
	if (bpb->Signature_word != 0xAA55)
		panic("FAT32: incorrect signature word!");

	FAT_size = bpb->BPB_FATSz32;
	FAT_start_lba = volume_first_lba + bpb->BPB_RsvdSecCnt;
	data_start_lba = FAT_start_lba + FAT_size * 2;
	root_dir_lba = bpb->BPB_RootClus;

	serial_info("FAT32: FAT start LBA: 0x%x", FAT_start_lba);
	serial_info("FAT32: FAT size: %u", FAT_size);
	serial_info("FAT32: data start LBA: 0x%x", data_start_lba);
	serial_info("FAT32: root directory LBA: 0x%x", root_dir_lba);

	FAT = vcalloc(512 * FAT_size * sizeof(u32));
	update_FAT();
}

// opens a file, and returns its cluster number and size
struct FAT32_OpenResult fat32_open(const char *name, const char *ext) {
	struct FAT32_OpenResult ret = { 0, { 0 } };
	if (strlen(name) > 8) {
		ret.size_or_error.error = FAT32_OPEN_NAME_TOO_LONG;
		return ret;
	}
	if (strlen(ext) > 3) {
		ret.size_or_error.error = FAT32_OPEN_EXT_TOO_LONG;
		return ret;
	}

	u32 file_cluster = get_cluster_num(name, ext);
	if (file_cluster == 0) {
		ret.size_or_error.error = FAT32_OPEN_NOT_FOUND;
		return ret;
	}

	ret.cluster = file_cluster;

	u64 current_sector = file_cluster, num_clusters = 0;
	do {
		num_clusters++;
		current_sector = FAT[current_sector];
		if (current_sector == 0)
			panic("FAT32: broken FAT!");
	} while (!is_ending_cluster(current_sector));
	ret.size_or_error.size = num_clusters;

	return ret;
}

// read a file: given starting cluster number and number of clusters
u32 fat32_read(u32 cluster, u32 size, void *buffer) {
	for (u32 i = 0; i < size; i++) {
		// FAT spec P28-29
		u32 sector = data_start_lba + cluster - 2;

		// the last cluster we read should be an ending cluster
		// everything other, should not be an ending cluster
		// then, set the current cluster to the next one
		if (i < size - 1) {
			if (is_ending_cluster(cluster))
				return 1;
			cluster = FAT[cluster];
		}

		// this still reads from the cluster that was before
		// we moveed to the next cluster
		nvme_read(sector, 1, (void *) ((u64) buffer + i * 512));
	}

	// further sanity checking
	if (!is_ending_cluster(cluster))
		return 1;

	return 0;
}

// create a new blank file
// on error, results ptr = 0, size = error code
struct FAT32_NewFileResult fat32_new_file(const char *name, const char *ext) {
	struct FAT32_NewFileResult ret = { 0, 0 };
	if (strlen(name) > 8) {
		ret.error = FAT32_NEW_FILE_NAME_TOO_LONG;
		return ret;
	}
	if (strlen(ext) > 3) {
		ret.error = FAT32_NEW_FILE_EXT_TOO_LONG;
		return ret;
	}

	u64 file_sector = get_cluster_num(name, ext);
	if (file_sector != 0) {
		ret.error = FAT32_NEW_FILE_EXISTS;
		return ret;
	}

	// mark one cluster as used
	u32 file_first_cluster = find_free_FAT_cluster();
	FAT[file_first_cluster] = 0xFFFFFFF;

	// find open directory entry
	u64 usable_index = 16;
	u64 current_cluster = root_dir_lba;
	struct FAT_DirectoryEntry *directory = (struct FAT_DirectoryEntry *) vcalloc(512);
	do {
		// check if current_sector has any open entries
		// file_sector = check_directory_entry_for_file(current_sector, name, ext);
		nvme_read(current_cluster - 2 + data_start_lba, 1, directory);
		for (u32 i = 0; i < 16; i++) {
			// assume name is readable = in use
			// [32, 126] are readable characters
			bool is_readable = true;
			for (u32 j = 0; j < 11; j++) {
				u8 c = directory[i].DIR_Name[j];
				if (c < 32 || c > 126) {
					is_readable = false;
					break;
				}
			}
			if (!is_readable) {
				usable_index = i;
				break;
			}
		}

		if (usable_index != 16)
			break;

		current_cluster = FAT[current_cluster];
		if (current_cluster == 0)
			panic("FAT32: broken FAT!");
	} while (!is_ending_cluster(current_cluster));

	struct FAT_DirectoryEntry entry = {
		.DIR_Name = { 0 },
		.DIR_Attr = 0x20,
		.DIR_NTRes = 0,
		.DIR_CrtTimeTenth = 0,
		.DIR_CrtTime = 0,
		.DIR_CrtDate = 0,
		.DIR_LstAccDate = 0,
		.DIR_FstClusHI = file_first_cluster & 0xFFFF0000,
		.DIR_WrtTime = 0,
		.DIR_WrtDate = 0,
		.DIR_FstClusLO = file_first_cluster & 0x0000FFFF,
		.DIR_FileSize = 512,
	};
	for (u32 i = 0; i < 11; i++)
		entry.DIR_Name[i] = ' ';
	for (u32 i = 0; i < strlen(name); i++)
		entry.DIR_Name[i] = toupper(name[i]);
	for (u32 i = 0; i < strlen(ext); i++)
		entry.DIR_Name[i + 8] = toupper(ext[i]);

	directory[usable_index] = entry;
	
	// write changes to disk

	// occupy one entry in the FAT
	nvme_write(FAT_start_lba + (file_first_cluster / 512), 1, (void *) ((u64) FAT + file_first_cluster - file_first_cluster % 512));
	// write file information to directory structure
	nvme_write(current_cluster - 2 + data_start_lba, 1, directory);

	// zero out first sector of file
	void *empty = vcalloc(512);
	nvme_write(file_first_cluster - 2 + data_start_lba, 1, empty);
	vfree(empty);

	vfree(directory);

	return ret;
}

