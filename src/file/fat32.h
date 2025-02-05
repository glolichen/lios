#ifndef FAT32_H
#define FAT32_H

#include "gpt.h"

enum FAT32_ReadError {
	FILE_NAME_TOO_LONG,
	FILE_EXT_TOO_LONG,
	FILE_NOT_FOUND,
};
struct FAT32_ReadResult {
	void *ptr;
	union {
		u64 size;
		enum FAT32_ReadError error;
	} size_or_error;
};

void fat32_init(struct Partition part);
struct FAT32_ReadResult fat32_read(const char *name, const char *ext);

#endif

