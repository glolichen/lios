#ifndef FAT32_H
#define FAT32_H

#include "gpt.h"

void fat32_init(struct Partition part);
void *fat32_read(u32 cluster);

#endif

