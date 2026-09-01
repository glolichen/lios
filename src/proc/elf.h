#ifndef ELF_H
#define ELF_H

#include <stdbool.h>
#include "../util/const.h"

struct ELFSegmentMem {
	u64 vaddr;
	u64 num_pages;
	u64 *pages;
};

struct ELFContext {
	void *file_buffer;
	u64 *proc_stack_pages;
	struct ELFSegmentMem *segments;
	// if entry_point = 0 load has failed
	u64 entry_point, rbp_rsp, stack_size;
};

struct ELFContext elf_load(const char *name, const char *ext);

#endif
