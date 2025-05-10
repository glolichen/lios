#ifndef FAT32_H
#define FAT32_H

#include "gpt.h"

typedef u32 FileDescriptor;

struct FAT32_OpenResult {
	u32 cluster;
	union {
		u32 size;
		enum FAT32_OpenError error;
	} size_or_error;
};
struct FAT32_NewFileResult {
	FileDescriptor fd;
	enum FAT32_NewFileError error;
};

void fat32_init(struct Partition part);
struct FAT32_OpenResult fat32_open(const char *name, const char *ext);
u32 fat32_read(u32 cluster, u32 size, void *buffer);
struct FAT32_NewFileResult fat32_new_file(const char *name, const char *ext);

#endif

