#include <stdbool.h>
#include "page.h"
#include "output.h"
#include "panic.h"
#include "const.h"
#include "interrupt.h"
#include "pmm.h"

// virtual address: 10 bits directory index, 10 bits table index, 12 bits offset in page
// u32 virt_addr_get_directory(u32 addr) {
// 	return (addr >> 22) & 0x3FF;
// }
// u32 virt_addr_get_table(u32 addr) {
// 	return (addr >> 12) & 0x3FF;
// }
// u32 virt_addr_get_offset(u32 addr) {
// 	return addr & 0xFFF;
// }

// void table_entry_set_flag(PageTableEntry *table_entry, enum PageTableFlags flag) {
// 	*table_entry |= flag;
// }
// void table_entry_unset_flag(PageTableEntry *table_entry, enum PageTableFlags flag) {
// 	*table_entry &= ~flag;
// }
// bool table_entry_query_flag(PageTableEntry *table_entry, enum PageTableFlags flag) {
// 	return (*table_entry & flag) == flag;
// }
// u32 table_entry_get_addr(PageTableEntry *table_entry) {
// 	return *table_entry & 0xFFFFF000;
// }
// void table_entry_clear_addr(PageTableEntry *table_entry) {
// 	*table_entry &= 0xFFF;
// }
// void table_entry_set_addr(PageTableEntry *table_entry, PhysicalAddress addr) {
// 	*table_entry |= addr & 0xFFFFF000;
// }

// void directory_set_flag(PageDirectoryEntry *directory, enum PageDirectoryFlags flag) {
// 	*directory |= flag;
// }
// void directory_unset_flag(PageDirectoryEntry *directory, enum PageDirectoryFlags flag) {
// 	*directory &= ~flag;
// }
// bool directory_query_flag(PageDirectoryEntry *directory, enum PageDirectoryFlags flag) {
// 	return (*directory & flag) == flag;
// }
// // get address of page table from page directory entry
// PageTable *directory_get_addr(PageDirectoryEntry *directory) {
// 	return (PageTable *) (*directory & 0xFFFFF000);
// }
// void directory_clear_addr(PageDirectoryEntry *directory) {
// 	*directory &= 0xFFF;
// }
// void directory_set_addr(PageDirectoryEntry *directory, PhysicalAddress addr) {
// 	*directory |= addr & 0xFFFFF000;
// }

// PageDirectoryTable *directory_table;

// PageTableEntry *get_table_entry_from_virt_addr(PageTable *table, u32 virt_addr) {
// 	return virt_addr ? &table->table[virt_addr_get_table(virt_addr)] : 0;
// }
// PageDirectoryEntry *get_directory_from_virt_addr(PageDirectoryTable *directory_table, u32 virt_addr) {
// 	return virt_addr ? &directory_table->table[virt_addr_get_directory(virt_addr)] : 0;
// }

// // map physical address to virtual address
// bool page_map_addr(PhysicalAddress phys_addr, u32 virt_addr) {
// 	PageDirectoryEntry *directory = &directory_table->table[virt_addr_get_directory(virt_addr)];
// 	if (!directory_query_flag(directory, PDF_PRESENT)) {
// 		// directory is not present => there is no page table
// 		// allocate it here
// 		PageTable *table = (PageTable *) pmm_allocate_blocks(1);
// 		if (!table)
// 			return false;
// 		for (u32 i = 0; i < PAGES_PER_TABLE; i++)
// 			table->table[i] = 0;
		
// 		// do the usual initialization ritual on the directory
// 		directory_set_flag(directory, PDF_PRESENT);
// 		directory_set_flag(directory, PDF_WRITABLE);
// 		directory_set_addr(directory, (PhysicalAddress) table);
// 	}
	
// 	PageTable *table = directory_get_addr(directory);
// 	PageTableEntry *table_entry = &table->table[virt_addr_get_table(virt_addr)];

// 	table_entry_set_addr(table_entry, phys_addr);
// 	table_entry_set_flag(table_entry, PTF_PRESENT);
// }

// bool page_alloc(PageTableEntry *table_entry) {
// 	PhysicalAddress block = pmm_allocate_blocks(1);
// 	if (!block)
// 		return false;
	
// 	table_entry_set_flag(table_entry, PTF_PRESENT);
// 	table_entry_clear_addr(table_entry);
// 	table_entry_set_addr(table_entry, block);
	
// 	return true;
// }

// void page_free(PageTableEntry *table_entry) {
// 	PhysicalAddress phys_addr = *table_entry & 0xFFFFF000;
// 	pmm_free_blocks(phys_addr, 1);
// 	table_entry_unset_flag(table_entry, PTF_PRESENT);
// }

// void page_init() {
// 	directory_table = (PageDirectoryTable *) pmm_allocate_blocks(1);
// 	if (!directory_table)
// 		panic("Out of memory!");

// 	for (int i = 0; i < 1024; i++) {
// 		directory_table->table[i] = 0;
// 		directory_set_flag(&directory_table->table[i], PDF_WRITABLE);
// 	}

// 	// https://wiki.osdev.org/User:Neon/Recursive_Paging
// 	directory_set_addr(&directory_table->table[1023], (PhysicalAddress) directory_table);
	
// 	PageTable *page_table = (PageTable *) pmm_allocate_blocks(1);
// 	if (!page_table)
// 		panic("Out of memory!");

// 	PageTableEntry entry = 0;
// 	table_entry_set_flag(&entry, PTF_PRESENT);
// 	table_entry_set_flag(&entry, PTF_WRITABLE);

// 	for (u32 i = 0, addr = 0; i < 1024; i++, addr += PAGE_SIZE) {
// 		table_entry_clear_addr(&entry);
// 		table_entry_set_addr(&entry, addr);
// 		page_table->table[i] = entry;
// 	}

// 	directory_set_flag(&directory_table->table[0], PDF_PRESENT);
// 	directory_set_flag(&directory_table->table[0], PDF_WRITABLE);
// 	directory_clear_addr(&directory_table->table[0]);
// 	directory_set_addr(&directory_table->table[0], (PhysicalAddress) page_table);

// 	asm volatile(
// 		"mov eax, %0;"
// 		"mov cr3, eax;"
// 		"mov eax, cr0;"
// 		"or eax, 0x80000001;"
// 		"mov cr0, eax;" :: "m"(directory_table) : "memory"
// 	);
// }

void page_fault_handler(struct InterruptData *data) {
	u64 address;
	asm volatile("mov %0, cr2" : "=g"(address));
	fb_printf("address: 0x%x\n", address);
}