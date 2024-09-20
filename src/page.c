#include <stdbool.h>
#include "page.h"
#include "output.h"
#include "const.h"
#include "interrupt.h"

PML4 *pml4_addr;

void page_invplg(u64 addr) {
	asm volatile("invlpg [%0]" :: "r"(addr) : "memory");
}

// https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/40332.pdf P597
u64 virt_addr_get_pml4e(u64 addr) {
	return (addr >> 39) & 0x1FF;
}
u64 virt_addr_get_pdpe(u64 addr) {
	return (addr >> 30) & 0x1FF;
}
u64 virt_addr_get_pde(u64 addr) {
	return (addr >> 21) & 0x1FF;
}
u64 virt_addr_get_pte(u64 addr) {
	return (addr >> 12) & 0x1FF;
}
u64 virt_addr_get_offset(u64 addr) {
	return addr & 0xFFF;
}

// Page map lavel 4 table entry related functions
void pml4e_set_flag(PML4E *pml4e, enum PML4E_PDPE_PDE_Flags flag) {
	*pml4e |= flag;
}
void pml4e_unset_flag(PML4E *pml4e, enum PML4E_PDPE_PDE_Flags flag) {
	*pml4e &= ~flag;
}
bool pml4e_query_flag(PML4E *pml4e, enum PML4E_PDPE_PDE_Flags flag) {
	return (*pml4e & flag) == flag;
}
PDPE *pml4e_get_addr(PML4E *pml4e) {
	return (PDPE *) (*pml4e & 0xFFFFFFFFFF000);
}
void pml4e_clear_addr(PML4E *pml4e) {
	*pml4e &= 0xFFF;
}
void pml4e_set_addr(PML4E *pml4e, PhysicalAddress addr) {
	*pml4e |= addr & 0xFFFFFFFFFF000;
}

// Page directory pointer table entry related functions
void pdpe_set_flag(PDPE *pdpe, enum PML4E_PDPE_PDE_Flags flag) {
	*pdpe |= flag;
}
void pdpe_unset_flag(PDPE *pdpe, enum PML4E_PDPE_PDE_Flags flag) {
	*pdpe &= ~flag;
}
bool pdpe_query_flag(PDPE *pdpe, enum PML4E_PDPE_PDE_Flags flag) {
	return (*pdpe & flag) == flag;
}
PDE *pdpe_get_addr(PDPE *pdpe) {
	return (PDE *) (*pdpe & 0xFFFFFFFFFF000);
}
void pdpe_clear_addr(PDPE *pdpe) {
	*pdpe &= 0xFFF;
}
void pdpe_set_addr(PDPE *pdpe, PhysicalAddress addr) {
	*pdpe |= addr & 0xFFFFFFFFFF000;
}

// Page directory entry related functions
void pde_set_flag(PDE *pde, enum PML4E_PDPE_PDE_Flags flag) {
	*pde |= flag;
}
void pde_unset_flag(PDE *pde, enum PML4E_PDPE_PDE_Flags flag) {
	*pde &= ~flag;
}
bool pde_query_flag(PDE *pde, enum PML4E_PDPE_PDE_Flags flag) {
	return (*pde & flag) == flag;
}
PTE *pde_get_addr(PDE *pde) {
	return (PTE *) (*pde & 0xFFFFFFFFFF000);
}
void pde_clear_addr(PDE *pde) {
	*pde &= 0xFFF;
}
void pde_set_addr(PDE *pde, PhysicalAddress addr) {
	*pde |= addr & 0xFFFFFFFFFF000;
}

// Page table entry related functions
void pte_set_flag(PTE *pte, enum PTE_Flags flag) {
	*pte |= flag;
}
void pte_unset_flag(PTE *pte, enum PTE_Flags flag) {
	*pte &= ~flag;
}
bool pte_query_flag(PTE *pte, enum PTE_Flags flag) {
	return (*pte & flag) == flag;
}
PhysicalAddress pte_get_addr(PTE *pte) {
	return *pte & 0xFFFFFFFFFF000;
}
void pte_clear_addr(PTE *pte) {
	*pte &= 0xFFF;
}
void pte_set_addr(PTE *pte, PhysicalAddress addr) {
	*pte |= addr & 0xFFFFFFFFFF000;
}

// PageDirectoryTable *directory_table;

// PageTableEntry *get_table_entry_from_virt_addr(PageTable *table, u32 virt_addr) {
// 	return virt_addr ? &table->table[virt_addr_get_table(virt_addr)] : 0;
// }
// PageDirectoryEntry *get_directory_from_virt_addr(PageDirectoryTable *directory_table, u32 virt_addr) {
// 	return virt_addr ? &directory_table->table[virt_addr_get_directory(virt_addr)] : 0;
// }

bool page_map(PhysicalAddress phys, u64 virt) {
	u64 pml4e = virt_addr_get_pml4e(virt);
	u64 pdpe = virt_addr_get_pdpe(virt);
	u64 pde = virt_addr_get_pde(virt);
	u64 pte = virt_addr_get_pte(virt);
	fb_printf("phys: 0x%x\n", phys);
	fb_printf("%u %u %u %u\n", pml4e, pdpe, pde, pte);

	if (pml4e_query_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_PRESENT))
		fb_printf("present");

	return true;
}

void page_init(u64 *pml4) {
	pml4_addr = (PML4 *) pml4;
}

void page_fault_handler(struct InterruptData *data) {
	u64 address;
	asm volatile("mov %0, cr2" : "=g"(address));
	fb_printf("address: 0x%x\n", address);
}

