#include <stdbool.h>
#include "page.h"
#include "output.h"
#include "const.h"
#include "interrupt.h"
#include "pmm.h"

PML4 *pml4_addr;

void invplg(u64 addr) {
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
PDPT *pml4e_get_addr(PML4E *pml4e) {
	return (PDPT *) (*pml4e & 0xFFFFFFFFFF000);
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
PDT *pdpe_get_addr(PDPE *pdpe) {
	return (PDT *) (*pdpe & 0xFFFFFFFFFF000);
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
PT *pde_get_addr(PDE *pde) {
	return (PT *) (*pde & 0xFFFFFFFFFF000);
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

PhysicalAddress page_virt_to_phys_addr(u64 virt) {
	u64 pml4e = virt_addr_get_pml4e(virt);
	u64 pdpe = virt_addr_get_pdpe(virt);
	u64 pde = virt_addr_get_pde(virt);
	u64 pte = virt_addr_get_pte(virt);

	if (!pml4e_query_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_PRESENT))
		return -1;
	PDPT *pdpt_addr = pml4e_get_addr(&pml4_addr->table[pml4e]);
	pdpt_addr = (PDPT *) ((u64) pdpt_addr + KERNEL_OFFSET);

	if (!pdpe_query_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_PRESENT))
		return -1;
	PDT *pdt_addr = pdpe_get_addr(&pdpt_addr->table[pdpe]);
	pdt_addr = (PDT *) ((u64) pdt_addr + KERNEL_OFFSET);

	if (!pde_query_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_PRESENT))
		return -1;
	PT *pt_addr = pde_get_addr(&pdt_addr->table[pde]);
	pt_addr = (PT *) ((u64) pt_addr + KERNEL_OFFSET);
	
	if (!pte_query_flag(&pt_addr->table[pte], PTE_PRESENT))
		return -1;
	
	return pte_get_addr(&pt_addr->table[pte]);
}

bool page_map(u64 virt, PhysicalAddress phys) {
	u64 pml4e = virt_addr_get_pml4e(virt);
	u64 pdpe = virt_addr_get_pdpe(virt);
	u64 pde = virt_addr_get_pde(virt);
	u64 pte = virt_addr_get_pte(virt);

	PDPT *pdpt_addr = pml4e_get_addr(&pml4_addr->table[pml4e]);
	if (!pml4e_query_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_PRESENT)) {
		u64 allocated_pdpt_addr = pmm_alloc_kernel();
		serial_info("page: no pdpt present, created new at 0x%x", allocated_pdpt_addr);
		pml4e_set_addr(&pml4_addr->table[pml4e], allocated_pdpt_addr);
		pml4e_set_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_PRESENT);
		pml4e_set_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_WRITABLE);
		pdpt_addr = (PDPT *) allocated_pdpt_addr;
	}
	pdpt_addr = (PDPT *) ((u64) pdpt_addr + KERNEL_OFFSET);

	PDT *pdt_addr = pdpe_get_addr(&pdpt_addr->table[pdpe]);
	if (!pdpe_query_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_PRESENT)) {
		u64 allocated_pdt_addr = pmm_alloc_kernel();
		serial_info("page: no pdt present, created new at 0x%x", allocated_pdt_addr);
		pdpe_set_addr(&pdpt_addr->table[pdpe], allocated_pdt_addr);
		pdpe_set_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_PRESENT);
		pdpe_set_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_WRITABLE);
		pdt_addr = (PDT *) allocated_pdt_addr;
	}
	pdt_addr = (PDT *) ((u64) pdt_addr + KERNEL_OFFSET);

	PT *pt_addr = pde_get_addr(&pdt_addr->table[pde]);
	if (!pde_query_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_PRESENT)) {
		u64 allocated_pt_addr = pmm_alloc_kernel();
		serial_info("page: no page table present, created new at 0x%x", allocated_pt_addr);
		pde_set_addr(&pdt_addr->table[pde], allocated_pt_addr);
		pde_set_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_PRESENT);
		pde_set_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_WRITABLE);
		pt_addr = (PT *) allocated_pt_addr;
	}
	pt_addr = (PT *) ((u64) pt_addr + KERNEL_OFFSET);

	pte_set_addr(&pt_addr->table[pte], phys);
	pte_set_flag(&pt_addr->table[pte], PTE_PRESENT);
	pte_set_flag(&pt_addr->table[pte], PTE_WRITABLE);

	return true;
}


// TODO: FREE unused tables/structures!
bool page_unmap(u64 virt) {
	u64 pml4e = virt_addr_get_pml4e(virt);
	u64 pdpe = virt_addr_get_pdpe(virt);
	u64 pde = virt_addr_get_pde(virt);
	u64 pte = virt_addr_get_pte(virt);

	bool used_otherwise = false;

	PDPT *pdpt_addr = pml4e_get_addr(&pml4_addr->table[pml4e]);
	pdpt_addr = (PDPT *) ((u64) pdpt_addr + KERNEL_OFFSET);

	PDT *pdt_addr = pdpe_get_addr(&pdpt_addr->table[pdpe]);
	pdt_addr = (PDT *) ((u64) pdt_addr + KERNEL_OFFSET);

	PT *pt_addr = pde_get_addr(&pdt_addr->table[pde]);
	pt_addr = (PT *) ((u64) pt_addr + KERNEL_OFFSET);
	pt_addr->table[pte] = 0;

	// for (u32 i = 0; i < 512; i++) {
	// 	if (pte_query_flag(&pt_addr->table[i], PTE_PRESENT)) {
	// 		used_otherwise = true;
	// 		break;
	// 	}
	// }
	// if (!used_otherwise) {
	// 	serial_info("page: free unused pdt 0x%x", pdt_addr);
	// 	// need to calculate physical address, then free that part
	// 	pmm_free((u64) pdt_addr);
	// }

	invplg(virt);

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

