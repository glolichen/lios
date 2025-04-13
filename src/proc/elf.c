#include <stdbool.h>
#include "elf.h"
#include "elfdefs.h"
#include "../io/output.h"
#include "../file/fat32.h"
#include "../mem/vmalloc.h"
#include "../mem/pmm.h"
#include "../mem/page.h"
#include "../util/hexdump.h"
#include "../util/misc.h"

bool elf_load(const char *name, const char *ext) {
	struct FAT32_OpenResult file_data = fat32_open("HLWORLD", "OUT");
	if (file_data.cluster == 0)
		return false;

	vga_printf("file read size: %u\n", file_data.size_or_error.size);
	void *buffer = vcalloc(file_data.size_or_error.size * 512);
	fat32_read(file_data.cluster, file_data.size_or_error.size, buffer);

	hexdump(buffer, file_data.size_or_error.size * 512, true);

	struct ELF_Header *header = (struct ELF_Header *) buffer;

	// for below, see Wikipedia page section "ELF header"

	// ELF files start eith 0x7F, E, L, F
	const unsigned char *ident = header->e_ident;
	if (ident[0] != 0x7F || ident[1] != 'E' || ident[2] != 'L' || ident[3] != 'F')
		return false;
	
	// we only execute 64 bit binaries
	if (ident[4] != 2)
		return false;

	// must be little-endian, as x86 architecture is that way
	if (ident[5] != 1)
		return false;

	// must be current version of ELD
	if (ident[6] != 1)
		return false;

	// NOTE: Wikipedia says the next bytes encode "Target ABI"
	// but the ELF spec says not, and since there is current no
	// ABI to speak of right now for us, we will leave it blank
	
	// we can only execute executable (obviously)
	if (header->e_type != 2)
		return false;

	// 0x3E is AMD64/x86-64, which is our target architecture
	if (header->e_machine != 0x3E)
		return false;

	if (header->e_version != 1)
		return false;

	u64 entry_point = header->e_entry;

	// NOTE: ph = program heder, sh = section header
	u64 ph_start = header->e_phoff;
	u64 sh_start = header->e_shoff;

	// program header size must equal 64
	if (header->e_ehsize != 64)
		return false;

	u16 ph_entry_size = header->e_phentsize;
	u16 ph_entry_count = header->e_phnum;

	u16 sh_entry_size = header->e_shentsize;
	u16 sh_entry_count = header->e_shnum;

	u16 shstrndx = header->e_shstrndx;

	serial_info("elf: program entry: 0x%x", entry_point);
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
	for (u16 i = 0; i < ph_entry_count; i++) {
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

		void *segment = (void *) virt_addr;
		u64 phys = pmm_alloc_high();
		page_map((u64) segment, phys, true);

		memcpy(segment, (void *) ((u64) buffer + file_offset), file_size);

		serial_info("elf: program header entry %u:", i);
		serial_info("    offset in file: 0x%x", file_offset);
		serial_info("    virtual address: 0x%x", virt_addr);
		serial_info("    file size: 0x%x", file_size);
		serial_info("    memory size: 0x%x", memory_size);
		serial_info("    flags: 0x%x", flags);
		serial_info("    align: 0x%x", align);

		hexdump((void *) virt_addr, memory_size, true);
	}

	// asm volatile("jmp %0" :: "r"(entry_point));

	vfree(buffer);

	return true;
}
