#include <stdbool.h>
#include "elf.h"
#include "elfdefs.h"
#include "proc.h"
#include "../io/output.h"
#include "../file/fat32.h"
#include "../mem/vmalloc.h"
#include "../mem/pmm.h"
#include "../mem/page.h"
#include "../util/hexdump.h"
#include "../util/kmath.h"
#include "../util/misc.h"

void enter_user_mode(u64 stack_base, u64 entry_point);

// TODO: on program exit syscall these all need to be handled
//  - physical page frames need to be freed
//  - segment vaddr -> page frame mapping to be removed
//  - file buffer freed

struct ELFContext elf_load(const char *name, const char *ext) {
	struct ELFContext ctx = (struct ELFContext) { 0 };

	struct FAT32_OpenResult file_data = fat32_open(name, ext);
	if (file_data.cluster == 0) {
		serial_error("file %s.%s not found!", name, ext);
		goto failure1;
	}

	serial_info("file read size: %u\n", file_data.size_or_error.size);
	ctx.file_buffer = vcalloc(file_data.size_or_error.size * 512);
	fat32_read(file_data.cluster, file_data.size_or_error.size, ctx.file_buffer);

	hexdump(ctx.file_buffer, file_data.size_or_error.size * 512, true);

	struct ELF_Header *header = (struct ELF_Header *) ctx.file_buffer;

	// for below, see Wikipedia page section "ELF header"

	// ELF files start eith 0x7F, E, L, F
	const unsigned char *ident = header->e_ident;
	if (ident[0] != 0x7F || ident[1] != 'E' || ident[2] != 'L' || ident[3] != 'F')
		goto failure2;

	// we only execute 64 bit binaries
	if (ident[4] != 2)
		goto failure2;

	// must be little-endian, as x86 architecture is that way
	if (ident[5] != 1)
		goto failure2;

	// must be current version of ELF
	if (ident[6] != 1)
		goto failure2;

	// NOTE: Wikipedia says the next bytes encode "Target ABI"
	// but the ELF spec says not, and since there is current no
	// ABI to speak of right now for us, we will leave it blank
	
	// we can only execute executable (obviously)
	if (header->e_type != 2)
		goto failure2;

	// 0x3E is AMD64/x86-64, which is our target architecture
	if (header->e_machine != 0x3E)
		goto failure2;

	if (header->e_version != 1)
		goto failure2;

	u64 entry_point = header->e_entry;

	// NOTE: ph = program header, sh = section header
	u64 ph_start = header->e_phoff;
	u64 sh_start = header->e_shoff;

	// program header size must equal 64
	if (header->e_ehsize != 64)
		goto failure2;

	u16 ph_entry_size = header->e_phentsize;
	u16 ph_entry_count = header->e_phnum;

	u16 sh_entry_size = header->e_shentsize;
	u16 sh_entry_count = header->e_shnum;

	u16 shstrndx = header->e_shstrndx;

	serial_info("elf: program entry point: 0x%x", entry_point);
	serial_info("elf: program header:");
	serial_info("    start: 0x%x", ph_start);
	serial_info("    entry size: %u", ph_entry_size);
	serial_info("    entry count: %u", ph_entry_count);
	serial_info("elf: section header:");
	serial_info("    start: 0x%x", sh_start);
	serial_info("    entry size: %u", sh_entry_size);
	serial_info("    entry count: %u", sh_entry_count);

	// the program headers/segments are what matters for loading, which is what we're doing
	// sections are for the linker, and the executable files we're dealing with are already linked
	struct ELF_ProgramHeader *ph = (struct ELF_ProgramHeader *) ((u64) header + ph_start);
	
	u64 num_valid_segments = 0;
	for (u16 i = 0; i < ph_entry_count; i++) {
		if (ph[i].p_type == 1)
			num_valid_segments++;
	}

	ctx.segments = vcalloc(num_valid_segments * sizeof(struct ELFSegmentMem));

	for (u16 i = 0, valid_index = 0; i < ph_entry_count; i++) {
		if (ph[i].p_type != 1) {
			serial_warn("elf: found segment with type %u, skipping", ph[i].p_type);
			continue;
		}

		u64 file_offset = ph[i].p_offset;
		u64 virt_addr = ph[i].p_vaddr;
		u64 file_size = ph[i].p_filesz;
		u64 memory_size = ph[i].p_memsz;
		u64 flags = ph[i].p_flags;
		u64 align = ph[i].p_align;

		// FIXME: only works on segments <4KiB, anything larger will break!!!
		// NOTE: fixed above, need to test
		//
		// NOTE: per https://stackoverflow.com/a/31011428, we need to allocate
		// p_memsz bytes for the segment. this is larger than p_filesz because
		// it may contain a .bss section. the part of the memory allocated
		// after the "p_filesz" mark is all zero

		u64 segment_num_pages = ceil_u64_div(memory_size, PAGE_SIZE);
		u64 *segment_pages = vmalloc(segment_num_pages * sizeof(u64));

		for (u64 j = 0; j < segment_num_pages; j++) {
			segment_pages[j] = pmm_alloc_high();
			page_map(virt_addr + j * PAGE_SIZE, segment_pages[j], false);
		}

		void *segment_vaddr = (void *) virt_addr;
		memcpy(segment_vaddr, (void *) ((u64) ctx.file_buffer + file_offset), file_size);
		// zero remaining segment for .bss
		memset((u8 *) segment_vaddr + file_size, 0, memory_size - file_size);

		ctx.segments[valid_index] = (struct ELFSegmentMem) {
			.vaddr = virt_addr,
			.num_pages = segment_num_pages,
			.pages = segment_pages
		};

		serial_info("elf: program header entry %u:", i);
		serial_info("    offset in file: 0x%x", file_offset);
		serial_info("    virtual address: 0x%x", virt_addr);
		serial_info("    file size: 0x%x", file_size);
		serial_info("    memory size: 0x%x", memory_size);
		serial_info("    flags: 0x%x", flags);
		serial_info("    align: 0x%x", align);

		hexdump((void *) virt_addr, memory_size, true);

		valid_index++;
	}

	// vfree(buffer);

	// set up the stack
	// stack starts at 0x800000000000 and ends at 0x800000000000 - 8MiB

	// NOTE: would normally need ceil_u64_div here but
	// STACK_SIZE is a multiple of PAGE_SIZE
	u64 stack_num_pages = STACK_SIZE / PAGE_SIZE;
	ctx.proc_stack_pages = vmalloc(stack_num_pages * sizeof(u64));

	u64 stack_low = LOWER_HALF_MEM_MAX - STACK_SIZE;

	for (u64 i = 0; i < stack_num_pages; i++) {
		ctx.proc_stack_pages[i] = pmm_alloc_high();
		page_map(stack_low + i * PAGE_SIZE, ctx.proc_stack_pages[i], false);
	}

	ctx.entry_point = entry_point;
	ctx.rbp_rsp = LOWER_HALF_MEM_MAX;
	ctx.stack_size = STACK_SIZE;

	return ctx;

	// serial_info("hi");
	// enter_user_mode(LOWER_HALF_MEM_MAX, entry_point);
	// serial_info("hello");

failure2:
	vfree(ctx.file_buffer);
failure1:
	return (struct ELFContext) { 0 };
}

