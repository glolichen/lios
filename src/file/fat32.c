#include "fat32.h"
#include "gpt.h"
#include "../io/output.h"
#include "../mem/vmalloc.h"
#include "../util/panic.h"
#include "nvme.h"

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

u32 FAT_size;
u64 volume_first_lba, volume_last_lba;
u64 FAT_start_lba, data_start_lba;

struct BPB_FAT32 *bpb;

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

	serial_info("FAT32: FAT start LBA: 0x%x", FAT_start_lba);
	serial_info("FAT32: FAT size: %u", FAT_size);
	serial_info("FAT32: data start LBA: 0x%x", data_start_lba);

	fat32_read(1);
}

void read_FAT(void *buffer) {
	// nvme_read is somewhat broken, reading more than 1 LBA is all 0??
	for (u32 i = 0; i < FAT_size; i++)
		nvme_read(FAT_start_lba + i, 1, (void *) ((u64) buffer + i * 512));
}

// FAT spec P14
bool is_end_of_cluster(u32 num) {
	u64 temp = num & 0xFFFFFFF;
	return temp >= 0xFFFFFF8 && num <= 0xFFFFFFF;
}

// provide pointer to cluter # of file's directory table entry
void *fat32_read(u32 cluster) {
	struct FAT_DirectoryEntry *data_section = vcalloc(512);
	nvme_read(data_start_lba + cluster / 16, 1, data_section);

	struct FAT_DirectoryEntry entry = data_section[cluster % 16];
	u32 start_cluster = (entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;

	u32 *FAT = vcalloc(FAT_size);
	read_FAT(FAT);

	u64 current_sector = start_cluster, num_sectors = 0;
	do {
		num_sectors++;
		serial_info("%u", current_sector);
		current_sector = FAT[current_sector];
		if (current_sector == 0)
			panic("FAT32: broken FAT!");
	} while (!is_end_of_cluster(current_sector));

	vfree(FAT);
	vfree(data_section);

	return 0;
}
