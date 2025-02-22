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
bool is_end_of_cluster(u32 num) {
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

// read a file given its cluster
// returns vmalloc'ed pointer, remember to vfree!
struct FAT32_ReadResult read_file_from_cluster_num(u32 file_start_cluster) {
	// first to store directory entry, then file data
	u64 current_sector = file_start_cluster, num_sectors = 0;
	do {
		num_sectors++;
		current_sector = FAT[current_sector];
		if (current_sector == 0)
			panic("FAT32: broken FAT!");
	} while (!is_end_of_cluster(current_sector));

	void *result = vcalloc(num_sectors * 512);
	current_sector = file_start_cluster;

	for (u32 i = 0; i < num_sectors; i++) {
		// FAT spec P28-29
		u32 sector = data_start_lba + current_sector - 2;
		current_sector = FAT[current_sector];
		nvme_read(sector, 1, (void *) ((u64) result + i * 512));
		// memcpy((void *) ((u64) result + i * 512), temp_data, 512);
	}

	if (!is_end_of_cluster(current_sector))
		panic("FAT32: what is going on?");

	struct FAT32_ReadResult ret;
	ret.ptr = result;
	ret.size_or_error.size = num_sectors * 512;
	return ret;
}

// given the name of a file, find the cluster the file is stored in
// if file does not exist, returns 0
u32 get_cluster_num(const char *name, const char *ext) {
	if (strlen(name) > 8)
		return 0;
	if (strlen(ext) > 3)
		return 0;

	u64 file_sector = 0;
	u64 current_sector = root_dir_lba;
	do {
		file_sector = check_directory_entry_for_file(current_sector, name, ext);
		if (file_sector != 0)
			break;

		current_sector = FAT[current_sector];
		if (current_sector == 0)
			panic("FAT32: broken FAT!");
	} while (!is_end_of_cluster(current_sector));

	if (file_sector == 0)
		return 0;

	serial_info("FAT32: found cluster for file, is 0x%x", file_sector);

	return file_sector;
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

// read a file
// on error, results ptr = 0, size = error code
struct FAT32_ReadResult fat32_open_and_read(const char *name, const char *ext) {
	struct FAT32_ReadResult ret = { 0, { 0 } };
	if (strlen(name) > 8) {
		ret.size_or_error.error = FAT32_READ_NAME_TOO_LONG;
		return ret;
	}
	if (strlen(ext) > 3) {
		ret.size_or_error.error = FAT32_READ_EXT_TOO_LONG;
		return ret;
	}

	u64 file_sector = get_cluster_num(name, ext);
	if (file_sector == 0) {
		ret.size_or_error.error = FAT32_READ_NOT_FOUND;
		return ret;
	}

	serial_info("FAT32: found cluster for file, is 0x%x", file_sector);

	return read_file_from_cluster_num(file_sector);
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

	u32 first_cluster = find_free_FAT_cluster();
	FAT[first_cluster] = 0xFFFFFFF;

	vga_printf("set sector %u\n", first_cluster);
	nvme_write(FAT_start_lba + (first_cluster / 512), 1, (void *) ((u64) FAT + first_cluster - first_cluster % 512));

	// FREE 3 AND 4

	return ret;
	// return read_file_from_cluster_num(file_sector);
}

