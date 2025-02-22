#ifndef FAT32_H
#define FAT32_H

#include "gpt.h"

typedef u32 FileDescriptor;

enum FAT32_ReadError {
	FAT32_READ_NAME_TOO_LONG,
	FAT32_READ_EXT_TOO_LONG,
	FAT32_READ_NOT_FOUND,
};
enum FAT32_NewFileError {
	FAT32_NEW_FILE_NAME_TOO_LONG,
	FAT32_NEW_FILE_EXT_TOO_LONG,
	FAT32_NEW_FILE_EXISTS,
	FAT32_NEW_FILE_NO_SPACE,
};
struct FAT32_ReadResult {
	void *ptr;
	union {
		u64 size;
		enum FAT32_ReadError error;
	} size_or_error;
};
struct FAT32_NewFileResult {
	FileDescriptor fd;
	enum FAT32_NewFileError error;
};

void fat32_init(struct Partition part);
struct FAT32_ReadResult fat32_open_and_read(const char *name, const char *ext);
struct FAT32_NewFileResult fat32_new_file(const char *name, const char *ext);

#endif

